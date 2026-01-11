/*
 * serial_api.h - API de comandos por Serial/COM, Web y App
 * Sistema Monitoreo Reefer v3.0
 * 
 * NOTA: Este módulo NO está integrado aún. Solo definiciones.
 * 
 * Permite controlar el ESP32 mediante:
 * 1. Puerto Serial (COM) - Para debug y configuración local
 * 2. API Web - Endpoints HTTP para control remoto
 * 3. App Android - Mismos endpoints que la web
 * 
 * COMANDOS DISPONIBLES:
 * - RESTART     : Reiniciar el ESP32
 * - RESET_WIFI  : Borrar configuración WiFi y reiniciar en modo AP
 * - RESET_CONFIG: Restaurar configuración de fábrica
 * - STATUS      : Obtener estado del sistema
 * - SET_TEMP    : Configurar temperatura crítica
 * - DEFROST_ON  : Activar modo descongelamiento
 * - DEFROST_OFF : Desactivar modo descongelamiento
 * - ALERT_ACK   : Silenciar alerta activa
 * - RELAY_ON    : Encender relay manualmente
 * - RELAY_OFF   : Apagar relay manualmente
 * - SUPABASE_ON : Habilitar Supabase
 * - SUPABASE_OFF: Deshabilitar Supabase
 * - HELP        : Mostrar comandos disponibles
 */

#ifndef SERIAL_API_H
#define SERIAL_API_H

#include <WebServer.h>

// Forward declarations
extern WebServer server;
extern Config config;
extern SystemState state;
extern void saveConfig();
extern void setRelay(bool on);
extern void acknowledgeAlert();
extern void clearAlert();

// ============================================
// CONFIGURACIÓN
// ============================================
#define SERIAL_API_BAUD 115200
#define SERIAL_BUFFER_SIZE 128

// ============================================
// VARIABLES
// ============================================
char serialBuffer[SERIAL_BUFFER_SIZE];
int serialBufferIndex = 0;

// ============================================
// PROCESAR COMANDO
// ============================================
String processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  
  Serial.printf("[CMD] Recibido: %s\n", cmd.c_str());
  
  // RESTART - Reiniciar ESP32
  if (cmd == "RESTART" || cmd == "REBOOT") {
    Serial.println("[CMD] Reiniciando en 2 segundos...");
    delay(2000);
    ESP.restart();
    return "OK: Reiniciando...";
  }
  
  // RESET_WIFI - Borrar WiFi y reiniciar en modo AP
  if (cmd == "RESET_WIFI" || cmd == "WIFI_RESET") {
    Serial.println("[CMD] Borrando configuración WiFi...");
    WiFi.disconnect(true, true);
    delay(1000);
    ESP.restart();
    return "OK: WiFi reseteado, reiniciando en modo AP...";
  }
  
  // RESET_CONFIG - Restaurar configuración de fábrica
  if (cmd == "RESET_CONFIG" || cmd == "FACTORY_RESET") {
    Serial.println("[CMD] Restaurando configuración de fábrica...");
    
    // Valores por defecto
    config.tempMax = -15.0;
    config.tempCritical = -10.0;
    config.alertDelaySec = 300;
    config.doorOpenMaxSec = 120;
    config.defrostCooldownSec = 1800;
    config.defrostRelayNC = false;
    config.relayEnabled = true;
    config.buzzerEnabled = true;
    config.telegramEnabled = true;
    config.supabaseEnabled = false;
    config.sensor1Enabled = true;
    config.sensor2Enabled = false;
    config.dht22Enabled = false;
    config.doorEnabled = true;
    config.simulationMode = false;
    
    saveConfig();
    return "OK: Configuración restaurada a valores de fábrica";
  }
  
  // STATUS - Estado del sistema
  if (cmd == "STATUS") {
    String status = "\n=== ESTADO DEL SISTEMA ===\n";
    status += "Temperatura: " + String(sensorData.tempAvg, 1) + "°C\n";
    status += "Temp Crítica: " + String(config.tempCritical, 1) + "°C\n";
    status += "Alerta: " + String(state.alertActive ? "ACTIVA" : "Normal") + "\n";
    status += "Relay: " + String(state.relayState ? "ON" : "OFF") + "\n";
    status += "Defrost: " + String(state.defrostMode ? "ACTIVO" : "OFF") + "\n";
    status += "WiFi: " + String(state.wifiConnected ? "Conectado" : "Desconectado") + "\n";
    status += "Internet: " + String(state.internetAvailable ? "OK" : "Sin conexión") + "\n";
    status += "Supabase: " + String(config.supabaseEnabled ? "Habilitado" : "Deshabilitado") + "\n";
    status += "IP: " + state.localIP + "\n";
    status += "Uptime: " + String((millis() - state.uptime) / 60000) + " min\n";
    return status;
  }
  
  // SET_TEMP <valor> - Configurar temperatura crítica
  if (cmd.startsWith("SET_TEMP ")) {
    float newTemp = cmd.substring(9).toFloat();
    if (newTemp < -50 || newTemp > 50) {
      return "ERROR: Temperatura fuera de rango (-50 a 50)";
    }
    config.tempCritical = newTemp;
    saveConfig();
    return "OK: Temperatura crítica = " + String(newTemp, 1) + "°C";
  }
  
  // DEFROST_ON - Activar descongelamiento
  if (cmd == "DEFROST_ON") {
    state.defrostMode = true;
    state.defrostStartTime = millis();
    state.alertActive = false;
    setRelay(false);
    return "OK: Modo descongelamiento ACTIVADO";
  }
  
  // DEFROST_OFF - Desactivar descongelamiento
  if (cmd == "DEFROST_OFF") {
    state.defrostMode = false;
    state.defrostStartTime = 0;
    return "OK: Modo descongelamiento DESACTIVADO";
  }
  
  // ALERT_ACK - Silenciar alerta
  if (cmd == "ALERT_ACK" || cmd == "ACK") {
    acknowledgeAlert();
    return "OK: Alerta silenciada";
  }
  
  // ALERT_CLEAR - Limpiar alerta
  if (cmd == "ALERT_CLEAR" || cmd == "CLEAR") {
    clearAlert();
    return "OK: Alerta limpiada";
  }
  
  // RELAY_ON - Encender relay
  if (cmd == "RELAY_ON") {
    setRelay(true);
    return "OK: Relay ENCENDIDO";
  }
  
  // RELAY_OFF - Apagar relay
  if (cmd == "RELAY_OFF") {
    setRelay(false);
    return "OK: Relay APAGADO";
  }
  
  // SUPABASE_ON - Habilitar Supabase
  if (cmd == "SUPABASE_ON") {
    config.supabaseEnabled = true;
    saveConfig();
    return "OK: Supabase HABILITADO";
  }
  
  // SUPABASE_OFF - Deshabilitar Supabase
  if (cmd == "SUPABASE_OFF") {
    config.supabaseEnabled = false;
    saveConfig();
    return "OK: Supabase DESHABILITADO";
  }
  
  // TELEGRAM_ON / TELEGRAM_OFF
  if (cmd == "TELEGRAM_ON") {
    config.telegramEnabled = true;
    saveConfig();
    return "OK: Telegram HABILITADO";
  }
  if (cmd == "TELEGRAM_OFF") {
    config.telegramEnabled = false;
    saveConfig();
    return "OK: Telegram DESHABILITADO";
  }
  
  // SIMULATION_ON / SIMULATION_OFF
  if (cmd == "SIMULATION_ON" || cmd == "SIM_ON") {
    config.simulationMode = true;
    saveConfig();
    return "OK: Modo simulación ACTIVADO";
  }
  if (cmd == "SIMULATION_OFF" || cmd == "SIM_OFF") {
    config.simulationMode = false;
    saveConfig();
    return "OK: Modo simulación DESACTIVADO";
  }
  
  // SET_SIM_TEMP <temp1> <temp2> - Configurar temperaturas simuladas
  if (cmd.startsWith("SET_SIM_TEMP ")) {
    int spacePos = cmd.indexOf(' ', 13);
    if (spacePos > 0) {
      config.simTemp1 = cmd.substring(13, spacePos).toFloat();
      config.simTemp2 = cmd.substring(spacePos + 1).toFloat();
    } else {
      config.simTemp1 = cmd.substring(13).toFloat();
      config.simTemp2 = config.simTemp1;
    }
    saveConfig();
    return "OK: Temp simulada = " + String(config.simTemp1, 1) + "°C / " + String(config.simTemp2, 1) + "°C";
  }
  
  // HELP - Mostrar ayuda
  if (cmd == "HELP" || cmd == "?") {
    String help = "\n=== COMANDOS DISPONIBLES ===\n";
    help += "RESTART        - Reiniciar ESP32\n";
    help += "RESET_WIFI     - Borrar WiFi, modo AP\n";
    help += "RESET_CONFIG   - Config de fábrica\n";
    help += "STATUS         - Estado del sistema\n";
    help += "SET_TEMP <°C>  - Temp crítica\n";
    help += "DEFROST_ON/OFF - Descongelamiento\n";
    help += "ALERT_ACK      - Silenciar alerta\n";
    help += "RELAY_ON/OFF   - Control relay\n";
    help += "SUPABASE_ON/OFF- Habilitar Supabase\n";
    help += "TELEGRAM_ON/OFF- Habilitar Telegram\n";
    help += "SIM_ON/OFF     - Modo simulación\n";
    help += "SET_SIM_TEMP   - Temp simulada\n";
    help += "HELP           - Esta ayuda\n";
    return help;
  }
  
  return "ERROR: Comando no reconocido. Escribe HELP para ver comandos.";
}

// ============================================
// LEER COMANDOS DEL SERIAL
// ============================================
void serialApiLoop() {
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (serialBufferIndex > 0) {
        serialBuffer[serialBufferIndex] = '\0';
        String response = processCommand(String(serialBuffer));
        Serial.println(response);
        serialBufferIndex = 0;
      }
    } else if (serialBufferIndex < SERIAL_BUFFER_SIZE - 1) {
      serialBuffer[serialBufferIndex++] = c;
    }
  }
}

// ============================================
// HANDLER WEB: /api/command
// ============================================
void handleApiCommand() {
  String cmd = "";
  
  // Obtener comando de query string o body
  if (server.hasArg("cmd")) {
    cmd = server.arg("cmd");
  } else if (server.hasArg("plain")) {
    cmd = server.arg("plain");
  }
  
  if (cmd.length() == 0) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "application/json", "{\"error\":\"No command provided\"}");
    return;
  }
  
  String response = processCommand(cmd);
  
  // Formatear respuesta como JSON
  String json = "{";
  json += "\"command\":\"" + cmd + "\",";
  json += "\"response\":\"" + response + "\",";
  json += "\"success\":" + String(response.startsWith("OK") ? "true" : "false");
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// ============================================
// HANDLER WEB: /api/restart
// ============================================
void handleApiRestart() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Reiniciando en 2 segundos...\"}");
  delay(2000);
  ESP.restart();
}

// ============================================
// HANDLER WEB: /api/factory_reset
// ============================================
void handleApiFactoryReset() {
  processCommand("RESET_CONFIG");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Configuración restaurada\"}");
}

// ============================================
// CONFIGURAR RUTAS WEB PARA COMANDOS
// ============================================
void setupSerialApiRoutes() {
  // Endpoint genérico para comandos
  server.on("/api/command", HTTP_GET, handleApiCommand);
  server.on("/api/command", HTTP_POST, handleApiCommand);
  
  // Endpoints específicos
  server.on("/api/restart", HTTP_POST, handleApiRestart);
  server.on("/api/factory_reset", HTTP_POST, handleApiFactoryReset);
  
  Serial.println("[SERIAL_API] Rutas configuradas");
}

// ============================================
// INICIALIZACIÓN
// ============================================
void serialApiInit() {
  serialBufferIndex = 0;
  Serial.println("[SERIAL_API] API de comandos inicializada");
  Serial.println("[SERIAL_API] Escribe HELP para ver comandos disponibles");
}

#endif
