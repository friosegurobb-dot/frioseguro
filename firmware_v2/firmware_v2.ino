/*
 * ============================================================================
 * FIRMWARE v4.0 - SISTEMA MONITOREO REEFER INDUSTRIAL
 * Campamento Parametican Silver - Pan American Silver
 * ============================================================================
 * 
 * ARQUITECTURA MODULAR:
 * - config.h        : Configuración centralizada (device_id, pines, umbrales)
 * - types.h         : Estructuras de datos y enums
 * - state_machine.h : Máquina de estados del sistema
 * - sensors.h       : Lectura de sensores (temp, puertas, DHT22)
 * - storage.h       : Almacenamiento en flash (Preferences)
 * - alerts.h        : Lógica de alertas y alarmas
 * - telegram.h      : Notificaciones Telegram
 * - supabase.h      : Integración con Supabase
 * - wifi_utils.h    : Gestión de WiFi
 * - web_api.h       : Servidor web y API REST
 * - html_ui.h       : Página HTML embebida
 * 
 * ESTADOS DEL SISTEMA:
 * - NORMAL: Operación normal, monitoreando
 * - DEFROST: En ciclo de descongelamiento
 * - COOLDOWN: Esperando post-descongelamiento (con timer real)
 * - LOADING_CONFIG: Aplicando nueva configuración
 * - ALERT: Alerta activa
 * - OFFLINE: Sin conexión WiFi
 * - INITIALIZING: Arrancando sistema
 * 
 * CARACTERÍSTICAS:
 * - Device ID centralizado en config.h
 * - Hasta 6 sondas de temperatura DS18B20
 * - Hasta 3 sensores de puerta
 * - 2 salidas de relé
 * - Operación 100% no bloqueante con millis()
 * - Timers reales para cooldown y config
 * 
 * ============================================================================
 */

// ============================================================================
// LIBRERÍAS ESTÁNDAR
// ============================================================================
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Preferences.h>

// ============================================================================
// CONFIGURACIÓN Y TIPOS
// ============================================================================
#include "config.h"
#include "types.h"

// ============================================================================
// OBJETOS GLOBALES
// ============================================================================
WebServer server(80);
WiFiManager wifiManager;
WiFiUDP udpDiscovery;
Preferences prefs;

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature ds18b20(&oneWire);
DHT dht(PIN_DHT22, DHT22);

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================
Config config;
SensorData sensorData;
SystemState state;

#define HISTORY_SIZE 60
HistoryPoint history[HISTORY_SIZE];
int historyIndex = 0;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void setRelay(int relayIndex, bool on);
void setRelayAll(bool on);
void sendTelegramAlert(String message);
void sendAlertToSupabase(String alertType, String severity, String message);
void supabaseSendDefrostStart(float tempAtStart, const char* triggeredBy);
void supabaseSendDefrostEnd(float tempAtEnd, unsigned long durationMin);
void acknowledgeAlert();
void clearAlert();
void triggerAlert(String message, bool critical);
void saveConfig();
void loadConfig();
bool testTelegram();
void resetWiFi();
String getEmbeddedHTML();

// ============================================================================
// INCLUIR MÓDULOS
// ============================================================================
#include "state_machine.h"
#include "html_ui.h"
#include "storage.h"
#include "telegram.h"
#include "supabase.h"
#include "sensors.h"
#include "alerts.h"
#include "wifi_utils.h"
#include "web_api.h"

// ============================================================================
// CONTROL DE RELÉS
// ============================================================================
void setRelay(int relayIndex, bool on) {
    if (relayIndex < 0 || relayIndex >= MAX_RELAYS) return;
    if (!config.relayOutputEnabled[relayIndex]) return;
    
    sensorData.relay[relayIndex].state = on;
    
    uint8_t pin;
    switch (relayIndex) {
        case 0: pin = PIN_RELAY_1; break;
        case 1: pin = PIN_RELAY_2; break;
        default: return;
    }
    
    digitalWrite(pin, on ? RELAY_ON : RELAY_OFF);
    Serial.printf("[RELAY] %s: %s\n", sensorData.relay[relayIndex].name, on ? "ON" : "OFF");
}

void setRelayAll(bool on) {
    for (int i = 0; i < MAX_RELAYS; i++) {
        setRelay(i, on);
    }
}

// Alias para compatibilidad con código existente
void setRelay(bool on) {
    setRelay(0, on);  // Relé principal (sirena)
}

// ============================================================================
// INICIALIZACIÓN DE PINES
// ============================================================================
void initPins() {
    Serial.println("[INIT] Configurando pines...");
    
    // Relés
    pinMode(PIN_RELAY_1, OUTPUT);
    pinMode(PIN_RELAY_2, OUTPUT);
    digitalWrite(PIN_RELAY_1, RELAY_OFF);
    digitalWrite(PIN_RELAY_2, RELAY_OFF);
    
    // Inicializar estructuras de relés
    sensorData.relay[0].pin = PIN_RELAY_1;
    sensorData.relay[0].name = RELAY_1_NAME;
    sensorData.relay[0].enabled = config.relayOutputEnabled[0];
    sensorData.relay[0].state = false;
    
    sensorData.relay[1].pin = PIN_RELAY_2;
    sensorData.relay[1].name = RELAY_2_NAME;
    sensorData.relay[1].enabled = config.relayOutputEnabled[1];
    sensorData.relay[1].state = false;
    
    // Buzzer
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
    
    // LEDs
    pinMode(PIN_LED_STATUS, OUTPUT);
    pinMode(PIN_LED_ALERT, OUTPUT);
    digitalWrite(PIN_LED_STATUS, LOW);
    digitalWrite(PIN_LED_ALERT, LOW);
    
    // Entrada de defrost
    pinMode(PIN_DEFROST_INPUT, INPUT_PULLUP);
    
    // Botón reset WiFi
    pinMode(PIN_WIFI_RESET, INPUT_PULLUP);
    
    Serial.println("[INIT] ✓ Pines configurados");
    Serial.printf("[INIT] Pin defrost: GPIO%d (modo %s)\n", 
                  PIN_DEFROST_INPUT, 
                  config.defrostRelayNC ? "NC" : "NA");
}

// ============================================================================
// HISTORIAL DE TEMPERATURAS
// ============================================================================
void updateHistory() {
    static unsigned long lastHistoryUpdate = 0;
    
    if (millis() - lastHistoryUpdate >= INTERVAL_HISTORY_UPDATE_MS) {
        lastHistoryUpdate = millis();
        
        history[historyIndex].temp = sensorData.tempAvg;
        history[historyIndex].humidity = sensorData.humidity;
        history[historyIndex].doorOpen = sensorData.anyDoorOpen;
        history[historyIndex].alertActive = state.alertActive;
        history[historyIndex].timestamp = millis();
        
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    }
}

// ============================================================================
// IMPRIMIR ESTADO COMPLETO POR SERIAL (JSON)
// ============================================================================
void printStatusJSON() {
    StaticJsonDocument<JSON_BUFFER_SIZE> doc;
    
    // Identificación del dispositivo
    doc["device_id"] = DEVICE_ID;
    doc["device_name"] = DEVICE_NAME;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["ip"] = state.localIP;
    doc["mdns"] = String(MDNS_NAME) + ".local";
    
    // Estado del sistema (máquina de estados)
    JsonObject stateObj = doc.createNestedObject("state");
    getStateJSON(stateObj);
    
    // Sensores
    JsonObject sensorsObj = doc.createNestedObject("sensors");
    getSensorsJSON(sensorsObj);
    
    // Umbrales configurados
    JsonObject thresholds = doc.createNestedObject("thresholds");
    thresholds["temp_max"] = config.tempMax;
    thresholds["temp_critical"] = config.tempCritical;
    thresholds["alert_delay_sec"] = config.alertDelaySec;
    thresholds["door_open_max_sec"] = config.doorOpenMaxSec;
    thresholds["defrost_cooldown_sec"] = config.defrostCooldownSec;
    
    // Sistema
    JsonObject system = doc.createNestedObject("system");
    system["uptime_sec"] = (millis() - state.bootTime) / 1000;
    system["free_heap"] = ESP.getFreeHeap();
    system["total_alerts"] = state.totalAlerts;
    system["simulation_mode"] = config.simulationMode;
    
    // Conectividad
    JsonObject network = doc.createNestedObject("network");
    network["wifi_connected"] = state.wifiConnected;
    network["wifi_rssi"] = WiFi.RSSI();
    network["internet_available"] = state.internetAvailable;
    network["supabase_enabled"] = config.supabaseEnabled;
    network["last_supabase_sync_sec"] = (millis() - state.lastSupabaseSync) / 1000;
    
    // Imprimir JSON
    Serial.println("\n===== STATUS JSON =====");
    serializeJsonPretty(doc, Serial);
    Serial.println("\n=======================\n");
}

// ============================================================================
// LED DE ESTADO
// ============================================================================
void updateStatusLED() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    
    unsigned long blinkInterval;
    
    switch (state.currentState) {
        case STATE_NORMAL:
            blinkInterval = 2000;  // Parpadeo lento = todo OK
            break;
        case STATE_DEFROST:
        case STATE_COOLDOWN:
            blinkInterval = 500;   // Parpadeo medio = en proceso
            break;
        case STATE_ALERT:
            blinkInterval = 100;   // Parpadeo rápido = alerta
            break;
        case STATE_LOADING_CONFIG:
            blinkInterval = 250;   // Parpadeo rápido = cargando
            break;
        default:
            blinkInterval = 1000;
    }
    
    if (millis() - lastBlink >= blinkInterval) {
        lastBlink = millis();
        ledState = !ledState;
        digitalWrite(PIN_LED_STATUS, ledState);
    }
    
    // LED de alerta
    digitalWrite(PIN_LED_ALERT, state.alertActive ? HIGH : LOW);
}

// ============================================================================
// VERIFICAR BOTÓN DE RESET WIFI
// ============================================================================
void checkWiFiResetButton() {
    static unsigned long buttonPressStart = 0;
    
    if (digitalRead(PIN_WIFI_RESET) == LOW) {
        if (buttonPressStart == 0) {
            buttonPressStart = millis();
        } else if (millis() - buttonPressStart >= WIFI_RESET_HOLD_SEC * 1000) {
            Serial.println("[WIFI] ⚠️ Reset WiFi solicitado!");
            resetWiFi();
            buttonPressStart = 0;
        }
    } else {
        buttonPressStart = 0;
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n");
    Serial.println("╔════════════════════════════════════════════════════╗");
    Serial.println("║   REEFER MONITOR v4.0 - INDUSTRIAL                 ║");
    Serial.println("║   Campamento Parametican Silver                    ║");
    Serial.println("║   Pan American Silver × Pandemonium Tech           ║");
    Serial.println("╚════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.printf("Device ID: %s\n", DEVICE_ID);
    Serial.printf("Firmware:  %s\n", FIRMWARE_VERSION);
    Serial.println();
    
    // Inicializar estado
    state.bootTime = millis();
    state.alertActive = false;
    state.alertCritical = false;
    state.alertAcknowledged = false;
    state.lastSupabaseSync = 0;
    state.totalAlerts = 0;
    
    // Inicializar máquina de estados
    initStateMachine();
    
    // Cargar configuración desde flash
    loadConfig();
    
    // Forzar Supabase habilitado
    if (!config.supabaseEnabled) {
        config.supabaseEnabled = true;
        saveConfig();
        Serial.println("[CONFIG] Supabase habilitado automáticamente");
    }
    
    // Inicializar pines
    initPins();
    
    // Conectar WiFi
    connectWiFi();
    
    // Inicializar sensores
    Serial.println("\n[SENSORES] Inicializando...");
    initSensors();
    
    // Configurar mDNS
    setupMDNS();
    
    // Configurar servidor web
    setupWebServer();
    
    // Actualizar estado online en Supabase
    if (config.supabaseEnabled && state.internetAvailable) {
        supabaseUpdateDeviceStatus(true);
    }
    
    // Pulso de relé al conectar (confirmación audible)
    #ifdef RELAY_PULSE_ON_CONNECT
    if (RELAY_PULSE_ON_CONNECT && state.wifiConnected) {
        setRelay(0, true);
        delay(RELAY_PULSE_DURATION_MS);
        setRelay(0, false);
    }
    #endif
    
    // Cambiar a estado NORMAL
    changeState(STATE_NORMAL, "Inicialización completa");
    
    Serial.println("\n[SISTEMA] ════════════════════════════════════════");
    Serial.println("[SISTEMA] ✓ INICIALIZACIÓN COMPLETA");
    Serial.printf("[SISTEMA] IP: %s\n", state.localIP.c_str());
    Serial.printf("[SISTEMA] mDNS: http://%s.local\n", MDNS_NAME);
    Serial.printf("[SISTEMA] Device ID: %s\n", DEVICE_ID);
    Serial.printf("[SISTEMA] Supabase: %s\n", config.supabaseEnabled ? "HABILITADO" : "DESHABILITADO");
    Serial.println("[SISTEMA] ════════════════════════════════════════\n");
}

// ============================================================================
// LOOP PRINCIPAL (100% NO BLOQUEANTE)
// ============================================================================
void loop() {
    // Manejar peticiones web
    server.handleClient();
    
    // Máquina de estados (verifica defrost, cooldown, config)
    stateMachineLoop();
    
    // Leer sensores (no bloqueante)
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead >= INTERVAL_SENSOR_READ_MS) {
        lastSensorRead = millis();
        readSensors();
    }
    
    // Verificar alertas (solo si estamos monitoreando)
    static unsigned long lastAlertCheck = 0;
    if (millis() - lastAlertCheck >= INTERVAL_ALERT_CHECK_MS) {
        lastAlertCheck = millis();
        if (isStateMonitoring(state.currentState)) {
            checkAlerts();
        }
    }
    
    // Imprimir estado JSON por serial
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint >= INTERVAL_STATUS_PRINT_MS) {
        lastStatusPrint = millis();
        printStatusJSON();
    }
    
    // Verificar conexión a internet
    checkInternet();
    
    // Actualizar historial
    updateHistory();
    
    // Sincronizar con Supabase
    supabaseSync();
    
    // Actualizar LED de estado
    updateStatusLED();
    
    // Verificar botón de reset WiFi
    checkWiFiResetButton();
    
    // Pequeña pausa para estabilidad
    delay(10);
}
