/*
 * current_sensor.h - Sensor de corriente para mantenimiento preventivo
 * Sistema Monitoreo Reefer v3.0
 * 
 * NOTA: Este módulo NO está integrado aún. Solo definiciones.
 * 
 * SENSORES COMPATIBLES:
 * - ACS712 (5A, 20A, 30A) - Más común y económico
 * - SCT-013 (pinza amperimétrica) - No invasivo
 * - INA219 (I2C) - Más preciso, mide voltaje y corriente
 * 
 * USO:
 * - Detectar si el compresor está funcionando
 * - Medir consumo del sistema de refrigeración
 * - Alertar si el consumo es anormal (compresor trabado, capacitor dañado)
 * - Mantenimiento preventivo basado en horas de funcionamiento
 * 
 * CONEXIONES ACS712:
 * - VCC: 5V
 * - GND: GND
 * - OUT: GPIO36 (ADC, solo entrada)
 */

#ifndef CURRENT_SENSOR_H
#define CURRENT_SENSOR_H

// ============================================
// CONFIGURACIÓN
// ============================================
#define PIN_CURRENT_SENSOR 36     // GPIO36 - ADC (VP)
#define CURRENT_SENSOR_TYPE 20    // ACS712: 5, 20, o 30 Amperes

// Calibración ACS712
#if CURRENT_SENSOR_TYPE == 5
  #define ACS712_SENSITIVITY 0.185  // 185mV/A para 5A
#elif CURRENT_SENSOR_TYPE == 20
  #define ACS712_SENSITIVITY 0.100  // 100mV/A para 20A
#else
  #define ACS712_SENSITIVITY 0.066  // 66mV/A para 30A
#endif

#define ACS712_OFFSET 2.5           // Voltaje en reposo (2.5V para 5V VCC)
#define ADC_VREF 3.3                // Referencia ADC del ESP32
#define ADC_RESOLUTION 4095.0       // 12-bit ADC

// Umbrales de corriente (Amperes)
#define CURRENT_COMPRESSOR_MIN 2.0    // Mínimo esperado cuando funciona
#define CURRENT_COMPRESSOR_MAX 15.0   // Máximo normal
#define CURRENT_OVERCURRENT 20.0      // Sobrecorriente - ALERTA
#define CURRENT_IDLE_MAX 0.5          // Máximo en reposo

// Intervalos
#define CURRENT_READ_INTERVAL 1000    // Leer cada 1 segundo
#define CURRENT_SAMPLES 100           // Muestras para promediar (AC)

// ============================================
// ESTRUCTURAS
// ============================================
struct CurrentSensorState {
  float currentAmps;          // Corriente actual (RMS)
  float currentPeak;          // Pico máximo detectado
  float currentAvg;           // Promedio últimos 10 segundos
  bool compressorRunning;     // true si el compresor está funcionando
  unsigned long compressorOnTime;   // Timestamp de encendido
  unsigned long compressorTotalHours; // Horas totales de funcionamiento
  bool overcurrentAlert;      // Alerta de sobrecorriente activa
  bool undercurrentAlert;     // Alerta de baja corriente (compresor trabado)
  unsigned long lastRead;
  
  // Para mantenimiento preventivo
  int startCount;             // Cantidad de arranques del compresor
  float maxCurrentEver;       // Máxima corriente registrada
};

CurrentSensorState currentState;

// Forward declarations
extern void sendTelegramAlert(String message);
extern SystemState state;
extern Config config;

// ============================================
// INICIALIZACIÓN
// ============================================
void currentSensorInit() {
  pinMode(PIN_CURRENT_SENSOR, INPUT);
  
  currentState.currentAmps = 0;
  currentState.currentPeak = 0;
  currentState.compressorRunning = false;
  currentState.compressorTotalHours = 0;  // TODO: Cargar de Preferences
  currentState.overcurrentAlert = false;
  currentState.undercurrentAlert = false;
  currentState.startCount = 0;
  currentState.maxCurrentEver = 0;
  currentState.lastRead = millis();
  
  Serial.printf("[CURRENT] Sensor ACS712-%dA inicializado en GPIO%d\n", 
                CURRENT_SENSOR_TYPE, PIN_CURRENT_SENSOR);
}

// ============================================
// LEER CORRIENTE (RMS para AC)
// ============================================
float currentSensorRead() {
  long sum = 0;
  int samples = CURRENT_SAMPLES;
  
  // Tomar múltiples muestras para calcular RMS
  for (int i = 0; i < samples; i++) {
    int raw = analogRead(PIN_CURRENT_SENSOR);
    // Convertir a voltaje
    float voltage = (raw / ADC_RESOLUTION) * ADC_VREF;
    // Restar offset y calcular corriente
    float current = (voltage - ACS712_OFFSET) / ACS712_SENSITIVITY;
    sum += current * current;  // Sumar cuadrados para RMS
    delayMicroseconds(200);    // ~5kHz de muestreo
  }
  
  // Calcular RMS
  float rms = sqrt(sum / samples);
  return abs(rms);
}

// ============================================
// VERIFICAR ESTADO DEL COMPRESOR
// ============================================
void currentSensorCheck() {
  float current = currentSensorRead();
  currentState.currentAmps = current;
  
  // Actualizar pico
  if (current > currentState.currentPeak) {
    currentState.currentPeak = current;
  }
  if (current > currentState.maxCurrentEver) {
    currentState.maxCurrentEver = current;
  }
  
  // Detectar si el compresor está funcionando
  bool wasRunning = currentState.compressorRunning;
  currentState.compressorRunning = (current > CURRENT_COMPRESSOR_MIN);
  
  // Detectar arranque del compresor
  if (currentState.compressorRunning && !wasRunning) {
    currentState.compressorOnTime = millis();
    currentState.startCount++;
    Serial.printf("[CURRENT] ⚡ Compresor ENCENDIDO (arranque #%d)\n", 
                  currentState.startCount);
  }
  
  // Detectar apagado del compresor
  if (!currentState.compressorRunning && wasRunning) {
    unsigned long runMinutes = (millis() - currentState.compressorOnTime) / 60000;
    currentState.compressorTotalHours += runMinutes / 60.0;
    Serial.printf("[CURRENT] ⚡ Compresor APAGADO (funcionó %lu min)\n", runMinutes);
  }
  
  // ALERTA: Sobrecorriente
  if (current > CURRENT_OVERCURRENT && !currentState.overcurrentAlert) {
    currentState.overcurrentAlert = true;
    
    String msg = "⚡ *SOBRECORRIENTE DETECTADA*\n\n";
    msg += "Corriente: " + String(current, 1) + "A\n";
    msg += "Límite: " + String(CURRENT_OVERCURRENT, 1) + "A\n\n";
    msg += "⚠️ Posible problema:\n";
    msg += "- Compresor trabado\n";
    msg += "- Capacitor dañado\n";
    msg += "- Cortocircuito";
    
    Serial.println("[CURRENT] ⚠️ SOBRECORRIENTE: " + String(current, 1) + "A");
    
    if (state.internetAvailable && config.telegramEnabled) {
      sendTelegramAlert(msg);
    }
  }
  
  // Reset alerta si la corriente vuelve a normal
  if (current < CURRENT_COMPRESSOR_MAX && currentState.overcurrentAlert) {
    currentState.overcurrentAlert = false;
    Serial.println("[CURRENT] ✓ Corriente normalizada");
  }
  
  // ALERTA: Compresor debería estar funcionando pero no hay corriente
  // (esto requiere lógica adicional basada en temperatura)
}

// ============================================
// OBTENER HORAS DE FUNCIONAMIENTO
// ============================================
float currentGetCompressorHours() {
  float hours = currentState.compressorTotalHours;
  
  // Agregar tiempo actual si está funcionando
  if (currentState.compressorRunning) {
    hours += (millis() - currentState.compressorOnTime) / 3600000.0;
  }
  
  return hours;
}

// ============================================
// VERIFICAR MANTENIMIENTO PREVENTIVO
// ============================================
String currentCheckMaintenance() {
  String status = "";
  float hours = currentGetCompressorHours();
  
  // Alertas de mantenimiento basadas en horas
  if (hours > 5000) {
    status = "⚠️ CRÍTICO: Más de 5000 horas. Revisar compresor urgente.";
  } else if (hours > 3000) {
    status = "⚠️ Más de 3000 horas. Programar mantenimiento preventivo.";
  } else if (hours > 1000) {
    status = "ℹ️ Más de 1000 horas. Verificar refrigerante y filtros.";
  } else {
    status = "✓ Sistema en buen estado";
  }
  
  // Verificar cantidad de arranques (muchos arranques = problema)
  if (currentState.startCount > 100) {
    status += "\n⚠️ Muchos ciclos de arranque. Verificar termostato.";
  }
  
  return status;
}

// ============================================
// LOOP PRINCIPAL
// ============================================
void currentSensorLoop() {
  if (millis() - currentState.lastRead >= CURRENT_READ_INTERVAL) {
    currentState.lastRead = millis();
    currentSensorCheck();
  }
}

// ============================================
// OBTENER ESTADO PARA API (JSON)
// ============================================
String currentSensorGetJSON() {
  String json = "{";
  json += "\"current_amps\":" + String(currentState.currentAmps, 2) + ",";
  json += "\"current_peak\":" + String(currentState.currentPeak, 2) + ",";
  json += "\"compressor_running\":" + String(currentState.compressorRunning ? "true" : "false") + ",";
  json += "\"compressor_hours\":" + String(currentGetCompressorHours(), 1) + ",";
  json += "\"start_count\":" + String(currentState.startCount) + ",";
  json += "\"overcurrent_alert\":" + String(currentState.overcurrentAlert ? "true" : "false") + ",";
  json += "\"max_current_ever\":" + String(currentState.maxCurrentEver, 2) + ",";
  json += "\"maintenance\":\"" + currentCheckMaintenance() + "\"";
  json += "}";
  return json;
}

#endif
