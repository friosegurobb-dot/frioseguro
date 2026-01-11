# Firmware v4.0 - Sistema Monitoreo Reefer Industrial

## Arquitectura

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           FIRMWARE v4.0                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐                  │
│  │  config.h    │    │   types.h    │    │state_machine │                  │
│  │              │    │              │    │     .h       │                  │
│  │ - Device ID  │    │ - Structs    │    │ - Estados    │                  │
│  │ - Pines      │    │ - Enums      │    │ - Timers     │                  │
│  │ - Umbrales   │    │ - Helpers    │    │ - Transic.   │                  │
│  └──────────────┘    └──────────────┘    └──────────────┘                  │
│         │                   │                   │                           │
│         └───────────────────┴───────────────────┘                           │
│                             │                                               │
│                    ┌────────┴────────┐                                      │
│                    │ firmware_v2.ino │                                      │
│                    │   (Principal)   │                                      │
│                    └────────┬────────┘                                      │
│                             │                                               │
│         ┌───────────────────┼───────────────────┐                          │
│         │                   │                   │                           │
│  ┌──────┴──────┐    ┌──────┴──────┐    ┌──────┴──────┐                    │
│  │  sensors.h  │    │  alerts.h   │    │ supabase.h  │                    │
│  │             │    │             │    │             │                    │
│  │ - DS18B20   │    │ - Lógica    │    │ - Sync      │                    │
│  │ - Puertas   │    │ - Trigger   │    │ - Readings  │                    │
│  │ - DHT22     │    │ - Clear     │    │ - Alerts    │                    │
│  └─────────────┘    └─────────────┘    └─────────────┘                    │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Estados del Sistema

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        MÁQUINA DE ESTADOS                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌──────────────┐                                                          │
│   │ INITIALIZING │ ──────────────────────────────────────┐                  │
│   └──────────────┘                                       │                  │
│          │                                               │                  │
│          │ Setup completo                                │                  │
│          ▼                                               │                  │
│   ┌──────────────┐      Señal defrost      ┌───────────────┐               │
│   │    NORMAL    │ ──────────────────────► │    DEFROST    │               │
│   │              │                          │               │               │
│   │ Monitoreando │ ◄────────────────────── │ Alertas OFF   │               │
│   │ temperatura  │      Señal termina      │ Timer máximo  │               │
│   └──────────────┘                          └───────┬───────┘               │
│          │                                          │                       │
│          │ Temp > crítica                           │ Señal termina         │
│          │ por X segundos                           ▼                       │
│          ▼                                  ┌───────────────┐               │
│   ┌──────────────┐                          │   COOLDOWN    │               │
│   │    ALERT     │                          │               │               │
│   │              │                          │ Timer real    │               │
│   │ Sirena ON    │                          │ 30 min        │               │
│   │ Telegram     │                          │ Reporta resto │               │
│   └──────────────┘                          └───────┬───────┘               │
│          │                                          │                       │
│          │ Temp OK                                  │ Timer expira          │
│          │                                          │                       │
│          └──────────────────────────────────────────┴───────► NORMAL        │
│                                                                              │
│   ┌──────────────┐                                                          │
│   │LOADING_CONFIG│  Guardar config → Timer 10 seg → NORMAL                  │
│   └──────────────┘                                                          │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Mapa de Pines ESP32-WROOM-32 (38 pines)

### Pines Utilizados

| GPIO | Función | Descripción |
|------|---------|-------------|
| 4 | PIN_ONEWIRE | Bus 1-Wire para DS18B20 (hasta 6 sondas) |
| 5 | PIN_DOOR_1 | Sensor puerta principal |
| 16 | PIN_DOOR_3 | Sensor puerta trasera |
| 17 | PIN_DOOR_2 | Sensor puerta lateral |
| 18 | PIN_DHT22 | Sensor DHT22 (ambiente) |
| 19 | PIN_BUZZER | Buzzer de alerta |
| 26 | PIN_RELAY_1 | Relé sirena/alarma |
| 27 | PIN_RELAY_2 | Relé auxiliar |
| 33 | PIN_DEFROST_INPUT | Señal defrost del reefer |
| 2 | PIN_LED_STATUS | LED estado (integrado) |
| 15 | PIN_LED_ALERT | LED alerta |
| 0 | PIN_WIFI_RESET | Botón reset WiFi |

### Pines Libres (12 disponibles)

| GPIO | Tipo | Notas |
|------|------|-------|
| 12 | Touch/ADC | Cuidado: afecta boot si HIGH |
| 13 | Touch/ADC | Libre |
| 14 | Touch/ADC | Libre |
| 21 | I2C SDA | Reservado para expansión I2C |
| 22 | I2C SCL | Reservado para expansión I2C |
| 23 | SPI MOSI | Libre |
| 25 | DAC1 | Libre |
| 32 | ADC1 | Reservado para sensor corriente |
| 34 | ADC1 | Solo entrada (sin pull-up) |
| 35 | ADC1 | Solo entrada (sin pull-up) |
| 36 (VP) | ADC1 | Solo entrada |
| 39 (VN) | ADC1 | Solo entrada |

### Pines NO USAR

| GPIO | Razón |
|------|-------|
| 1 | TX0 (Serial) |
| 3 | RX0 (Serial) |
| 6-11 | Flash SPI interno |

## Capacidad del Sistema

| Recurso | Máximo | Actual | Disponible |
|---------|--------|--------|------------|
| Sondas DS18B20 | 8 | 6 | 2 |
| Sensores puerta | 3 | 3 | 0 |
| Relés | 2 | 2 | 0 |
| DHT22 | 1 | 1 | 0 |
| Sensor corriente | 1 | 0 | 1 |
| Pines GPIO libres | - | - | 12 |
| Pines ADC libres | - | - | 6 |

## Configuración del Device ID

Editar en `config.h`:

```cpp
// Para Santa Cruz (producción)
#define DEVICE_ID           "REEFER_01_SCZ"
#define DEVICE_NAME         "Reefer Principal"
#define DEVICE_LOCATION     "Cerro Moro, Santa Cruz"

// Para Bahía Blanca (desarrollo)
#define DEVICE_ID           "REEFER_DEV_BHI"
#define DEVICE_NAME         "Reefer Desarrollo"
#define DEVICE_LOCATION     "Bahía Blanca, Buenos Aires"
```

## Habilitar/Deshabilitar Sensores

En `config.h`:

```cpp
// Sondas de temperatura
#define TEMP_SENSOR_1_ENABLED   true
#define TEMP_SENSOR_2_ENABLED   false
#define TEMP_SENSOR_3_ENABLED   false
// ...

// Puertas
#define DOOR_1_ENABLED      true
#define DOOR_2_ENABLED      false
#define DOOR_3_ENABLED      false

// DHT22
#define DHT22_ENABLED       false

// Relés
#define RELAY_1_ENABLED     true
#define RELAY_2_ENABLED     false
```

## Agregar Nuevos Sensores

### Agregar sonda DS18B20

1. Conectar al bus 1-Wire (GPIO4)
2. En `config.h`, habilitar: `#define TEMP_SENSOR_X_ENABLED true`
3. Opcionalmente cambiar nombre: `#define TEMP_SENSOR_X_NAME "Mi Sonda"`

### Agregar sensor de puerta

1. Conectar reed switch al GPIO correspondiente
2. En `config.h`, habilitar: `#define DOOR_X_ENABLED true`
3. Opcionalmente cambiar nombre: `#define DOOR_X_NAME "Mi Puerta"`

## Tiempos Configurables

| Parámetro | Default | Descripción |
|-----------|---------|-------------|
| ALERT_DELAY_SEC | 300 | Segundos antes de alertar (5 min) |
| DOOR_OPEN_MAX_SEC | 180 | Máximo puerta abierta (3 min) |
| DEFROST_COOLDOWN_SEC | 1800 | Espera post-defrost (30 min) |
| DEFROST_MAX_DURATION_SEC | 3600 | Máximo defrost (60 min) |
| CONFIG_APPLY_TIME_SEC | 10 | Tiempo aplicar config (10 seg) |

## Payload JSON de Estado

```json
{
  "device_id": "REEFER_DEV_BHI",
  "device_name": "Reefer Desarrollo",
  "firmware_version": "4.0.0",
  "state": {
    "state": "NORMAL",
    "state_code": 0,
    "state_since_sec": 3600
  },
  "sensors": {
    "temperatures": {
      "avg": -20.5,
      "min": -21.0,
      "max": -20.0,
      "sensors": [
        {"index": 0, "name": "Interior Principal", "value": -20.5, "valid": true}
      ]
    },
    "doors": {
      "any_open": false,
      "sensors": [
        {"index": 0, "name": "Principal", "is_open": false}
      ]
    }
  },
  "thresholds": {
    "temp_critical": -10.0,
    "alert_delay_sec": 300,
    "defrost_cooldown_sec": 1800
  }
}
```

## Compilación

1. Abrir `firmware_v2.ino` en Arduino IDE
2. Seleccionar placa: ESP32 Dev Module
3. Verificar `config.h` con el DEVICE_ID correcto
4. Compilar y subir

## Librerías Requeridas

- WiFiManager
- ArduinoJson
- OneWire
- DallasTemperature
- DHT sensor library
