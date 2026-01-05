/*
 * CONFIGURACIÓN - Sistema FrioSeguro para Carnicerías
 * Firmware para monitoreo de temperatura en locales comerciales
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// WIFIMANAGER - Configuración del Access Point
// ============================================
#define AP_NAME "FrioSeguro-Setup"
#define AP_PASSWORD "frio1234"
#define AP_TIMEOUT 180

// ============================================
// mDNS
// ============================================
#define MDNS_NAME "reefer"  // http://reefer.local

// ============================================
// DEVICE ID - Cambiar para cada local
// ============================================
#define DEVICE_ID "CARNICERIA-LOCAL1"
#define DEVICE_NAME "Freezer Local Centro"
#define ORG_SLUG "carniceria-demo"  // Slug de la organización en Supabase

// ============================================
// TELEGRAM
// ============================================
#define TELEGRAM_BOT_TOKEN "TU_BOT_TOKEN_AQUI"
const char* TELEGRAM_CHAT_IDS[] = {
    "TU_CHAT_ID_AQUI",
};
const int TELEGRAM_CHAT_COUNT = 1;

// ============================================
// SUPABASE
// ============================================
#define SUPABASE_URL "https://xhdeacnwdzvkivfjzard.supabase.co"
#define SUPABASE_ANON_KEY "sb_publishable_JhTUv1X2LHMBVILUaysJ3g_Ho11zu-Q"
#define SUPABASE_SYNC_INTERVAL_MS 5000

// ============================================
// PINES ESP32
// ============================================
#define PIN_ONEWIRE 4
#define PIN_DHT22 18
#define PIN_DOOR_SENSOR 5
#define PIN_RELAY 16
#define PIN_LED_OK 2
#define PIN_LED_ALERT 15
#define PIN_BUZZER 17
#define PIN_WIFI_RESET 0

// ============================================
// SENSORES HABILITADOS
// ============================================
#define SENSOR_DS18B20_1_ENABLED true
#define SENSOR_DS18B20_2_ENABLED false
#define SENSOR_DHT22_ENABLED false
#define SENSOR_DOOR_ENABLED false

// ============================================
// UMBRALES POR DEFECTO (para carnicería)
// ============================================
#define DEFAULT_TEMP_MIN -25.0
#define DEFAULT_TEMP_MAX -15.0       // Carnes congeladas
#define DEFAULT_TEMP_CRITICAL -8.0
#define DEFAULT_ALERT_DELAY_SEC 180  // 3 minutos
#define DEFAULT_DOOR_OPEN_MAX_SEC 120

// ============================================
// UBICACIÓN
// ============================================
#define LOCATION_NAME "Carnicería Demo"
#define LOCATION_DETAIL "Local Centro"
#define LOCATION_LAT -34.603722
#define LOCATION_LON -58.381592

// ============================================
// CONFIGURACIÓN DEL SISTEMA
// ============================================
#define SENSOR_READ_INTERVAL_MS 5000
#define INTERNET_CHECK_INTERVAL_MS 30000
#define HISTORY_MAX_POINTS 288

// ============================================
// RELAY
// ============================================
#define RELAY_ON LOW
#define RELAY_OFF HIGH

// ============================================
// MODO SIMULACIÓN
// ============================================
#define SIMULATION_MODE false

#endif
