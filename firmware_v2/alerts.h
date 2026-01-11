/*
 * ============================================================================
 * ALERTS.H - LÓGICA DE ALERTAS v4.0
 * Sistema Monitoreo Reefer Industrial
 * ============================================================================
 * 
 * Integrado con la máquina de estados:
 * - Solo verifica alertas en estados NORMAL o ALERT
 * - Suspende alertas durante DEFROST y COOLDOWN
 * - Reporta a Supabase y Telegram
 * 
 * ============================================================================
 */

#ifndef ALERTS_H
#define ALERTS_H

#include "config.h"
#include "types.h"

extern Config config;
extern SensorData sensorData;
extern SystemState state;

extern void setRelay(int relayIndex, bool on);
extern void sendTelegramAlert(String message);
extern void sendAlertToSupabase(String alertType, String severity, String message);
extern void changeState(SystemStateEnum newState, const char* reason);

// Variables de tracking de alertas
static unsigned long highTempAccumulatedSec = 0;
static unsigned long lastAlertCheckTime = 0;
static unsigned long doorOpenAlertSent[MAX_DOOR_SENSORS] = {0};

// ============================================================================
// ACTIVAR ALERTA
// ============================================================================
void triggerAlert(String message, bool critical = true) {
    if (state.alertActive) return;
    
    // Cambiar a estado ALERT
    changeState(STATE_ALERT, message.c_str());
    
    state.alertActive = true;
    state.alertCritical = critical;
    state.alertMessage = message;
    state.alertStartTime = millis();
    state.totalAlerts++;
    state.alertAcknowledged = false;
    
    Serial.println("🚨 ════════════════════════════════════════");
    Serial.println("🚨 [ALERTA] " + message);
    Serial.println("🚨 ════════════════════════════════════════");
    
    // Activar sirena (relé 1)
    if (config.relayEnabled) {
        setRelay(0, true);
    }
    
    // Activar buzzer
    if (config.buzzerEnabled) {
        digitalWrite(PIN_BUZZER, HIGH);
    }
    
    // Enviar a Telegram (con rate limit de 5 minutos)
    if (config.telegramEnabled && state.internetAvailable) {
        unsigned long now = millis();
        if (now - state.lastTelegramAlert > 300000) {
            String telegramMsg = "🚨 *ALERTA " + String(critical ? "CRÍTICA" : "WARNING") + "*\n\n";
            telegramMsg += "📍 *Dispositivo:* " + String(DEVICE_ID) + "\n";
            telegramMsg += "📝 " + message + "\n";
            telegramMsg += "🌡️ *Temp actual:* " + String(sensorData.tempAvg, 1) + "°C";
            
            sendTelegramAlert(telegramMsg);
            state.lastTelegramAlert = now;
        }
    }
    
    // Enviar a Supabase
    if (config.supabaseEnabled && state.internetAvailable) {
        sendAlertToSupabase("temperature", critical ? "critical" : "warning", message);
    }
}

// ============================================================================
// DESACTIVAR ALERTA
// ============================================================================
void clearAlert() {
    if (!state.alertActive) return;
    
    state.alertActive = false;
    state.alertCritical = false;
    state.alertMessage = "";
    state.alertAcknowledged = false;
    
    // Apagar sirena y buzzer
    setRelay(0, false);
    digitalWrite(PIN_BUZZER, LOW);
    
    // Volver a estado NORMAL
    changeState(STATE_NORMAL, "Alerta resuelta");
    
    Serial.println("✅ [ALERTA] Alerta desactivada - Condiciones normalizadas");
}

// ============================================================================
// RECONOCER ALERTA (silenciar sin resolver)
// ============================================================================
void acknowledgeAlert() {
    if (!state.alertActive) return;
    
    state.alertAcknowledged = true;
    
    // Apagar sirena y buzzer pero mantener estado de alerta
    setRelay(0, false);
    digitalWrite(PIN_BUZZER, LOW);
    
    Serial.println("🔕 [ALERTA] Alerta reconocida - Sirena silenciada");
}

// ============================================================================
// VERIFICAR ALERTAS DE TEMPERATURA
// ============================================================================
void checkTemperatureAlerts() {
    if (!sensorData.tempValid) return;
    
    float temp = sensorData.tempAvg;
    unsigned long now = millis();
    
    // Calcular tiempo transcurrido desde última verificación
    unsigned long elapsed = 0;
    if (lastAlertCheckTime > 0) {
        elapsed = (now - lastAlertCheckTime) / 1000;
    }
    lastAlertCheckTime = now;
    
    // Verificar si temperatura está por encima del umbral crítico
    if (temp > config.tempCritical) {
        // Acumular tiempo en temperatura alta
        highTempAccumulatedSec += elapsed;
        
        // Si superó el delay y no hay alerta activa ni reconocida
        if (highTempAccumulatedSec >= (unsigned long)config.alertDelaySec && 
            !state.alertActive && 
            !state.alertAcknowledged) {
            
            String msg = "🌡️ Temperatura CRÍTICA: " + String(temp, 1) + "°C";
            msg += " (límite: " + String(config.tempCritical, 1) + "°C)";
            msg += " - Sostenida por " + String(highTempAccumulatedSec / 60) + " min";
            
            triggerAlert(msg, true);
        }
    } else {
        // Temperatura OK - resetear acumulador
        highTempAccumulatedSec = 0;
        state.alertAcknowledged = false;
        
        // Si había alerta activa, desactivarla
        if (state.alertActive && state.currentState == STATE_ALERT) {
            clearAlert();
        }
    }
}

// ============================================================================
// VERIFICAR ALERTAS DE PUERTAS
// ============================================================================
void checkDoorAlerts() {
    unsigned long now = millis();
    
    for (int i = 0; i < MAX_DOOR_SENSORS; i++) {
        if (!sensorData.door[i].enabled) continue;
        
        if (sensorData.door[i].isOpen && sensorData.door[i].openSince > 0) {
            unsigned long openDuration = (now - sensorData.door[i].openSince) / 1000;
            
            // Si superó el tiempo máximo y no se envió alerta recientemente
            if (openDuration >= (unsigned long)config.doorOpenMaxSec) {
                // Rate limit: una alerta por puerta cada 10 minutos
                if (now - doorOpenAlertSent[i] > 600000) {
                    doorOpenAlertSent[i] = now;
                    
                    String msg = "🚪 Puerta " + String(sensorData.door[i].name);
                    msg += " abierta por " + String(openDuration / 60) + " minutos";
                    msg += " (máximo: " + String(config.doorOpenMaxSec / 60) + " min)";
                    
                    // Enviar a Telegram
                    if (config.telegramEnabled && state.internetAvailable) {
                        sendTelegramAlert("⚠️ *ALERTA PUERTA*\n\n" + msg);
                    }
                    
                    // Enviar a Supabase
                    if (config.supabaseEnabled && state.internetAvailable) {
                        sendAlertToSupabase("door", "warning", msg);
                    }
                    
                    Serial.println("⚠️ [PUERTA] " + msg);
                }
            }
        } else {
            // Puerta cerrada - resetear timer de alerta
            doorOpenAlertSent[i] = 0;
        }
    }
}

// ============================================================================
// VERIFICAR TODAS LAS ALERTAS (llamar desde loop)
// ============================================================================
void checkAlerts() {
    // Solo verificar alertas si estamos en estado de monitoreo
    if (!isStateMonitoring(state.currentState)) {
        // Log periódico cuando estamos en estado suspendido
        static unsigned long lastSuspendedLog = 0;
        if (millis() - lastSuspendedLog > 30000) {
            lastSuspendedLog = millis();
            Serial.printf("[ALERTS] Monitoreo suspendido - Estado: %s\n", state.stateName);
            
            if (state.currentState == STATE_COOLDOWN) {
                Serial.printf("[ALERTS] Cooldown restante: %d segundos\n", 
                              state.cooldownRemainingSeconds);
            }
        }
        return;
    }
    
    // Verificar alertas de temperatura
    checkTemperatureAlerts();
    
    // Verificar alertas de puertas
    checkDoorAlerts();
}

// ============================================================================
// OBTENER JSON DE ALERTAS
// ============================================================================
void getAlertsJSON(JsonObject& obj) {
    obj["active"] = state.alertActive;
    obj["critical"] = state.alertCritical;
    obj["acknowledged"] = state.alertAcknowledged;
    obj["message"] = state.alertMessage;
    obj["total_alerts"] = state.totalAlerts;
    
    if (state.alertActive) {
        obj["alert_duration_sec"] = (millis() - state.alertStartTime) / 1000;
    }
    
    obj["high_temp_accumulated_sec"] = highTempAccumulatedSec;
    obj["alert_threshold_sec"] = config.alertDelaySec;
}

#endif // ALERTS_H
