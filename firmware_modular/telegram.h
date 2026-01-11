/*
 * telegram.h - Notificaciones Telegram
 * Sistema Monitoreo Reefer v3.0
 */

#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "types.h"

extern Config config;
extern SystemState state;
extern SensorData sensorData;

// ============================================
// ENVIAR MENSAJE A TELEGRAM
// ============================================
void sendTelegramMessage(String message) {
  if (strlen(TELEGRAM_BOT_TOKEN) < 10) return;
  if (millis() - state.lastTelegramAlert < 300000) return;
  
  HTTPClient http;
  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";
  
  for (int i = 0; i < TELEGRAM_CHAT_COUNT; i++) {
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<512> doc;
    doc["chat_id"] = TELEGRAM_CHAT_IDS[i];
    doc["text"] = message;
    doc["parse_mode"] = "Markdown";
    
    String body;
    serializeJson(doc, body);
    
    int code = http.POST(body);
    Serial.printf("[TELEGRAM] Enviado a %s: %d\n", TELEGRAM_CHAT_IDS[i], code);
    http.end();
  }
  
  state.lastTelegramAlert = millis();
}

// ============================================
// ENVIAR ALERTA A TELEGRAM
// ============================================
void sendTelegramAlert(String message) {
  String fullMsg = "🏔️ *" + String(DEVICE_NAME) + "*\n\n" + message;
  fullMsg += "\n\n📍 " + String(LOCATION_DETAIL);
  fullMsg += "\n🌡️ Temp: " + String(sensorData.tempAvg, 1) + "°C";
  sendTelegramMessage(fullMsg);
}

// ============================================
// PROBAR CONEXIÓN TELEGRAM
// ============================================
bool testTelegram() {
  if (!state.internetAvailable) return false;
  
  String msg = "✅ *Test de conexión*\n\n";
  msg += "📍 " + String(LOCATION_DETAIL) + "\n";
  msg += "🌡️ Temperatura: " + String(sensorData.tempAvg, 1) + "°C\n";
  msg += "📶 WiFi: " + String(WiFi.RSSI()) + " dBm\n";
  msg += "🔗 IP: " + state.localIP;
  
  unsigned long savedTime = state.lastTelegramAlert;
  state.lastTelegramAlert = 0;
  sendTelegramMessage(msg);
  state.lastTelegramAlert = savedTime;
  
  return true;
}

#endif
