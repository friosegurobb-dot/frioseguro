/*
 * door_sensors.h - Múltiples sensores de puerta (Reed Switch)
 * Sistema Monitoreo Reefer v3.0
 * 
 * NOTA: Este módulo NO está integrado aún. Solo definiciones.
 * 
 * Permite monitorear hasta 4 puertas/accesos diferentes.
 * Cada sensor es un Reed Switch (contacto magnético).
 * 
 * CONEXIONES:
 * - Puerta 1: GPIO5  (ya existente)
 * - Puerta 2: GPIO13
 * - Puerta 3: GPIO14
 * - Puerta 4: GPIO27
 */

#ifndef DOOR_SENSORS_H
#define DOOR_SENSORS_H

// ============================================
// CONFIGURACIÓN
// ============================================
#define MAX_DOORS 4

#define PIN_DOOR_1 5    // Puerta principal (ya existente)
#define PIN_DOOR_2 13   // Puerta secundaria
#define PIN_DOOR_3 14   // Acceso lateral
#define PIN_DOOR_4 27   // Compuerta trasera

// Tiempo máximo de puerta abierta antes de alertar (segundos)
#define DOOR_OPEN_ALERT_SEC 120  // 2 minutos

// ============================================
// ESTRUCTURAS
// ============================================
struct DoorSensor {
  uint8_t pin;
  const char* name;
  bool enabled;
  bool isOpen;
  unsigned long openSince;      // Timestamp de cuando se abrió
  unsigned long totalOpenTime;  // Tiempo total abierta (acumulado)
  int openCount;                // Cantidad de veces que se abrió
  bool alertSent;               // Ya se envió alerta por esta puerta
};

struct DoorsState {
  DoorSensor doors[MAX_DOORS];
  int enabledCount;
  bool anyDoorOpen;
  unsigned long lastCheck;
};

DoorsState doorsState;

// Forward declarations
extern void sendTelegramAlert(String message);
extern SystemState state;
extern Config config;

// ============================================
// INICIALIZACIÓN
// ============================================
void doorSensorsInit() {
  // Configurar puertas
  doorsState.doors[0] = {PIN_DOOR_1, "Principal", true, false, 0, 0, 0, false};
  doorsState.doors[1] = {PIN_DOOR_2, "Secundaria", false, false, 0, 0, 0, false};
  doorsState.doors[2] = {PIN_DOOR_3, "Lateral", false, false, 0, 0, 0, false};
  doorsState.doors[3] = {PIN_DOOR_4, "Trasera", false, false, 0, 0, 0, false};
  
  doorsState.enabledCount = 0;
  
  for (int i = 0; i < MAX_DOORS; i++) {
    if (doorsState.doors[i].enabled) {
      pinMode(doorsState.doors[i].pin, INPUT_PULLUP);
      doorsState.enabledCount++;
      Serial.printf("[DOORS] Puerta %d (%s) en GPIO%d\n", 
                    i+1, doorsState.doors[i].name, doorsState.doors[i].pin);
    }
  }
  
  Serial.printf("[DOORS] %d puertas habilitadas\n", doorsState.enabledCount);
}

// ============================================
// HABILITAR/DESHABILITAR PUERTA
// ============================================
void doorSetEnabled(int doorIndex, bool enabled) {
  if (doorIndex < 0 || doorIndex >= MAX_DOORS) return;
  
  doorsState.doors[doorIndex].enabled = enabled;
  
  if (enabled) {
    pinMode(doorsState.doors[doorIndex].pin, INPUT_PULLUP);
  }
  
  // Recontar puertas habilitadas
  doorsState.enabledCount = 0;
  for (int i = 0; i < MAX_DOORS; i++) {
    if (doorsState.doors[i].enabled) doorsState.enabledCount++;
  }
}

// ============================================
// LEER ESTADO DE UNA PUERTA
// ============================================
bool doorRead(int doorIndex) {
  if (doorIndex < 0 || doorIndex >= MAX_DOORS) return false;
  if (!doorsState.doors[doorIndex].enabled) return false;
  
  // Reed switch: LOW = cerrado (imán cerca), HIGH = abierto
  return digitalRead(doorsState.doors[doorIndex].pin) == HIGH;
}

// ============================================
// VERIFICAR TODAS LAS PUERTAS
// ============================================
void doorSensorsCheck() {
  doorsState.anyDoorOpen = false;
  
  for (int i = 0; i < MAX_DOORS; i++) {
    if (!doorsState.doors[i].enabled) continue;
    
    bool currentState = doorRead(i);
    DoorSensor* door = &doorsState.doors[i];
    
    // Detectar cambio de estado
    if (currentState && !door->isOpen) {
      // Puerta se ABRIÓ
      door->isOpen = true;
      door->openSince = millis();
      door->openCount++;
      door->alertSent = false;
      
      Serial.printf("[DOORS] 🚪 Puerta %s ABIERTA\n", door->name);
      
    } else if (!currentState && door->isOpen) {
      // Puerta se CERRÓ
      unsigned long openDuration = (millis() - door->openSince) / 1000;
      door->totalOpenTime += openDuration;
      door->isOpen = false;
      door->openSince = 0;
      door->alertSent = false;
      
      Serial.printf("[DOORS] 🚪 Puerta %s CERRADA (estuvo abierta %lu seg)\n", 
                    door->name, openDuration);
    }
    
    // Verificar si lleva mucho tiempo abierta
    if (door->isOpen) {
      doorsState.anyDoorOpen = true;
      unsigned long openSeconds = (millis() - door->openSince) / 1000;
      
      if (openSeconds >= DOOR_OPEN_ALERT_SEC && !door->alertSent) {
        // Enviar alerta
        String msg = "🚪 *PUERTA ABIERTA*\n\n";
        msg += "La puerta " + String(door->name) + " lleva ";
        msg += String(openSeconds / 60) + " minutos abierta.";
        
        if (state.internetAvailable && config.telegramEnabled) {
          sendTelegramAlert(msg);
        }
        
        door->alertSent = true;
        Serial.printf("[DOORS] ⚠️ Alerta: Puerta %s abierta %lu seg\n", 
                      door->name, openSeconds);
      }
    }
  }
}

// ============================================
// OBTENER TIEMPO ABIERTA (segundos)
// ============================================
unsigned long doorGetOpenSeconds(int doorIndex) {
  if (doorIndex < 0 || doorIndex >= MAX_DOORS) return 0;
  if (!doorsState.doors[doorIndex].isOpen) return 0;
  
  return (millis() - doorsState.doors[doorIndex].openSince) / 1000;
}

// ============================================
// LOOP PRINCIPAL
// ============================================
void doorSensorsLoop() {
  if (millis() - doorsState.lastCheck >= 500) {  // Cada 500ms
    doorsState.lastCheck = millis();
    doorSensorsCheck();
  }
}

// ============================================
// OBTENER ESTADO PARA API (JSON)
// ============================================
String doorSensorsGetJSON() {
  String json = "{\"doors\":[";
  
  for (int i = 0; i < MAX_DOORS; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"index\":" + String(i) + ",";
    json += "\"name\":\"" + String(doorsState.doors[i].name) + "\",";
    json += "\"enabled\":" + String(doorsState.doors[i].enabled ? "true" : "false") + ",";
    json += "\"is_open\":" + String(doorsState.doors[i].isOpen ? "true" : "false") + ",";
    json += "\"open_seconds\":" + String(doorGetOpenSeconds(i)) + ",";
    json += "\"open_count\":" + String(doorsState.doors[i].openCount);
    json += "}";
  }
  
  json += "],\"any_open\":" + String(doorsState.anyDoorOpen ? "true" : "false") + "}";
  return json;
}

#endif
