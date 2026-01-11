/*
 * power_monitor.h - Monitor de corte de luz / alimentación
 * Sistema Monitoreo Reefer v3.0
 * 
 * NOTA: Este módulo NO está integrado aún. Solo definiciones.
 * 
 * MÉTODOS DE DETECCIÓN:
 * 1. Divisor de voltaje desde 220V AC (con optoacoplador)
 * 2. Sensor de voltaje ZMPT101B (más preciso)
 * 3. Detección simple: 5V del cargador vs batería
 * 
 * CONEXIONES RECOMENDADAS:
 * - PIN_POWER_DETECT: GPIO34 (ADC, solo entrada)
 * - Usar optoacoplador PC817 para aislar 220V
 */

#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

// ============================================
// CONFIGURACIÓN
// ============================================
#define PIN_POWER_DETECT 34       // GPIO34 - ADC para detectar voltaje
#define PIN_BATTERY_LEVEL 35      // GPIO35 - ADC para nivel de batería (opcional)

#define POWER_CHECK_INTERVAL 1000 // Verificar cada 1 segundo
#define POWER_THRESHOLD 2000      // Umbral ADC (0-4095) para detectar corte
#define POWER_DEBOUNCE_MS 3000    // 3 segundos de debounce para evitar falsos positivos

// ============================================
// VARIABLES
// ============================================
struct PowerState {
  bool acPowerPresent;        // true = hay luz, false = corte
  bool batteryBackup;         // true = funcionando con batería
  float batteryVoltage;       // Voltaje de batería (si hay sensor)
  unsigned long powerLostTime;    // Timestamp de cuando se perdió la luz
  unsigned long powerRestoredTime; // Timestamp de cuando volvió
  bool alertSent;             // Ya se envió alerta de corte
  unsigned long lastCheck;
};

PowerState powerState;

// Forward declarations
extern void sim800SendPowerAlert(bool powerLost);
extern void sendTelegramAlert(String message);
extern SystemState state;
extern Config config;

// ============================================
// INICIALIZACIÓN
// ============================================
void powerMonitorInit() {
  pinMode(PIN_POWER_DETECT, INPUT);
  
  #ifdef PIN_BATTERY_LEVEL
    pinMode(PIN_BATTERY_LEVEL, INPUT);
  #endif
  
  // Leer estado inicial
  int reading = analogRead(PIN_POWER_DETECT);
  powerState.acPowerPresent = (reading > POWER_THRESHOLD);
  powerState.batteryBackup = !powerState.acPowerPresent;
  powerState.alertSent = false;
  powerState.lastCheck = millis();
  
  Serial.printf("[POWER] Inicializado. AC: %s\n", 
                powerState.acPowerPresent ? "OK" : "SIN LUZ");
}

// ============================================
// LEER VOLTAJE DE BATERÍA
// ============================================
float powerReadBatteryVoltage() {
  #ifdef PIN_BATTERY_LEVEL
    int raw = analogRead(PIN_BATTERY_LEVEL);
    // Asumiendo divisor de voltaje 2:1 y batería LiPo 3.7V
    // ADC 12-bit (0-4095) con referencia 3.3V
    float voltage = (raw / 4095.0) * 3.3 * 2.0;
    powerState.batteryVoltage = voltage;
    return voltage;
  #else
    return 0.0;
  #endif
}

// ============================================
// VERIFICAR ESTADO DE ALIMENTACIÓN
// ============================================
void powerMonitorCheck() {
  static unsigned long lastStateChange = 0;
  static bool lastState = true;
  
  int reading = analogRead(PIN_POWER_DETECT);
  bool currentState = (reading > POWER_THRESHOLD);
  
  // Debounce - esperar que el estado sea estable
  if (currentState != lastState) {
    lastStateChange = millis();
    lastState = currentState;
    return;
  }
  
  // Si pasó el tiempo de debounce y el estado cambió
  if (millis() - lastStateChange >= POWER_DEBOUNCE_MS) {
    if (currentState != powerState.acPowerPresent) {
      powerState.acPowerPresent = currentState;
      powerState.batteryBackup = !currentState;
      
      if (!currentState) {
        // CORTE DE LUZ DETECTADO
        powerState.powerLostTime = millis();
        powerState.alertSent = false;
        
        Serial.println("⚡ [POWER] ¡CORTE DE LUZ DETECTADO!");
        
        // Enviar alerta por SMS (SIM800)
        sim800SendPowerAlert(true);
        
        // Enviar por Telegram si hay internet (puede que no haya)
        if (state.internetAvailable && config.telegramEnabled) {
          sendTelegramAlert("⚡ *CORTE DE LUZ*\n\nSe detectó un corte de energía eléctrica.\nEl sistema está funcionando con batería de respaldo.");
        }
        
        powerState.alertSent = true;
        
      } else {
        // LUZ RESTAURADA
        powerState.powerRestoredTime = millis();
        unsigned long outageMinutes = (powerState.powerRestoredTime - powerState.powerLostTime) / 60000;
        
        Serial.printf("✅ [POWER] Luz restaurada. Duración corte: %lu min\n", outageMinutes);
        
        // Notificar restauración
        sim800SendPowerAlert(false);
        
        if (state.internetAvailable && config.telegramEnabled) {
          String msg = "✅ *LUZ RESTAURADA*\n\n";
          msg += "El suministro eléctrico ha vuelto.\n";
          msg += "Duración del corte: " + String(outageMinutes) + " minutos";
          sendTelegramAlert(msg);
        }
      }
    }
  }
}

// ============================================
// OBTENER TIEMPO SIN LUZ
// ============================================
unsigned long powerGetOutageSeconds() {
  if (powerState.acPowerPresent) return 0;
  return (millis() - powerState.powerLostTime) / 1000;
}

// ============================================
// LOOP PRINCIPAL (llamar desde loop())
// ============================================
void powerMonitorLoop() {
  if (millis() - powerState.lastCheck >= POWER_CHECK_INTERVAL) {
    powerState.lastCheck = millis();
    powerMonitorCheck();
    
    // Actualizar voltaje de batería
    powerReadBatteryVoltage();
  }
}

// ============================================
// OBTENER ESTADO PARA API
// ============================================
String powerGetStatusJSON() {
  String json = "{";
  json += "\"ac_power\":" + String(powerState.acPowerPresent ? "true" : "false") + ",";
  json += "\"battery_backup\":" + String(powerState.batteryBackup ? "true" : "false") + ",";
  json += "\"battery_voltage\":" + String(powerState.batteryVoltage, 2) + ",";
  json += "\"outage_seconds\":" + String(powerGetOutageSeconds());
  json += "}";
  return json;
}

#endif
