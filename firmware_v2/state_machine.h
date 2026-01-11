/*
 * ============================================================================
 * STATE_MACHINE.H - MÁQUINA DE ESTADOS v4.0
 * Sistema Monitoreo Reefer Industrial
 * ============================================================================
 * 
 * Estados del sistema:
 * - NORMAL: Operación normal, monitoreando temperatura
 * - DEFROST: En ciclo de descongelamiento (alertas suspendidas)
 * - COOLDOWN: Esperando post-descongelamiento con timer real
 * - LOADING_CONFIG: Aplicando nueva configuración
 * - ALERT: Alerta activa
 * - OFFLINE: Sin conexión WiFi
 * - INITIALIZING: Arrancando sistema
 * 
 * Todas las operaciones son NO BLOQUEANTES usando millis()
 * 
 * ============================================================================
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "config.h"
#include "types.h"

// Referencias externas
extern SystemState state;
extern SensorData sensorData;
extern Config config;

// Forward declarations
extern void setRelay(bool on);
extern void sendTelegramAlert(String message);
extern void supabaseSendDefrostStart(float tempAtStart, const char* triggeredBy);
extern void supabaseSendDefrostEnd(float tempAtEnd, unsigned long durationMin);

// ============================================================================
// TIMERS NO BLOQUEANTES GLOBALES
// ============================================================================
NonBlockingTimer cooldownTimer;
NonBlockingTimer configLoadTimer;
NonBlockingTimer defrostMaxTimer;

// ============================================================================
// CAMBIO DE ESTADO
// ============================================================================
void changeState(SystemStateEnum newState, const char* reason = "") {
    if (state.currentState == newState) return;
    
    state.previousState = state.currentState;
    state.currentState = newState;
    state.stateChangedAt = millis();
    state.stateName = getStateName(newState);
    
    Serial.printf("\n[STATE] ═══════════════════════════════════════\n");
    Serial.printf("[STATE] %s → %s\n", 
                  getStateName(state.previousState), 
                  getStateName(newState));
    if (strlen(reason) > 0) {
        Serial.printf("[STATE] Razón: %s\n", reason);
    }
    Serial.printf("[STATE] ═══════════════════════════════════════\n\n");
}

// ============================================================================
// INICIALIZACIÓN DE LA MÁQUINA DE ESTADOS
// ============================================================================
void initStateMachine() {
    state.currentState = STATE_INITIALIZING;
    state.previousState = STATE_INITIALIZING;
    state.stateChangedAt = millis();
    state.stateName = getStateName(STATE_INITIALIZING);
    
    state.defrostStartTime = 0;
    state.cooldownStartTime = 0;
    state.cooldownEndTime = 0;
    state.cooldownRemainingSeconds = 0;
    state.configLoadStartTime = 0;
    state.configLoadEndTime = 0;
    state.configLoadRemainingSeconds = 0;
    
    Serial.println("[STATE] Máquina de estados inicializada");
}

// ============================================================================
// ENTRAR EN MODO DESCONGELAMIENTO
// ============================================================================
void enterDefrostMode(const char* triggeredBy) {
    if (state.currentState == STATE_DEFROST) return;
    
    changeState(STATE_DEFROST, triggeredBy);
    
    state.defrostStartTime = millis();
    
    // Suspender alertas durante defrost
    state.alertActive = false;
    state.alertCritical = false;
    state.alertAcknowledged = false;
    state.alertMessage = "";
    
    // Apagar sirena si estaba encendida
    setRelay(false);
    
    // Timer de seguridad: máximo tiempo en defrost
    defrostMaxTimer.start(config.defrostMaxDurationSec * 1000UL);
    
    // Registrar en Supabase
    supabaseSendDefrostStart(sensorData.tempAvg, triggeredBy);
    
    Serial.printf("[DEFROST] ⚡ INICIADO - Temp actual: %.1f°C\n", sensorData.tempAvg);
    Serial.printf("[DEFROST] Máximo permitido: %d minutos\n", config.defrostMaxDurationSec / 60);
}

// ============================================================================
// SALIR DE MODO DESCONGELAMIENTO → ENTRAR EN COOLDOWN
// ============================================================================
void exitDefrostMode() {
    if (state.currentState != STATE_DEFROST) return;
    
    unsigned long defrostDuration = millis() - state.defrostStartTime;
    unsigned long defrostMinutes = defrostDuration / 60000;
    
    // Registrar fin de defrost en Supabase
    supabaseSendDefrostEnd(sensorData.tempAvg, defrostMinutes);
    
    Serial.printf("[DEFROST] ✓ FINALIZADO - Duró %lu minutos\n", defrostMinutes);
    Serial.printf("[DEFROST] Temp al finalizar: %.1f°C\n", sensorData.tempAvg);
    
    // Iniciar período de cooldown
    enterCooldownMode();
}

// ============================================================================
// ENTRAR EN MODO COOLDOWN (POST-DESCONGELAMIENTO)
// ============================================================================
void enterCooldownMode() {
    changeState(STATE_COOLDOWN, "Post-descongelamiento");
    
    state.cooldownStartTime = millis();
    state.cooldownEndTime = state.cooldownStartTime + (config.defrostCooldownSec * 1000UL);
    state.cooldownRemainingSeconds = config.defrostCooldownSec;
    
    // Iniciar timer de cooldown
    cooldownTimer.start(config.defrostCooldownSec * 1000UL);
    
    Serial.printf("[COOLDOWN] ⏳ INICIADO - Esperando %d minutos\n", config.defrostCooldownSec / 60);
    Serial.printf("[COOLDOWN] El sistema se enfriará antes de reactivar monitoreo\n");
}

// ============================================================================
// SALIR DE MODO COOLDOWN → VOLVER A NORMAL
// ============================================================================
void exitCooldownMode() {
    if (state.currentState != STATE_COOLDOWN) return;
    
    cooldownTimer.stop();
    state.cooldownRemainingSeconds = 0;
    
    changeState(STATE_NORMAL, "Cooldown completado");
    
    Serial.println("[COOLDOWN] ✓ COMPLETADO - Volviendo a monitoreo normal");
}

// ============================================================================
// ENTRAR EN MODO CARGANDO CONFIGURACIÓN
// ============================================================================
void enterLoadingConfigMode() {
    changeState(STATE_LOADING_CONFIG, "Aplicando nueva configuración");
    
    state.configLoadStartTime = millis();
    state.configLoadEndTime = state.configLoadStartTime + (config.configApplyTimeSec * 1000UL);
    state.configLoadRemainingSeconds = config.configApplyTimeSec;
    
    // Iniciar timer
    configLoadTimer.start(config.configApplyTimeSec * 1000UL);
    
    Serial.printf("[CONFIG] ⚙️ APLICANDO - Esperando %d segundos\n", config.configApplyTimeSec);
}

// ============================================================================
// SALIR DE MODO CARGANDO CONFIGURACIÓN
// ============================================================================
void exitLoadingConfigMode() {
    if (state.currentState != STATE_LOADING_CONFIG) return;
    
    configLoadTimer.stop();
    state.configLoadRemainingSeconds = 0;
    
    changeState(STATE_NORMAL, "Configuración aplicada");
    
    Serial.println("[CONFIG] ✓ CONFIGURACIÓN APLICADA");
}

// ============================================================================
// ACTUALIZAR TIMERS DE ESTADO (llamar en cada loop)
// ============================================================================
void updateStateTimers() {
    // Actualizar segundos restantes de cooldown
    if (state.currentState == STATE_COOLDOWN) {
        state.cooldownRemainingSeconds = cooldownTimer.remainingSeconds();
        
        // Log cada 30 segundos
        static unsigned long lastCooldownLog = 0;
        if (millis() - lastCooldownLog > 30000) {
            lastCooldownLog = millis();
            Serial.printf("[COOLDOWN] ⏳ Restante: %d segundos (%.1f min)\n", 
                          state.cooldownRemainingSeconds,
                          state.cooldownRemainingSeconds / 60.0);
        }
    }
    
    // Actualizar segundos restantes de carga de config
    if (state.currentState == STATE_LOADING_CONFIG) {
        state.configLoadRemainingSeconds = configLoadTimer.remainingSeconds();
    }
}

// ============================================================================
// VERIFICAR TRANSICIONES DE ESTADO (llamar en cada loop)
// ============================================================================
void checkStateTransitions() {
    switch (state.currentState) {
        case STATE_INITIALIZING:
            // Transición automática a NORMAL después de inicialización
            // (se hace manualmente desde setup())
            break;
            
        case STATE_NORMAL:
            // Verificar si hay señal de defrost
            if (sensorData.defrostSignalActive) {
                enterDefrostMode("relay_signal");
            }
            break;
            
        case STATE_DEFROST:
            // Verificar si terminó el defrost (señal desactivada)
            if (!sensorData.defrostSignalActive) {
                exitDefrostMode();
            }
            // Verificar timeout de seguridad
            else if (defrostMaxTimer.check()) {
                Serial.println("[DEFROST] ⚠️ TIMEOUT - Forzando salida de defrost");
                exitDefrostMode();
            }
            break;
            
        case STATE_COOLDOWN:
            // Verificar si terminó el cooldown
            if (cooldownTimer.check()) {
                exitCooldownMode();
            }
            break;
            
        case STATE_LOADING_CONFIG:
            // Verificar si terminó la carga de config
            if (configLoadTimer.check()) {
                exitLoadingConfigMode();
            }
            break;
            
        case STATE_ALERT:
            // Las alertas se manejan en alerts.h
            break;
            
        case STATE_OFFLINE:
            // Verificar si volvió la conexión
            if (state.wifiConnected && state.internetAvailable) {
                changeState(STATE_NORMAL, "Conexión restaurada");
            }
            break;
    }
}

// ============================================================================
// VERIFICAR SEÑAL DE DEFROST DEL REEFER
// ============================================================================
void checkDefrostSignal() {
    static bool lastSignal = false;
    static unsigned long debounceStart = 0;
    
    // Leer pin
    bool pinState = digitalRead(PIN_DEFROST_INPUT);
    
    // Determinar si está en defrost según configuración NC/NA
    bool defrostDetected;
    if (config.defrostRelayNC) {
        // NC: HIGH = defrost
        defrostDetected = (pinState == HIGH);
    } else {
        // NA: LOW = defrost
        defrostDetected = (pinState == LOW);
    }
    
    // Debounce de 2 segundos
    if (defrostDetected != lastSignal) {
        if (debounceStart == 0) {
            debounceStart = millis();
        } else if (millis() - debounceStart > 2000) {
            lastSignal = defrostDetected;
            debounceStart = 0;
            sensorData.defrostSignalActive = defrostDetected;
        }
    } else {
        debounceStart = 0;
    }
}

// ============================================================================
// OBTENER JSON DEL ESTADO ACTUAL
// ============================================================================
void getStateJSON(JsonObject& obj) {
    obj["state"] = state.stateName;
    obj["state_code"] = (int)state.currentState;
    obj["state_since_sec"] = (millis() - state.stateChangedAt) / 1000;
    
    // Información específica según estado
    switch (state.currentState) {
        case STATE_DEFROST:
            obj["defrost_minutes"] = (millis() - state.defrostStartTime) / 60000;
            obj["defrost_max_minutes"] = config.defrostMaxDurationSec / 60;
            break;
            
        case STATE_COOLDOWN:
            obj["cooldown_remaining_sec"] = state.cooldownRemainingSeconds;
            obj["cooldown_remaining_min"] = state.cooldownRemainingSeconds / 60;
            obj["cooldown_total_sec"] = config.defrostCooldownSec;
            break;
            
        case STATE_LOADING_CONFIG:
            obj["config_remaining_sec"] = state.configLoadRemainingSeconds;
            break;
            
        case STATE_ALERT:
            obj["alert_message"] = state.alertMessage;
            obj["alert_critical"] = state.alertCritical;
            obj["alert_acknowledged"] = state.alertAcknowledged;
            break;
    }
}

// ============================================================================
// LOOP PRINCIPAL DE LA MÁQUINA DE ESTADOS
// ============================================================================
void stateMachineLoop() {
    // Verificar señal de defrost
    static unsigned long lastDefrostCheck = 0;
    if (millis() - lastDefrostCheck >= INTERVAL_DEFROST_CHECK_MS) {
        lastDefrostCheck = millis();
        checkDefrostSignal();
    }
    
    // Actualizar timers
    updateStateTimers();
    
    // Verificar transiciones
    checkStateTransitions();
}

#endif // STATE_MACHINE_H
