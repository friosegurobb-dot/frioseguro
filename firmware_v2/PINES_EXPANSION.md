# Pines ESP32 - Guía de Expansión

## Pines Actualmente en Uso

| GPIO | Función Actual | Módulo |
|------|----------------|--------|
| 2 | LED_OK (built-in) | Estado |
| 4 | OneWire (DS18B20) | Sensores temp |
| 5 | Sensor puerta 1 | Puerta |
| 15 | LED_ALERT | Estado |
| 17 | Buzzer | Alertas |
| 18 | DHT22 | Humedad |
| 19 | WiFi Reset Button | Config |
| 26 | Relay sirena | Alertas |

---

## Pines Disponibles para Expansión

### ADC (Analógicos) - Solo Entrada
| GPIO | Recomendación | Notas |
|------|---------------|-------|
| **34** | Sensor voltaje AC | Solo entrada, no tiene pull-up |
| **35** | Nivel batería | Solo entrada |
| **36 (VP)** | Sensor corriente ACS712 | Solo entrada, más estable |
| **39 (VN)** | Sensor adicional | Solo entrada |

### Digitales de Propósito General
| GPIO | Recomendación | Notas |
|------|---------------|-------|
| **13** | Puerta 2 | Tiene pull-up interno |
| **14** | Puerta 3 | Tiene pull-up interno |
| **27** | Puerta 4 | Tiene pull-up interno |
| **25** | Reset SIM800 | O relay adicional |
| **32** | Relay 2 | Para luz estroboscópica |
| **33** | Relay 3 | Para ventilador |

### UART2 (Serial para SIM800)
| GPIO | Función | Notas |
|------|---------|-------|
| **16** | RX2 (recibe de SIM800 TX) | UART2 |
| **17** | TX2 (envía a SIM800 RX) | ⚠️ Ya usado por buzzer |

> **Nota**: Si usas SIM800, mover buzzer a GPIO25 o GPIO33

### I2C (Para sensores avanzados)
| GPIO | Función | Notas |
|------|---------|-------|
| **21** | SDA | Display OLED, INA219 |
| **22** | SCL | Display OLED, INA219 |

---

## Configuración Recomendada para Expansión Completa

```
SENSORES DE TEMPERATURA (Bus OneWire - GPIO4)
├── DS18B20 #1 - Evaporador
├── DS18B20 #2 - Condensador  
├── DS18B20 #3 - Ambiente interno
└── DS18B20 #4 - Ambiente externo

SENSORES DE PUERTA (Reed Switch)
├── GPIO5  - Puerta principal
├── GPIO13 - Puerta secundaria
├── GPIO14 - Acceso lateral
└── GPIO27 - Compuerta trasera

MONITOREO ELÉCTRICO
├── GPIO34 - Detector corte de luz (optoacoplador)
├── GPIO35 - Nivel batería backup
└── GPIO36 - Sensor corriente ACS712

COMUNICACIÓN GSM (SIM800L)
├── GPIO16 - RX2 (desde SIM800)
└── GPIO25 - TX2 (hacia SIM800) *mover buzzer*

SALIDAS
├── GPIO26 - Relay 1 (sirena)
├── GPIO32 - Relay 2 (luz estroboscópica)
├── GPIO33 - Buzzer (reubicado)
└── GPIO2  - LED estado

DISPLAY OLED (Opcional)
├── GPIO21 - SDA
└── GPIO22 - SCL
```

---

## Módulos .h Disponibles (No Integrados)

| Archivo | Descripción | Pines que usa |
|---------|-------------|---------------|
| `sim800.h` | SMS de emergencia | GPIO16, GPIO25 |
| `power_monitor.h` | Detector corte luz | GPIO34, GPIO35 |
| `door_sensors.h` | Múltiples puertas | GPIO5, 13, 14, 27 |
| `current_sensor.h` | Corriente compresor | GPIO36 |
| `serial_api.h` | Comandos COM/Web/App | Serial USB |

---

## Notas Importantes

### Pines a EVITAR
- **GPIO0**: Boot mode (no usar)
- **GPIO1**: TX0 (Serial debug)
- **GPIO3**: RX0 (Serial debug)
- **GPIO6-11**: Flash SPI (no usar)
- **GPIO12**: Boot fail si HIGH al inicio

### Consideraciones Eléctricas
- **Corriente máxima por pin**: 12mA (recomendado), 40mA (absoluto)
- **Voltaje máximo entrada**: 3.3V (usar divisores para 5V)
- **ADC**: 12-bit (0-4095), referencia 3.3V
- **Pull-up interno**: ~45kΩ

### Para SIM800L
- Alimentación: 3.7-4.2V (NO 5V directo)
- Picos de corriente: hasta 2A
- Usar capacitor 1000µF en VCC
- Divisor de voltaje en RX del SIM800 (5V->3.3V)

---

## Ejemplo: Agregar Sensor de Corriente

```cpp
// En config.h agregar:
#define PIN_CURRENT_SENSOR 36

// En firmware_modular.ino agregar:
#include "current_sensor.h"

// En setup() agregar:
currentSensorInit();

// En loop() agregar:
currentSensorLoop();
```
