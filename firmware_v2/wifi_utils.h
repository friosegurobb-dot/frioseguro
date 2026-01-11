/*
 * wifi_utils.h - Gestión de WiFi y conectividad
 * Sistema Monitoreo Reefer v3.0
 */

#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include "config.h"
#include "types.h"

extern WiFiManager wifiManager;
extern SystemState state;

// ============================================
// CONECTAR WIFI
// ============================================
void connectWiFi() {
  Serial.println("\n[WIFI] Iniciando WiFiManager...");
  
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setConnectTimeout(30);
  
  String apName = String(DEVICE_NAME) + "_Setup";
  
  if (!wifiManager.autoConnect(apName.c_str(), "reefer123")) {
    Serial.println("[WIFI] ✗ Falló conexión - Modo AP activo");
    state.apMode = true;
    state.wifiConnected = false;
  } else {
    Serial.println("[WIFI] ✓ Conectado!");
    state.wifiConnected = true;
    state.apMode = false;
    state.localIP = WiFi.localIP().toString();
    Serial.printf("[WIFI] IP: %s\n", state.localIP.c_str());
  }
}

// ============================================
// CONFIGURAR mDNS
// ============================================
void setupMDNS() {
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[mDNS] ✓ http://%s.local\n", MDNS_NAME);
  } else {
    Serial.println("[mDNS] ✗ Error");
  }
}

// ============================================
// VERIFICAR CONEXIÓN A INTERNET
// ============================================
void checkInternet() {
  if (millis() - state.lastInternetCheck < 30000) return;
  state.lastInternetCheck = millis();
  
  if (!state.wifiConnected) {
    state.internetAvailable = false;
    return;
  }
  
  HTTPClient http;
  http.setTimeout(5000);
  http.begin("http://www.google.com/generate_204");
  int httpCode = http.GET();
  http.end();
  
  state.internetAvailable = (httpCode == 204 || httpCode == 200);
  
  static bool lastState = false;
  if (state.internetAvailable != lastState) {
    Serial.printf("[INTERNET] %s\n", state.internetAvailable ? "✓ Online" : "✗ Offline");
    lastState = state.internetAvailable;
  }
}

// ============================================
// RECONECTAR WIFI
// ============================================
void reconnectWiFi() {
  static unsigned long lastReconnect = 0;
  if (millis() - lastReconnect < 30000) return;
  lastReconnect = millis();
  
  Serial.println("[WIFI] Intentando reconectar...");
  WiFi.reconnect();
}

// ============================================
// RESETEAR WIFI
// ============================================
void resetWiFi() {
  Serial.println("[WIFI] Reseteando configuración...");
  wifiManager.resetSettings();
  delay(1000);
  ESP.restart();
}

#endif
