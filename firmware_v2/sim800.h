/*
 * sim800.h - Módulo GSM SIM800L para SMS de emergencia
 * Sistema Monitoreo Reefer v3.0
 * 
 * NOTA: Este módulo NO está integrado aún. Solo definiciones.
 * 
 * CONEXIONES SIM800L:
 * - VCC: 3.7-4.2V (usar regulador, NO conectar directo a 5V)
 * - GND: GND
 * - TXD: GPIO16 (RX2 del ESP32)
 * - RXD: GPIO17 (TX2 del ESP32) - usar divisor de voltaje 5V->3.3V
 * - RST: GPIO25 (opcional, para reset por hardware)
 */

#ifndef SIM800_H
#define SIM800_H

#include <HardwareSerial.h>

// ============================================
// CONFIGURACIÓN SIM800
// ============================================
#define SIM800_RX_PIN 16      // ESP32 RX <- SIM800 TX
#define SIM800_TX_PIN 17      // ESP32 TX -> SIM800 RX
#define SIM800_RST_PIN 25     // Reset del SIM800 (opcional)
#define SIM800_BAUD 9600

// Números de teléfono para SMS de emergencia
#define SMS_PHONE_1 "+56912345678"  // Cambiar por número real
#define SMS_PHONE_2 "+56987654321"  // Segundo número (opcional)
#define SMS_PHONE_COUNT 1           // Cantidad de números activos

const char* SMS_PHONES[] = {
  SMS_PHONE_1,
  // SMS_PHONE_2,
};

// ============================================
// VARIABLES
// ============================================
HardwareSerial sim800Serial(2);  // UART2 del ESP32

struct SIM800State {
  bool initialized;
  bool registered;        // Registrado en red
  int signalStrength;     // 0-31 (CSQ)
  String operatorName;
  unsigned long lastCheck;
  bool smsSent;
};

SIM800State sim800State;

// ============================================
// INICIALIZACIÓN
// ============================================
bool sim800Init() {
  Serial.println("[SIM800] Inicializando...");
  
  sim800Serial.begin(SIM800_BAUD, SERIAL_8N1, SIM800_RX_PIN, SIM800_TX_PIN);
  delay(1000);
  
  // Reset por hardware si está conectado
  #ifdef SIM800_RST_PIN
    pinMode(SIM800_RST_PIN, OUTPUT);
    digitalWrite(SIM800_RST_PIN, LOW);
    delay(100);
    digitalWrite(SIM800_RST_PIN, HIGH);
    delay(3000);
  #endif
  
  // Test AT
  sim800Serial.println("AT");
  delay(500);
  
  if (sim800Serial.find("OK")) {
    Serial.println("[SIM800] ✓ Módulo respondiendo");
    sim800State.initialized = true;
    
    // Configurar modo texto para SMS
    sim800Serial.println("AT+CMGF=1");
    delay(500);
    
    // Verificar registro en red
    sim800Serial.println("AT+CREG?");
    delay(500);
    
    return true;
  } else {
    Serial.println("[SIM800] ✗ No responde");
    sim800State.initialized = false;
    return false;
  }
}

// ============================================
// VERIFICAR SEÑAL
// ============================================
int sim800GetSignal() {
  if (!sim800State.initialized) return -1;
  
  sim800Serial.println("AT+CSQ");
  delay(500);
  
  // Respuesta: +CSQ: XX,YY donde XX es la señal (0-31)
  if (sim800Serial.find("+CSQ:")) {
    int signal = sim800Serial.parseInt();
    sim800State.signalStrength = signal;
    return signal;
  }
  return -1;
}

// ============================================
// VERIFICAR REGISTRO EN RED
// ============================================
bool sim800IsRegistered() {
  if (!sim800State.initialized) return false;
  
  sim800Serial.println("AT+CREG?");
  delay(500);
  
  // Respuesta: +CREG: 0,1 (1=registrado, 5=roaming)
  if (sim800Serial.find("+CREG:")) {
    sim800Serial.parseInt();  // Ignorar primer número
    int status = sim800Serial.parseInt();
    sim800State.registered = (status == 1 || status == 5);
    return sim800State.registered;
  }
  return false;
}

// ============================================
// ENVIAR SMS
// ============================================
bool sim800SendSMS(const char* phone, String message) {
  if (!sim800State.initialized || !sim800State.registered) {
    Serial.println("[SIM800] ✗ No inicializado o sin red");
    return false;
  }
  
  Serial.printf("[SIM800] Enviando SMS a %s...\n", phone);
  
  // Comando AT para enviar SMS
  sim800Serial.print("AT+CMGS=\"");
  sim800Serial.print(phone);
  sim800Serial.println("\"");
  delay(500);
  
  // Esperar prompt '>'
  if (sim800Serial.find(">")) {
    sim800Serial.print(message);
    sim800Serial.write(26);  // Ctrl+Z para enviar
    delay(5000);
    
    if (sim800Serial.find("OK")) {
      Serial.println("[SIM800] ✓ SMS enviado");
      return true;
    }
  }
  
  Serial.println("[SIM800] ✗ Error enviando SMS");
  return false;
}

// ============================================
// ENVIAR ALERTA POR SMS A TODOS LOS NÚMEROS
// ============================================
void sim800SendAlert(String alertMessage) {
  String fullMsg = "ALERTA REEFER\n";
  fullMsg += alertMessage;
  fullMsg += "\n\nSistema Monitoreo Reefer";
  
  for (int i = 0; i < SMS_PHONE_COUNT; i++) {
    sim800SendSMS(SMS_PHONES[i], fullMsg);
    delay(2000);  // Pausa entre SMS
  }
  
  sim800State.smsSent = true;
}

// ============================================
// ENVIAR SMS DE CORTE DE LUZ
// ============================================
void sim800SendPowerAlert(bool powerLost) {
  String msg;
  if (powerLost) {
    msg = "⚠️ CORTE DE LUZ DETECTADO\n";
    msg += "El sistema está funcionando con batería.\n";
    msg += "Verificar suministro eléctrico.";
  } else {
    msg = "✅ LUZ RESTAURADA\n";
    msg += "El suministro eléctrico ha vuelto.";
  }
  
  sim800SendAlert(msg);
}

// ============================================
// LOOP DE VERIFICACIÓN (llamar periódicamente)
// ============================================
void sim800Loop() {
  if (!sim800State.initialized) return;
  
  // Verificar cada 60 segundos
  if (millis() - sim800State.lastCheck >= 60000) {
    sim800State.lastCheck = millis();
    sim800GetSignal();
    sim800IsRegistered();
    
    Serial.printf("[SIM800] Señal: %d/31, Red: %s\n", 
                  sim800State.signalStrength,
                  sim800State.registered ? "OK" : "NO");
  }
}

#endif
