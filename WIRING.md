# Guía de Conexiones — Domotic Home (ATmega2560)

## Componentes necesarios

| Cant. | Componente                | Propósito                             |
|-------|---------------------------|---------------------------------------|
| 4     | Pulsadores / DIP-Switch   | Ventana 1, Ventana 2, Puerta, Humo    |
| 2     | Pulsadores / DIP-Switch   | Selector de ubicación (bits 0 y 1)    |
| 2     | Potenciómetros 10kΩ       | Temperatura (ADC0) y Luz (ADC1)       |
| 1     | Módulo RFID RC522         | Control de accesos                    |
| 1     | Servomotor SG90           | Puerta de garaje                      |
| 1     | LCD 16x2                  | Interfaz visual local                 |
| 2     | LEDs + resistencias 220Ω  | Calefactor y Ventilador (simulados)   |
| 1     | LED + resistencia 220Ω    | Iluminación dimerizada (PWM)          |
| 1     | LED + resistencia 220Ω    | Volumen de sonido (PWM)               |
| 1     | Protoboard                | Conexiones                            |
| 1     | Fuente 5V / USB           | Alimentación                          |

---

## Diagrama de conexiones

### 1. Módulo Seguridad — Puerto C (entradas con pull-up)

```
ATmega2560              Componente
─────────────           ───────────
PC0 (pin 37/A8)   ═══  DIP 1 — Ventana 1 (GND al cerrar)
PC1 (pin 36/A9)   ═══  DIP 2 — Ventana 2 (GND al cerrar)
PC2 (pin 35/A10)  ═══  DIP 3 — Puerta     (GND al cerrar)
PC3 (pin 34/A11)  ═══  DIP 4 — Humo       (GND al activar)
```

Configuración: Pull-up internos activados por software. Cada switch conecta a GND cuando está "abierto/activado".

---

### 2. Módulo Accesos — Selector + RFID + Servo

```
ATmega2560              Componente
─────────────           ───────────
PA0 (pin 22)      ═══  DIP 5 — Selector bit 0 (LSB)
PA1 (pin 23)      ═══  DIP 6 — Selector bit 1 (MSB)

Pin 53 (SS)       ═══  RFID RC522 → SDA
Pin 52 (SCK)      ═══  RFID RC522 → SCK
Pin 51 (MOSI)     ═══  RFID RC522 → MOSI
Pin 50 (MISO)     ═══  RFID RC522 → MISO
Pin 5  (RST)      ═══  RFID RC522 → RST    ⚠ CONFLICTO (ver abajo)
3.3V              ═══  RFID RC522 → VCC
GND               ═══  RFID RC522 → GND

Pin 11            ═══  Servomotor → Señal  ⚠ CONFLICTO (ver abajo)
5V                ═══  Servomotor → VCC
GND               ═══  Servomotor → GND
```

---

### 3. Módulo Confort — ADC (C puro) + Actuadores

```
ATmega2560              Componente
─────────────           ───────────
PK0 (pin A0)      ═══  Potenciómetro 1 — Temperatura (centro)
PK1 (pin A1)      ═══  Potenciómetro 2 — Iluminación  (centro)
Aref              ═══  Capacitor 100nF a GND (estabilización)

PL0 (pin 49)      ═══  LED + 220Ω → GND  (Calefactor)
PL1 (pin 48)      ═══  LED + 220Ω → GND  (Ventilador)

Pin 4             ═══  LED + 220Ω → GND  (Dimmer PWM)
```

Potenciómetros: patas laterales a 5V y GND, pata central al pin ADC.

---

### 4. Módulo Remoto

```
ATmega2560              Componente
─────────────           ───────────
Pin 9             ═══  LED + 220Ω → GND  (Volumen sonido PWM)
USB/Serial        ═══  PC (Monitor Serial) — comandos HORNO/SONIDO/MERCADO
```

---

### 5. Módulo Pantalla — LCD 16x2 (paralelo 4 bits)

```
ATmega2560              LCD 16x2
─────────────           ────────
Pin 12            ═══  RS
Pin 11            ═══  EN              ⚠ CONFLICTO (ver abajo)
Pin 5             ═══  D4              ⚠ CONFLICTO (ver abajo)
Pin 6             ═══  D5
Pin 7             ═══  D6
Pin 8             ═══  D7
5V                ═══  VCC, A (backlight)
GND               ═══  GND, K (backlight)
Pot 10kΩ (centro) ═══  V0 (contraste)
Pot 10kΩ (lateral)═══  5V y GND
```

---

## ⚠ Conflictos de pines detectados en el código actual

El código tiene **2 conflictos** que deben resolverse antes de cablear:

| Pin | Módulo A              | Módulo B              | Solución recomendada                |
|-----|-----------------------|-----------------------|-------------------------------------|
| 11  | `servoGaraje.attach(11)` | `LCD_EN` (Definiciones.h) | Mover servo a pin **10** o LCD_EN a pin **3** |
| 5   | `RST_PIN 5` (RFID)    | `LCD_D4` (Definiciones.h) | Mover RST_PIN a pin **2** o LCD_D4 a pin **3** |

### Cambios necesarios en el código:

**Archivo `src/accesos/Accesos.cpp`:**
```cpp
#define RST_PIN 2       // Cambiar de 5 a 2

servoGaraje.attach(10); // Cambiar de 11 a 10
```

**O alternativamente, archivo `src/common/Definiciones.h`:**
```cpp
#define LCD_EN 3        // Cambiar de 11 a 3
#define LCD_D4 2        // Cambiar de 5 a 2 (si RST_PIN se queda en 5... conflicto con LCD_D4 otra vez)
```

**Recomendación:** Aplicar ambos cambios en Accesos.cpp (RST_PIN → 2, servo → 10).

---

## Resumen de pines final (con conflictos resueltos)

| Pin  | Conexión              | Módulo       |
|------|-----------------------|--------------|
| PC0  | DIP Ventana 1         | Seguridad    |
| PC1  | DIP Ventana 2         | Seguridad    |
| PC2  | DIP Puerta            | Seguridad    |
| PC3  | DIP Humo              | Seguridad    |
| PA0  | DIP Selector bit 0    | Accesos      |
| PA1  | DIP Selector bit 1    | Accesos      |
| A0   | Potenciómetro Temp    | Confort      |
| A1   | Potenciómetro Luz     | Confort      |
| 2    | RFID RST              | Accesos      |
| 4    | LED Dimmer            | Confort      |
| 6    | LCD D5                | Pantalla     |
| 7    | LCD D6                | Pantalla     |
| 8    | LCD D7                | Pantalla     |
| 9    | LED Volumen Sonido    | Remoto       |
| 10   | Servomotor Garaje     | Accesos      |
| 12   | LCD RS                | Pantalla     |
| 48   | LED Ventilador        | Confort      |
| 49   | LED Calefactor        | Confort      |
| 50   | RFID MISO (SPI)       | Accesos      |
| 51   | RFID MOSI (SPI)       | Accesos      |
| 52   | RFID SCK (SPI)        | Accesos      |
| 53   | RFID SDA/SS (SPI)     | Accesos      |
| 3    | LCD EN                | Pantalla     |
| 5    | LCD D4                | Pantalla     |

> **Nota:** Los pines 3 y 5 se liberan para LCD_EN y LCD_D4 respectivamente tras mover el servo al pin 10 y RST_PIN al pin 2.
