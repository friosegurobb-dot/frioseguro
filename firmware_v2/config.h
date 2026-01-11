/*
 * ============================================================================
 * CONFIG.H - CONFIGURACIÓN CENTRALIZADA FIRMWARE v4.0
 * Sistema Monitoreo Reefer Industrial
 * Campamento Parametican Silver - Pan American Silver
 * ============================================================================
 * 
 * ARQUITECTURA ESP32-WROOM-32 (38 pines)
 * 
 * Este archivo centraliza TODA la configuración del dispositivo:
 * - Device ID (único por reefer)
 * - Asignación de pines
 * - Sensores habilitados
 * - Umbrales de alerta
 * - Tiempos del sistema
 * 
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// SECCIÓN 1: IDENTIFICACIÓN DEL DISPOSITIVO
// ============================================================================
// IMPORTANTE: Cambiar para cada dispositivo físico!
//
// Formato: REEFER_[NUM]_[UBICACION]
//   - Producción Santa Cruz: REEFER_01_SCZ ... REEFER_06_SCZ
//   - Desarrollo Bahía Blanca: REEFER_DEV_BHI
//
// Este ID viaja en TODOS los payloads (Supabase, Telegram, Serial, API)

#define DEVICE_ID           "REEFER_DEV_BHI"
#define DEVICE_NAME         "Reefer Desarrollo"
#define DEVICE_LOCATION     "Bahía Blanca, Buenos Aires"
#define FIRMWARE_VERSION    "4.0.0"

// ============================================================================
// SECCIÓN 2: CREDENCIALES Y SERVICIOS
// ============================================================================

// WiFiManager - Portal de configuración
#define AP_NAME             "Reefer-Setup"
#define AP_PASSWORD         "reefer1234"
#define AP_TIMEOUT          180

// mDNS - Acceso por nombre
#define MDNS_NAME           "reefer"

// Telegram
#define TELEGRAM_BOT_TOKEN  "8175168657:AAE5HJBnp4Hx6LOECBh7Ps3utw35WMRdGnI"
#define TELEGRAM_CHAT_ID    "7713503644"

// Supabase
#define SUPABASE_URL        "https://xhdeacnwdzvkivfjzard.supabase.co"
#define SUPABASE_ANON_KEY   "sb_publishable_JhTUv1X2LHMBVILUaysJ3g_Ho11zu-Q"

// ============================================================================
// SECCIÓN 3: MAPA DE PINES ESP32-WROOM-32 (38 pines)
// ============================================================================
//
// ┌─────────────────────────────────────────────────────────────────────────┐
// │                    ESP32-WROOM-32 PINOUT                                │
// ├─────────────────────────────────────────────────────────────────────────┤
// │                                                                         │
// │  [EN]  [VP/36] [VN/39] [34]  [35]  [32]  [33]  [25]  [26]  [27]  [14]  │
// │   │      │       │      │     │     │     │     │     │     │     │    │
// │   │      │       │      │     │     │     │     │     │     │     │    │
// │   │    ADC1    ADC1   ADC1  ADC1  ADC1  ADC1   DAC   DAC  Touch Touch  │
// │   │    IN      IN     IN    IN    IN    IN                             │
// │                                                                         │
// │  [3V3] [GND] [15]  [2]   [4]   [16]  [17]  [5]   [18]  [19]  [GND]    │
// │                │     │     │     │     │     │     │     │             │
// │              Touch  LED  1Wire  RX2   TX2  SPI   SPI   SPI            │
// │                                                                         │
// │  [VIN] [GND] [13]  [12]  [14]  [27]  [26]  [25]  [33]  [32]  [GND]    │
// │                │     │                                                  │
// │              Touch  Touch                                               │
// │                                                                         │
// │  [21]  [22]  [23]  [GND]                                               │
// │    │     │     │                                                        │
// │   I2C   I2C   SPI                                                       │
// │   SDA   SCL   MOSI                                                      │
// │                                                                         │
// └─────────────────────────────────────────────────────────────────────────┘
//
// PINES NO USAR (reservados/problemáticos):
// - GPIO0: Boot (usar solo con cuidado)
// - GPIO1: TX0 (Serial)
// - GPIO3: RX0 (Serial)
// - GPIO6-11: Flash SPI (NO USAR)
// - GPIO12: Boot mode (cuidado al inicio)
//
// ============================================================================

// ----------------------------------------------------------------------------
// 3.1 BUS ONEWIRE - SONDAS DE TEMPERATURA DS18B20
// ----------------------------------------------------------------------------
// Todas las sondas DS18B20 van en el mismo bus (protocolo 1-Wire)
// Máximo recomendado: 8 sensores por bus (con resistencia 4.7K pullup)

#define PIN_ONEWIRE         4       // GPIO4 - Bus 1-Wire para DS18B20

// Cantidad máxima de sondas soportadas
#define MAX_TEMP_SENSORS    6

// Configuración individual de sondas (habilitar según hardware conectado)
#define TEMP_SENSOR_1_ENABLED   true    // Sonda principal
#define TEMP_SENSOR_2_ENABLED   false   // Sonda secundaria
#define TEMP_SENSOR_3_ENABLED   false   // Sonda adicional
#define TEMP_SENSOR_4_ENABLED   false   // Sonda adicional
#define TEMP_SENSOR_5_ENABLED   false   // Sonda adicional
#define TEMP_SENSOR_6_ENABLED   false   // Sonda adicional

// Nombres descriptivos de las sondas (para UI y reportes)
#define TEMP_SENSOR_1_NAME  "Interior Principal"
#define TEMP_SENSOR_2_NAME  "Interior Secundario"
#define TEMP_SENSOR_3_NAME  "Evaporador"
#define TEMP_SENSOR_4_NAME  "Condensador"
#define TEMP_SENSOR_5_NAME  "Ambiente"
#define TEMP_SENSOR_6_NAME  "Reserva"

// ----------------------------------------------------------------------------
// 3.2 SENSOR DHT22 - TEMPERATURA AMBIENTE Y HUMEDAD
// ----------------------------------------------------------------------------
#define PIN_DHT22           18      // GPIO18 - DHT22
#define DHT22_ENABLED       false   // Habilitar sensor DHT22

// ----------------------------------------------------------------------------
// 3.3 SENSORES DE PUERTA (Reed Switch / Magnéticos)
// ----------------------------------------------------------------------------
// Cada puerta tiene su propio pin GPIO con pull-up interno
// HIGH = Puerta abierta, LOW = Puerta cerrada (con pull-up)

#define MAX_DOOR_SENSORS    3

#define PIN_DOOR_1          5       // GPIO5 - Puerta principal
#define PIN_DOOR_2          17      // GPIO17 - Puerta lateral
#define PIN_DOOR_3          16      // GPIO16 - Puerta trasera

#define DOOR_1_ENABLED      false   // Habilitar puerta 1
#define DOOR_2_ENABLED      false   // Habilitar puerta 2
#define DOOR_3_ENABLED      false   // Habilitar puerta 3

#define DOOR_1_NAME         "Principal"
#define DOOR_2_NAME         "Lateral"
#define DOOR_3_NAME         "Trasera"

// Alias para compatibilidad con código existente
#define PIN_DOOR            PIN_DOOR_1
#define PIN_DOOR_SENSOR     PIN_DOOR_1

// ----------------------------------------------------------------------------
// 3.4 SALIDAS DE RELÉ
// ----------------------------------------------------------------------------
// Lógica invertida: LOW = ON, HIGH = OFF (común en módulos de relé)

#define MAX_RELAYS          2

#define PIN_RELAY_1         26      // GPIO26 - Relé sirena/alarma
#define PIN_RELAY_2         27      // GPIO27 - Relé auxiliar (luz, ventilador, etc.)

#define RELAY_1_ENABLED     true    // Habilitar relé 1 (sirena)
#define RELAY_2_ENABLED     false   // Habilitar relé 2 (auxiliar)

#define RELAY_1_NAME        "Sirena"
#define RELAY_2_NAME        "Auxiliar"

// Alias para compatibilidad
#define PIN_RELAY           PIN_RELAY_1

// Lógica de relé
#define RELAY_ON            LOW
#define RELAY_OFF           HIGH

// Pulso de relé al conectar WiFi (confirmación audible)
#define RELAY_PULSE_ON_CONNECT      true
#define RELAY_PULSE_DURATION_MS     1000

// ----------------------------------------------------------------------------
// 3.5 ENTRADA DE SEÑAL DEFROST (del sistema de frío)
// ----------------------------------------------------------------------------
// Detecta cuando el reefer entra en ciclo de descongelamiento
// Puede ser NC (Normalmente Cerrado) o NA (Normalmente Abierto)

#define PIN_DEFROST_INPUT   33      // GPIO33 - Señal defrost del reefer

// Modo del contacto:
// false = NA (Normalmente Abierto): HIGH = normal, LOW = defrost
// true = NC (Normalmente Cerrado): LOW = normal, HIGH = defrost
#define DEFROST_PIN_NC      false

// ----------------------------------------------------------------------------
// 3.6 INDICADORES LED Y BUZZER
// ----------------------------------------------------------------------------
#define PIN_LED_STATUS      2       // GPIO2 - LED integrado (estado general)
#define PIN_LED_ALERT       15      // GPIO15 - LED rojo (alerta)
#define PIN_BUZZER          19      // GPIO19 - Buzzer

#define BUZZER_ENABLED      true

// ----------------------------------------------------------------------------
// 3.7 BOTÓN DE RESET WIFI
// ----------------------------------------------------------------------------
#define PIN_WIFI_RESET      0       // GPIO0 - Botón BOOT del ESP32
#define WIFI_RESET_HOLD_SEC 5       // Segundos para resetear WiFi

// ----------------------------------------------------------------------------
// 3.8 PINES LIBRES DISPONIBLES
// ----------------------------------------------------------------------------
// Los siguientes pines están libres para expansión futura:
//
// GPIO12 - Touch/ADC (cuidado: afecta boot si está HIGH)
// GPIO13 - Touch/ADC
// GPIO14 - Touch/ADC
// GPIO21 - I2C SDA (para sensores I2C futuros)
// GPIO22 - I2C SCL (para sensores I2C futuros)
// GPIO23 - SPI MOSI
// GPIO25 - DAC1
// GPIO32 - ADC1 (para sensor de corriente)
// GPIO34 - ADC1 (solo entrada, sin pull-up)
// GPIO35 - ADC1 (solo entrada, sin pull-up)
// GPIO36 (VP) - ADC1 (solo entrada)
// GPIO39 (VN) - ADC1 (solo entrada)
//
// Total pines libres: 12
// Pines ADC disponibles: 6 (para sensores analógicos)
// Pines I2C disponibles: 2 (para expansión con sensores I2C)

#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         22
#define PIN_CURRENT_SENSOR  32      // Para sensor de corriente ACS712

// ============================================================================
// SECCIÓN 4: ESTADOS DEL SISTEMA
// ============================================================================
// El sistema opera en uno de estos estados mutuamente excluyentes

typedef enum {
    STATE_NORMAL = 0,               // Operación normal, monitoreando
    STATE_DEFROST,                  // En ciclo de descongelamiento
    STATE_COOLDOWN,                 // Esperando post-descongelamiento
    STATE_LOADING_CONFIG,           // Aplicando nueva configuración
    STATE_ALERT,                    // Alerta activa
    STATE_OFFLINE,                  // Sin conexión WiFi
    STATE_INITIALIZING              // Arrancando sistema
} SystemStateEnum;

// Nombres de estados para reportes
#define STATE_NAME_NORMAL           "NORMAL"
#define STATE_NAME_DEFROST          "EN_DESCONGELAMIENTO"
#define STATE_NAME_COOLDOWN         "ESPERA_POST_DESCONGELADO"
#define STATE_NAME_LOADING_CONFIG   "CARGANDO_CONFIG"
#define STATE_NAME_ALERT            "ALERTA"
#define STATE_NAME_OFFLINE          "OFFLINE"
#define STATE_NAME_INITIALIZING     "INICIALIZANDO"

// ============================================================================
// SECCIÓN 5: UMBRALES Y TIEMPOS POR DEFECTO
// ============================================================================
// Estos valores se pueden modificar desde la web/app y se guardan en flash

// Temperatura
#define DEFAULT_TEMP_MIN            -40.0   // Mínimo esperado
#define DEFAULT_TEMP_MAX            -18.0   // Umbral warning
#define DEFAULT_TEMP_CRITICAL       -10.0   // Umbral crítico (activa alarma)

// Tiempos de alerta (en segundos)
#define DEFAULT_ALERT_DELAY_SEC     300     // 5 min antes de alertar
#define DEFAULT_DOOR_OPEN_MAX_SEC   180     // 3 min máximo puerta abierta

// Tiempos de descongelamiento (en segundos)
#define DEFAULT_DEFROST_COOLDOWN_SEC    1800    // 30 min post-defrost
#define DEFAULT_DEFROST_MAX_DURATION_SEC 3600   // 60 min máximo defrost

// Tiempo de aplicación de configuración (en segundos)
#define CONFIG_APPLY_TIME_SEC       10      // 10 seg para aplicar config

// ============================================================================
// SECCIÓN 6: INTERVALOS DE OPERACIÓN (no bloqueantes, en ms)
// ============================================================================

#define INTERVAL_SENSOR_READ_MS     2000    // Leer sensores cada 2 seg
#define INTERVAL_STATUS_PRINT_MS    5000    // Imprimir status cada 5 seg
#define INTERVAL_DEFROST_CHECK_MS   500     // Verificar defrost cada 500ms
#define INTERVAL_ALERT_CHECK_MS     1000    // Verificar alertas cada 1 seg
#define INTERVAL_INTERNET_CHECK_MS  30000   // Verificar internet cada 30 seg
#define INTERVAL_SUPABASE_SYNC_MS   5000    // Sincronizar Supabase cada 5 seg
#define INTERVAL_HISTORY_UPDATE_MS  60000   // Actualizar historial cada 1 min
#define INTERVAL_DEVICE_STATUS_MS   60000   // Actualizar estado dispositivo cada 1 min

// ============================================================================
// SECCIÓN 7: LÍMITES DEL SISTEMA
// ============================================================================

#define HISTORY_SIZE                60      // Puntos de historial (1 hora a 1/min)
#define MAX_ALERTS_QUEUE            10      // Cola de alertas pendientes
#define JSON_BUFFER_SIZE            2048    // Buffer para JSON
#define MAX_WIFI_RETRIES            3       // Reintentos de conexión WiFi

// ============================================================================
// SECCIÓN 8: CONFIGURACIÓN DE SIMULACIÓN (para testing)
// ============================================================================

#define SIMULATION_MODE             false   // Modo simulación
#define SIM_TEMP_DEFAULT            -20.0   // Temperatura simulada
#define SIM_DOOR_OPEN               false   // Puerta simulada

// ============================================================================
// SECCIÓN 9: UBICACIÓN GPS (opcional)
// ============================================================================

#define LOCATION_LAT                -38.7196  // Bahía Blanca
#define LOCATION_LON                -62.2724

// ============================================================================
// RESUMEN DE CAPACIDAD DEL SISTEMA
// ============================================================================
//
// SENSORES SOPORTADOS:
// - Sondas de temperatura DS18B20: hasta 6 (en bus 1-Wire)
// - Sensores de puerta: hasta 3
// - DHT22 (ambiente): 1
// - Sensor de corriente: 1 (futuro)
//
// SALIDAS:
// - Relés: 2
// - LEDs: 2
// - Buzzer: 1
//
// ENTRADAS ESPECIALES:
// - Señal defrost: 1
// - Botón reset WiFi: 1
//
// PINES LIBRES: 12
// PINES ADC LIBRES: 6
//
// LÍMITE REALISTA POR REEFER:
// - 6 sondas de temperatura (más que suficiente)
// - 3 puertas (principal, lateral, trasera)
// - 2 relés (sirena + auxiliar)
// - 1 sensor ambiente (DHT22)
// - 1 sensor de corriente (compresor)
//
// ============================================================================

#endif // CONFIG_H
