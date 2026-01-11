/*
 * ============================================================================
 * TYPES.H - ESTRUCTURAS DE DATOS v4.0
 * Sistema Monitoreo Reefer Industrial
 * ============================================================================
 */

#ifndef TYPES_H
#define TYPES_H

#include "config.h"

// ============================================================================
// ESTRUCTURA: Datos de una sonda de temperatura
// ============================================================================
struct TempSensor {
    float value;                    // Temperatura actual
    float minToday;                 // Mínima del día
    float maxToday;                 // Máxima del día
    bool valid;                     // Lectura válida
    bool enabled;                   // Sensor habilitado
    const char* name;               // Nombre descriptivo
    uint8_t address[8];             // Dirección 1-Wire (para identificación)
};

// ============================================================================
// ESTRUCTURA: Datos de un sensor de puerta
// ============================================================================
struct DoorSensor {
    bool isOpen;                    // Estado actual
    bool enabled;                   // Sensor habilitado
    const char* name;               // Nombre descriptivo
    uint8_t pin;                    // Pin GPIO
    unsigned long openSince;        // Timestamp de apertura (0 si cerrada)
    unsigned long totalOpenToday;   // Segundos abierta hoy
    int opensToday;                 // Cantidad de aperturas hoy
};

// ============================================================================
// ESTRUCTURA: Datos de un relé
// ============================================================================
struct RelayOutput {
    bool state;                     // Estado actual (true = ON)
    bool enabled;                   // Relé habilitado
    const char* name;               // Nombre descriptivo
    uint8_t pin;                    // Pin GPIO
};

// ============================================================================
// ESTRUCTURA: Configuración del sistema (persistente en flash)
// ============================================================================
struct Config {
    // Umbrales de temperatura
    float tempMax;                  // Umbral warning
    float tempCritical;             // Umbral crítico
    
    // Tiempos (en segundos)
    int alertDelaySec;              // Delay antes de alertar
    int doorOpenMaxSec;             // Máximo tiempo puerta abierta
    int defrostCooldownSec;         // Espera post-descongelamiento
    int defrostMaxDurationSec;      // Duración máxima defrost
    int configApplyTimeSec;         // Tiempo para aplicar config
    
    // Configuración de defrost
    bool defrostRelayNC;            // true = NC, false = NA
    
    // Funcionalidades habilitadas
    bool relayEnabled;
    bool buzzerEnabled;
    bool telegramEnabled;
    bool supabaseEnabled;
    bool dht22Enabled;
    
    // Sensores de temperatura habilitados
    bool tempSensorEnabled[MAX_TEMP_SENSORS];
    
    // Puertas habilitadas
    bool doorEnabled[MAX_DOOR_SENSORS];
    
    // Relés habilitados
    bool relayOutputEnabled[MAX_RELAYS];
    
    // Modo simulación
    bool simulationMode;
    float simTemp;
    bool simDoorOpen;
    
    // Checksum para validar configuración
    uint32_t checksum;
};

// ============================================================================
// ESTRUCTURA: Datos de todos los sensores
// ============================================================================
struct SensorData {
    // Sondas de temperatura
    TempSensor temp[MAX_TEMP_SENSORS];
    int tempSensorCount;            // Cantidad detectada
    float tempAvg;                  // Promedio de todas las sondas
    float tempMin;                  // Mínima de todas las sondas
    float tempMax;                  // Máxima de todas las sondas
    bool tempValid;                 // Al menos una lectura válida
    
    // DHT22
    float humidity;
    float tempAmbient;
    bool dhtValid;
    
    // Puertas
    DoorSensor door[MAX_DOOR_SENSORS];
    bool anyDoorOpen;               // Alguna puerta abierta
    int doorsOpenCount;             // Cantidad de puertas abiertas
    
    // Relés
    RelayOutput relay[MAX_RELAYS];
    
    // Señal de defrost
    bool defrostSignalActive;       // Señal del reefer activa
};

// ============================================================================
// ESTRUCTURA: Estado del sistema
// ============================================================================
struct SystemState {
    // Estado principal (máquina de estados)
    SystemStateEnum currentState;
    SystemStateEnum previousState;
    unsigned long stateChangedAt;   // Timestamp del cambio de estado
    
    // Información del estado actual
    const char* stateName;          // Nombre del estado para reportes
    
    // Timers de estado (no bloqueantes)
    unsigned long defrostStartTime;     // Inicio del defrost
    unsigned long cooldownStartTime;    // Inicio del cooldown
    unsigned long cooldownEndTime;      // Fin esperado del cooldown
    int cooldownRemainingSeconds;       // Segundos restantes de cooldown
    
    unsigned long configLoadStartTime;  // Inicio de carga de config
    unsigned long configLoadEndTime;    // Fin esperado de carga
    int configLoadRemainingSeconds;     // Segundos restantes
    
    // Alertas
    bool alertActive;
    bool alertCritical;
    bool alertAcknowledged;
    String alertMessage;
    unsigned long alertStartTime;
    unsigned long highTempStartTime;
    int totalAlerts;
    
    // Conectividad
    bool wifiConnected;
    bool internetAvailable;
    bool apMode;
    String localIP;
    int wifiRSSI;
    
    // Supabase
    unsigned long lastSupabaseSync;
    bool supabaseSyncOk;
    
    // Telegram
    unsigned long lastTelegramAlert;
    
    // Sistema
    unsigned long bootTime;
    unsigned long uptimeSeconds;
    uint32_t freeHeap;
    
    // Última lectura de sensores
    unsigned long lastSensorRead;
    unsigned long lastInternetCheck;
};

// ============================================================================
// ESTRUCTURA: Punto de historial
// ============================================================================
struct HistoryPoint {
    float temp;
    float humidity;
    bool doorOpen;
    bool alertActive;
    unsigned long timestamp;
};

// ============================================================================
// ESTRUCTURA: Timer no bloqueante
// ============================================================================
struct NonBlockingTimer {
    unsigned long startTime;
    unsigned long duration;
    bool running;
    bool expired;
    
    void start(unsigned long durationMs) {
        startTime = millis();
        duration = durationMs;
        running = true;
        expired = false;
    }
    
    void stop() {
        running = false;
        expired = false;
    }
    
    bool check() {
        if (!running) return false;
        if (millis() - startTime >= duration) {
            expired = true;
            running = false;
            return true;
        }
        return false;
    }
    
    unsigned long remaining() {
        if (!running) return 0;
        unsigned long elapsed = millis() - startTime;
        if (elapsed >= duration) return 0;
        return duration - elapsed;
    }
    
    unsigned long remainingSeconds() {
        return remaining() / 1000;
    }
};

// ============================================================================
// ESTRUCTURA: Cola de eventos/alertas
// ============================================================================
struct AlertEvent {
    String type;                    // "temperature", "door", "power", "offline"
    String severity;                // "info", "warning", "critical", "emergency"
    String message;
    float value;                    // Valor que disparó la alerta
    unsigned long timestamp;
    bool sent;                      // Ya enviada a Supabase/Telegram
};

// ============================================================================
// FUNCIONES HELPER PARA ESTADOS
// ============================================================================

inline const char* getStateName(SystemStateEnum state) {
    switch (state) {
        case STATE_NORMAL:          return STATE_NAME_NORMAL;
        case STATE_DEFROST:         return STATE_NAME_DEFROST;
        case STATE_COOLDOWN:        return STATE_NAME_COOLDOWN;
        case STATE_LOADING_CONFIG:  return STATE_NAME_LOADING_CONFIG;
        case STATE_ALERT:           return STATE_NAME_ALERT;
        case STATE_OFFLINE:         return STATE_NAME_OFFLINE;
        case STATE_INITIALIZING:    return STATE_NAME_INITIALIZING;
        default:                    return "UNKNOWN";
    }
}

inline bool isStateMonitoring(SystemStateEnum state) {
    return state == STATE_NORMAL || state == STATE_ALERT;
}

inline bool isStateSuspended(SystemStateEnum state) {
    return state == STATE_DEFROST || state == STATE_COOLDOWN || state == STATE_LOADING_CONFIG;
}

#endif // TYPES_H
