/*
 * storage.h - Almacenamiento de configuración en Preferences
 * Sistema Monitoreo Reefer v3.0
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include "config.h"
#include "types.h"

extern Preferences prefs;
extern Config config;

// ============================================
// CARGAR CONFIGURACIÓN
// ============================================
void loadConfig() {
  prefs.begin("reefer", false);
  
  config.tempMax = prefs.getFloat("tempMax", DEFAULT_TEMP_MAX);
  config.tempCritical = prefs.getFloat("tempCrit", DEFAULT_TEMP_CRITICAL);
  config.alertDelaySec = prefs.getInt("alertDelay", DEFAULT_ALERT_DELAY_SEC);
  config.doorOpenMaxSec = prefs.getInt("doorMax", DEFAULT_DOOR_OPEN_MAX_SEC);
  config.defrostCooldownSec = prefs.getInt("defrostCD", 1800);
  config.defrostRelayNC = prefs.getBool("defrostNC", false);
  config.relayEnabled = prefs.getBool("relayEn", true);
  config.buzzerEnabled = prefs.getBool("buzzerEn", true);
  config.telegramEnabled = prefs.getBool("telegramEn", true);
  config.supabaseEnabled = prefs.getBool("supabaseEn", true);  // Habilitado por defecto
  config.sensor1Enabled = prefs.getBool("sensor1", SENSOR_DS18B20_1_ENABLED);
  config.sensor2Enabled = prefs.getBool("sensor2", SENSOR_DS18B20_2_ENABLED);
  config.dht22Enabled = prefs.getBool("dht22", SENSOR_DHT22_ENABLED);
  config.doorEnabled = prefs.getBool("doorEn", SENSOR_DOOR_ENABLED);
  config.simulationMode = prefs.getBool("simMode", SIMULATION_MODE);
  config.simTemp1 = prefs.getFloat("simTemp1", -22.0);
  config.simTemp2 = prefs.getFloat("simTemp2", -22.0);
  config.simDoorOpen = prefs.getBool("simDoor", false);
  
  Serial.println("[OK] Configuración cargada");
}

// ============================================
// GUARDAR CONFIGURACIÓN
// ============================================
void saveConfig() {
  prefs.putFloat("tempMax", config.tempMax);
  prefs.putFloat("tempCrit", config.tempCritical);
  prefs.putInt("alertDelay", config.alertDelaySec);
  prefs.putInt("doorMax", config.doorOpenMaxSec);
  prefs.putInt("defrostCD", config.defrostCooldownSec);
  prefs.putBool("defrostNC", config.defrostRelayNC);
  prefs.putBool("relayEn", config.relayEnabled);
  prefs.putBool("buzzerEn", config.buzzerEnabled);
  prefs.putBool("telegramEn", config.telegramEnabled);
  prefs.putBool("supabaseEn", config.supabaseEnabled);
  prefs.putBool("sensor1", config.sensor1Enabled);
  prefs.putBool("sensor2", config.sensor2Enabled);
  prefs.putBool("dht22", config.dht22Enabled);
  prefs.putBool("doorEn", config.doorEnabled);
  prefs.putBool("simMode", config.simulationMode);
  prefs.putFloat("simTemp1", config.simTemp1);
  prefs.putFloat("simTemp2", config.simTemp2);
  prefs.putBool("simDoor", config.simDoorOpen);
  
  Serial.println("[OK] Configuración guardada");
}

#endif
