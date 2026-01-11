/*
 * ============================================================================
 * SENSORS.H - LECTURA DE SENSORES v4.0
 * Sistema Monitoreo Reefer Industrial
 * ============================================================================
 * 
 * Soporta:
 * - Hasta 6 sondas DS18B20 en bus 1-Wire
 * - Hasta 3 sensores de puerta (reed switch)
 * - 1 sensor DHT22 (temperatura ambiente + humedad)
 * 
 * Todas las operaciones son NO BLOQUEANTES
 * 
 * ============================================================================
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include "config.h"
#include "types.h"

// Referencias externas
extern OneWire oneWire;
extern DallasTemperature ds18b20;
extern DHT dht;
extern Config config;
extern SensorData sensorData;
extern SystemState state;

// ============================================================================
// NOMBRES DE SENSORES (arrays para acceso por índice)
// ============================================================================
const char* TEMP_SENSOR_NAMES[MAX_TEMP_SENSORS] = {
    TEMP_SENSOR_1_NAME,
    TEMP_SENSOR_2_NAME,
    TEMP_SENSOR_3_NAME,
    TEMP_SENSOR_4_NAME,
    TEMP_SENSOR_5_NAME,
    TEMP_SENSOR_6_NAME
};

const uint8_t DOOR_PINS[MAX_DOOR_SENSORS] = {
    PIN_DOOR_1,
    PIN_DOOR_2,
    PIN_DOOR_3
};

const char* DOOR_NAMES[MAX_DOOR_SENSORS] = {
    DOOR_1_NAME,
    DOOR_2_NAME,
    DOOR_3_NAME
};

const bool DOOR_ENABLED_DEFAULT[MAX_DOOR_SENSORS] = {
    DOOR_1_ENABLED,
    DOOR_2_ENABLED,
    DOOR_3_ENABLED
};

// ============================================================================
// INICIALIZACIÓN DE SENSORES DE TEMPERATURA
// ============================================================================
void initTempSensors() {
    Serial.println("[SENSOR] Iniciando sensores DS18B20...");
    
    ds18b20.begin();
    delay(500);
    
    int count = ds18b20.getDeviceCount();
    sensorData.tempSensorCount = count;
    
    Serial.printf("[SENSOR] Sensores DS18B20 detectados: %d\n", count);
    
    if (count == 0) {
        Serial.println("[SENSOR] ⚠️ NO HAY SENSORES - Reintentando...");
        delay(1000);
        ds18b20.begin();
        delay(500);
        count = ds18b20.getDeviceCount();
        sensorData.tempSensorCount = count;
        Serial.printf("[SENSOR] Segundo intento: %d sensores\n", count);
    }
    
    // Inicializar estructuras de cada sensor
    for (int i = 0; i < MAX_TEMP_SENSORS; i++) {
        sensorData.temp[i].value = -127.0;
        sensorData.temp[i].minToday = 999.0;
        sensorData.temp[i].maxToday = -999.0;
        sensorData.temp[i].valid = false;
        sensorData.temp[i].name = TEMP_SENSOR_NAMES[i];
        sensorData.temp[i].enabled = config.tempSensorEnabled[i] && (i < count);
        
        // Obtener dirección del sensor si existe
        if (i < count) {
            ds18b20.getAddress(sensorData.temp[i].address, i);
        }
    }
    
    // Configurar resolución y hacer primera lectura
    if (count > 0) {
        ds18b20.setResolution(12);  // 12 bits = 0.0625°C precisión
        ds18b20.setWaitForConversion(true);
        ds18b20.requestTemperatures();
        
        float t = ds18b20.getTempCByIndex(0);
        Serial.printf("[SENSOR] >>> TEMPERATURA INICIAL: %.2f°C <<<\n", t);
        
        if (t > -55 && t < 125) {
            sensorData.temp[0].value = t;
            sensorData.temp[0].valid = true;
            sensorData.tempAvg = t;
            sensorData.tempValid = true;
        }
        
        // Cambiar a modo no bloqueante
        ds18b20.setWaitForConversion(false);
    }
}

// ============================================================================
// INICIALIZACIÓN DE SENSORES DE PUERTA
// ============================================================================
void initDoorSensors() {
    Serial.println("[SENSOR] Iniciando sensores de puerta...");
    
    for (int i = 0; i < MAX_DOOR_SENSORS; i++) {
        sensorData.door[i].pin = DOOR_PINS[i];
        sensorData.door[i].name = DOOR_NAMES[i];
        sensorData.door[i].enabled = config.doorEnabled[i];
        sensorData.door[i].isOpen = false;
        sensorData.door[i].openSince = 0;
        sensorData.door[i].totalOpenToday = 0;
        sensorData.door[i].opensToday = 0;
        
        if (sensorData.door[i].enabled) {
            pinMode(DOOR_PINS[i], INPUT_PULLUP);
            Serial.printf("[SENSOR] Puerta %d (%s) en GPIO%d\n", 
                          i + 1, DOOR_NAMES[i], DOOR_PINS[i]);
        }
    }
}

// ============================================================================
// INICIALIZACIÓN DE DHT22
// ============================================================================
void initDHT22() {
    if (config.dht22Enabled) {
        dht.begin();
        Serial.println("[SENSOR] DHT22 inicializado");
    }
}

// ============================================================================
// INICIALIZACIÓN COMPLETA DE SENSORES
// ============================================================================
void initSensors() {
    initTempSensors();
    initDoorSensors();
    initDHT22();
    
    Serial.println("[SENSOR] ✓ Todos los sensores inicializados");
}

// ============================================================================
// LECTURA DE TEMPERATURAS (NO BLOQUEANTE)
// ============================================================================
// Usa máquina de estados interna para lectura asíncrona

enum TempReadState { TEMP_IDLE, TEMP_REQUESTING, TEMP_READING };
static TempReadState tempReadState = TEMP_IDLE;
static unsigned long tempRequestTime = 0;

void readTempSensors() {
    // Modo simulación
    if (config.simulationMode) {
        sensorData.temp[0].value = config.simTemp;
        sensorData.temp[0].valid = true;
        sensorData.tempAvg = config.simTemp;
        sensorData.tempValid = true;
        return;
    }
    
    switch (tempReadState) {
        case TEMP_IDLE:
            // Iniciar solicitud de conversión
            ds18b20.requestTemperatures();
            tempRequestTime = millis();
            tempReadState = TEMP_REQUESTING;
            break;
            
        case TEMP_REQUESTING:
            // Esperar 750ms para conversión de 12 bits
            if (millis() - tempRequestTime >= 750) {
                tempReadState = TEMP_READING;
            }
            break;
            
        case TEMP_READING:
            // Leer todos los sensores
            float sum = 0;
            int validCount = 0;
            float minTemp = 999.0;
            float maxTemp = -999.0;
            
            for (int i = 0; i < sensorData.tempSensorCount && i < MAX_TEMP_SENSORS; i++) {
                if (!sensorData.temp[i].enabled) continue;
                
                float t = ds18b20.getTempCByIndex(i);
                
                if (t > -55 && t < 125) {
                    sensorData.temp[i].value = t;
                    sensorData.temp[i].valid = true;
                    
                    // Actualizar min/max del día
                    if (t < sensorData.temp[i].minToday) {
                        sensorData.temp[i].minToday = t;
                    }
                    if (t > sensorData.temp[i].maxToday) {
                        sensorData.temp[i].maxToday = t;
                    }
                    
                    sum += t;
                    validCount++;
                    
                    if (t < minTemp) minTemp = t;
                    if (t > maxTemp) maxTemp = t;
                } else {
                    sensorData.temp[i].valid = false;
                }
            }
            
            // Calcular promedios globales
            if (validCount > 0) {
                sensorData.tempAvg = sum / validCount;
                sensorData.tempMin = minTemp;
                sensorData.tempMax = maxTemp;
                sensorData.tempValid = true;
            } else {
                sensorData.tempValid = false;
            }
            
            tempReadState = TEMP_IDLE;
            break;
    }
}

// ============================================================================
// LECTURA DE PUERTAS
// ============================================================================
void readDoorSensors() {
    // Modo simulación
    if (config.simulationMode) {
        sensorData.door[0].isOpen = config.simDoorOpen;
        sensorData.anyDoorOpen = config.simDoorOpen;
        return;
    }
    
    sensorData.anyDoorOpen = false;
    sensorData.doorsOpenCount = 0;
    
    for (int i = 0; i < MAX_DOOR_SENSORS; i++) {
        if (!sensorData.door[i].enabled) continue;
        
        bool wasOpen = sensorData.door[i].isOpen;
        bool isOpen = digitalRead(sensorData.door[i].pin) == HIGH;
        
        sensorData.door[i].isOpen = isOpen;
        
        if (isOpen) {
            sensorData.anyDoorOpen = true;
            sensorData.doorsOpenCount++;
            
            // Registrar inicio de apertura
            if (!wasOpen) {
                sensorData.door[i].openSince = millis();
                sensorData.door[i].opensToday++;
                Serial.printf("[PUERTA] %s ABIERTA\n", sensorData.door[i].name);
            }
        } else {
            // Registrar cierre
            if (wasOpen && sensorData.door[i].openSince > 0) {
                unsigned long openDuration = (millis() - sensorData.door[i].openSince) / 1000;
                sensorData.door[i].totalOpenToday += openDuration;
                sensorData.door[i].openSince = 0;
                Serial.printf("[PUERTA] %s CERRADA (estuvo abierta %lu seg)\n", 
                              sensorData.door[i].name, openDuration);
            }
        }
    }
}

// ============================================================================
// LECTURA DE DHT22
// ============================================================================
void readDHT22() {
    if (!config.dht22Enabled) return;
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (!isnan(h)) {
        sensorData.humidity = h;
        sensorData.dhtValid = true;
    }
    if (!isnan(t)) {
        sensorData.tempAmbient = t;
    }
}

// ============================================================================
// LECTURA COMPLETA DE SENSORES (llamar desde loop)
// ============================================================================
void readSensors() {
    readTempSensors();
    readDoorSensors();
    readDHT22();
    
    state.lastSensorRead = millis();
}

// ============================================================================
// OBTENER JSON DE SENSORES
// ============================================================================
void getSensorsJSON(JsonObject& obj) {
    // Temperaturas
    JsonObject temps = obj.createNestedObject("temperatures");
    temps["avg"] = sensorData.tempAvg;
    temps["min"] = sensorData.tempMin;
    temps["max"] = sensorData.tempMax;
    temps["valid"] = sensorData.tempValid;
    temps["sensor_count"] = sensorData.tempSensorCount;
    
    JsonArray tempArray = temps.createNestedArray("sensors");
    for (int i = 0; i < MAX_TEMP_SENSORS; i++) {
        if (!sensorData.temp[i].enabled) continue;
        
        JsonObject sensor = tempArray.createNestedObject();
        sensor["index"] = i;
        sensor["name"] = sensorData.temp[i].name;
        sensor["value"] = sensorData.temp[i].value;
        sensor["valid"] = sensorData.temp[i].valid;
        sensor["min_today"] = sensorData.temp[i].minToday;
        sensor["max_today"] = sensorData.temp[i].maxToday;
    }
    
    // DHT22
    if (config.dht22Enabled) {
        JsonObject dhtObj = obj.createNestedObject("dht22");
        dhtObj["humidity"] = sensorData.humidity;
        dhtObj["temp_ambient"] = sensorData.tempAmbient;
        dhtObj["valid"] = sensorData.dhtValid;
    }
    
    // Puertas
    JsonObject doors = obj.createNestedObject("doors");
    doors["any_open"] = sensorData.anyDoorOpen;
    doors["open_count"] = sensorData.doorsOpenCount;
    
    JsonArray doorArray = doors.createNestedArray("sensors");
    for (int i = 0; i < MAX_DOOR_SENSORS; i++) {
        if (!sensorData.door[i].enabled) continue;
        
        JsonObject door = doorArray.createNestedObject();
        door["index"] = i;
        door["name"] = sensorData.door[i].name;
        door["is_open"] = sensorData.door[i].isOpen;
        door["open_since_sec"] = sensorData.door[i].isOpen ? 
            (millis() - sensorData.door[i].openSince) / 1000 : 0;
        door["opens_today"] = sensorData.door[i].opensToday;
        door["total_open_today_sec"] = sensorData.door[i].totalOpenToday;
    }
}

// ============================================================================
// RESETEAR ESTADÍSTICAS DIARIAS (llamar a medianoche)
// ============================================================================
void resetDailyStats() {
    for (int i = 0; i < MAX_TEMP_SENSORS; i++) {
        sensorData.temp[i].minToday = 999.0;
        sensorData.temp[i].maxToday = -999.0;
    }
    
    for (int i = 0; i < MAX_DOOR_SENSORS; i++) {
        sensorData.door[i].totalOpenToday = 0;
        sensorData.door[i].opensToday = 0;
    }
    
    Serial.println("[SENSOR] Estadísticas diarias reseteadas");
}

#endif // SENSORS_H
