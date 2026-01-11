/*
 * ============================================================================
 * STORAGE.H - ALMACENAMIENTO EN FLASH v4.0
 * Sistema Monitoreo Reefer Industrial
 * ============================================================================
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include "config.h"
#include "types.h"

extern Preferences prefs;
extern Config config;
extern SystemState state;

// Forward declaration
extern void enterLoadingConfigMode();

// ============================================================================
// CARGAR CONFIGURACIÓN DESDE FLASH
// ============================================================================
void loadConfig() {
    Serial.println("[STORAGE] Cargando configuración...");
    
    prefs.begin("reefer", true);  // Solo lectura
    
    // Umbrales de temperatura
    config.tempMax = prefs.getFloat("tempMax", DEFAULT_TEMP_MAX);
    config.tempCritical = prefs.getFloat("tempCrit", DEFAULT_TEMP_CRITICAL);
    
    // Tiempos
    config.alertDelaySec = prefs.getInt("alertDelay", DEFAULT_ALERT_DELAY_SEC);
    config.doorOpenMaxSec = prefs.getInt("doorMax", DEFAULT_DOOR_OPEN_MAX_SEC);
    config.defrostCooldownSec = prefs.getInt("defrostCool", DEFAULT_DEFROST_COOLDOWN_SEC);
    config.defrostMaxDurationSec = prefs.getInt("defrostMax", DEFAULT_DEFROST_MAX_DURATION_SEC);
    config.configApplyTimeSec = prefs.getInt("configTime", CONFIG_APPLY_TIME_SEC);
    
    // Configuración de defrost
    config.defrostRelayNC = prefs.getBool("defrostNC", DEFROST_PIN_NC);
    
    // Funcionalidades
    config.relayEnabled = prefs.getBool("relayEn", RELAY_1_ENABLED);
    config.buzzerEnabled = prefs.getBool("buzzerEn", BUZZER_ENABLED);
    config.telegramEnabled = prefs.getBool("telegramEn", true);
    config.supabaseEnabled = prefs.getBool("supabaseEn", true);
    config.dht22Enabled = prefs.getBool("dht22En", DHT22_ENABLED);
    
    // Sensores de temperatura habilitados
    config.tempSensorEnabled[0] = prefs.getBool("temp1En", TEMP_SENSOR_1_ENABLED);
    config.tempSensorEnabled[1] = prefs.getBool("temp2En", TEMP_SENSOR_2_ENABLED);
    config.tempSensorEnabled[2] = prefs.getBool("temp3En", TEMP_SENSOR_3_ENABLED);
    config.tempSensorEnabled[3] = prefs.getBool("temp4En", TEMP_SENSOR_4_ENABLED);
    config.tempSensorEnabled[4] = prefs.getBool("temp5En", TEMP_SENSOR_5_ENABLED);
    config.tempSensorEnabled[5] = prefs.getBool("temp6En", TEMP_SENSOR_6_ENABLED);
    
    // Puertas habilitadas
    config.doorEnabled[0] = prefs.getBool("door1En", DOOR_1_ENABLED);
    config.doorEnabled[1] = prefs.getBool("door2En", DOOR_2_ENABLED);
    config.doorEnabled[2] = prefs.getBool("door3En", DOOR_3_ENABLED);
    
    // Relés habilitados
    config.relayOutputEnabled[0] = prefs.getBool("relay1En", RELAY_1_ENABLED);
    config.relayOutputEnabled[1] = prefs.getBool("relay2En", RELAY_2_ENABLED);
    
    // Modo simulación
    config.simulationMode = prefs.getBool("simMode", SIMULATION_MODE);
    config.simTemp = prefs.getFloat("simTemp", SIM_TEMP_DEFAULT);
    config.simDoorOpen = prefs.getBool("simDoor", SIM_DOOR_OPEN);
    
    prefs.end();
    
    Serial.println("[STORAGE] ✓ Configuración cargada");
    Serial.printf("[STORAGE] Temp crítica: %.1f°C\n", config.tempCritical);
    Serial.printf("[STORAGE] Delay alerta: %d seg\n", config.alertDelaySec);
    Serial.printf("[STORAGE] Cooldown defrost: %d seg\n", config.defrostCooldownSec);
}

// ============================================================================
// GUARDAR CONFIGURACIÓN EN FLASH
// ============================================================================
void saveConfig() {
    Serial.println("[STORAGE] Guardando configuración...");
    
    // Entrar en modo LOADING_CONFIG
    enterLoadingConfigMode();
    
    prefs.begin("reefer", false);  // Lectura/escritura
    
    // Umbrales de temperatura
    prefs.putFloat("tempMax", config.tempMax);
    prefs.putFloat("tempCrit", config.tempCritical);
    
    // Tiempos
    prefs.putInt("alertDelay", config.alertDelaySec);
    prefs.putInt("doorMax", config.doorOpenMaxSec);
    prefs.putInt("defrostCool", config.defrostCooldownSec);
    prefs.putInt("defrostMax", config.defrostMaxDurationSec);
    prefs.putInt("configTime", config.configApplyTimeSec);
    
    // Configuración de defrost
    prefs.putBool("defrostNC", config.defrostRelayNC);
    
    // Funcionalidades
    prefs.putBool("relayEn", config.relayEnabled);
    prefs.putBool("buzzerEn", config.buzzerEnabled);
    prefs.putBool("telegramEn", config.telegramEnabled);
    prefs.putBool("supabaseEn", config.supabaseEnabled);
    prefs.putBool("dht22En", config.dht22Enabled);
    
    // Sensores de temperatura
    prefs.putBool("temp1En", config.tempSensorEnabled[0]);
    prefs.putBool("temp2En", config.tempSensorEnabled[1]);
    prefs.putBool("temp3En", config.tempSensorEnabled[2]);
    prefs.putBool("temp4En", config.tempSensorEnabled[3]);
    prefs.putBool("temp5En", config.tempSensorEnabled[4]);
    prefs.putBool("temp6En", config.tempSensorEnabled[5]);
    
    // Puertas
    prefs.putBool("door1En", config.doorEnabled[0]);
    prefs.putBool("door2En", config.doorEnabled[1]);
    prefs.putBool("door3En", config.doorEnabled[2]);
    
    // Relés
    prefs.putBool("relay1En", config.relayOutputEnabled[0]);
    prefs.putBool("relay2En", config.relayOutputEnabled[1]);
    
    // Modo simulación
    prefs.putBool("simMode", config.simulationMode);
    prefs.putFloat("simTemp", config.simTemp);
    prefs.putBool("simDoor", config.simDoorOpen);
    
    prefs.end();
    
    Serial.println("[STORAGE] ✓ Configuración guardada");
}

// ============================================================================
// RESETEAR CONFIGURACIÓN A VALORES POR DEFECTO
// ============================================================================
void resetConfig() {
    Serial.println("[STORAGE] Reseteando configuración...");
    
    prefs.begin("reefer", false);
    prefs.clear();
    prefs.end();
    
    loadConfig();  // Cargará los valores por defecto
    
    Serial.println("[STORAGE] ✓ Configuración reseteada");
}

// ============================================================================
// OBTENER JSON DE CONFIGURACIÓN
// ============================================================================
void getConfigJSON(JsonObject& obj) {
    obj["temp_max"] = config.tempMax;
    obj["temp_critical"] = config.tempCritical;
    obj["alert_delay_sec"] = config.alertDelaySec;
    obj["door_open_max_sec"] = config.doorOpenMaxSec;
    obj["defrost_cooldown_sec"] = config.defrostCooldownSec;
    obj["defrost_max_duration_sec"] = config.defrostMaxDurationSec;
    obj["defrost_relay_nc"] = config.defrostRelayNC;
    
    obj["relay_enabled"] = config.relayEnabled;
    obj["buzzer_enabled"] = config.buzzerEnabled;
    obj["telegram_enabled"] = config.telegramEnabled;
    obj["supabase_enabled"] = config.supabaseEnabled;
    obj["dht22_enabled"] = config.dht22Enabled;
    
    obj["simulation_mode"] = config.simulationMode;
    obj["sim_temp"] = config.simTemp;
    obj["sim_door_open"] = config.simDoorOpen;
    
    // Arrays de sensores habilitados
    JsonArray tempSensors = obj.createNestedArray("temp_sensors_enabled");
    for (int i = 0; i < MAX_TEMP_SENSORS; i++) {
        tempSensors.add(config.tempSensorEnabled[i]);
    }
    
    JsonArray doors = obj.createNestedArray("doors_enabled");
    for (int i = 0; i < MAX_DOOR_SENSORS; i++) {
        doors.add(config.doorEnabled[i]);
    }
    
    JsonArray relays = obj.createNestedArray("relays_enabled");
    for (int i = 0; i < MAX_RELAYS; i++) {
        relays.add(config.relayOutputEnabled[i]);
    }
}

#endif // STORAGE_H
