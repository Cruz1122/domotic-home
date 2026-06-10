# Mapeo Completo de Pines - ATmega2560

Este documento contiene la consolidación del mapeo de pines para el microcontrolador ATmega2560, extraído de los diagramas técnicos provistos. La información se ha organizado en tablas estructuradas según sus funciones principales y bloques lógicos.

---

## 1. Pines de Alimentación y Control Base

| Pin | Mapeo del Microcontrolador (Puertos y Funciones) | Categoría / Grupo | Nota |
| :--- | :--- | :--- | :--- |
| **RESET** | RESET (Activo en bajo) | Control | Señal de reinicio del sistema |
| **VCC** | VCC | Alimentación | Línea de voltaje positivo |
| **GND** | GND | Alimentación | Línea de tierra de referencia |
| **AREF** | AREF | Referencia | Voltaje de referencia analógica para el ADC |

---

## 2. Pines Cronológicos 0 al 21 (PWM & Comunicación)

| Pin | Mapeo del Microcontrolador (Puertos y Funciones) | Grupo |
| :--- | :--- | :--- |
| **0** | RX0 PE0 / RXD0 / PCINT8 | COMUNICACION |
| **1** | TX0 PE1 / TXD0 / PDO | COMUNICACION |
| **2** | PE4 / OC3B / INT4 | PWM |
| **3** | PE5 / OC3C / INT5 | PWM |
| **4** | PG5 / OC0B | PWM |
| **5** | PE3 / OC3A / AIN1 | PWM |
| **6** | PH3 / OC4A | PWM |
| **7** | PH4 / OC4B | PWM |
| **8** | PH5 / OC4C | PWM |
| **9** | PH6 / OC2B | PWM |
| **10** | PB4 / OC2A / PCINT4 | PWM |
| **11** | PB5 / OC1A / PCINT5 | PWM |
| **12** | PB6 / OC1B / PCINT6 | PWM |
| **13** | PB7 / OC0A / OC1C / PCINT7 | PWM |
| **14** | TX3 PJ1 / TXD3 / PCINT10 | COMUNICACION |
| **15** | RX3 PJ0 / RXD3 / PCINT9 | COMUNICACION |
| **16** | TX2 PH1 / TXD2 | COMUNICACION |
| **17** | RX2 PH0 / RXD2 | COMUNICACION |
| **18** | TX1 PD3 / TXD1 / INT3 | COMUNICACION |
| **19** | RX1 PD2 / RXD1 / INT2 | COMUNICACION |
| **20** | SDA PD1 / SDA / INT1 | COMUNICACION |
| **21** | SCL PD0 / SCL / INT0 | COMUNICACION |

---

## 3. Pines Cronológicos 22 al 53 (Digital / Funciones Especiales)

| Pin | Mapeo del Microcontrolador (Puertos y Funciones) | Grupo | Nota |
| :--- | :--- | :--- | :--- |
| **22** | PA0 / AD0 | DIGITAL | Bus de direcciones/datos multiplexado |
| **23** | PA1 / AD1 | DIGITAL | Bus de direcciones/datos multiplexado |
| **24** | PA2 / AD2 | DIGITAL | Bus de direcciones/datos multiplexado |
| **25** | PA3 / AD3 | DIGITAL | Bus de direcciones/datos multiplexado |
| **26** | PA4 / AD4 | DIGITAL | Bus de direcciones/datos multiplexado |
| **27** | PA5 / AD5 | DIGITAL | Bus de direcciones/datos multiplexado |
| **28** | PA6 / AD6 | DIGITAL | Bus de direcciones/datos multiplexado |
| **29** | PA7 / AD7 | DIGITAL | Bus de direcciones/datos multiplexado |
| **30** | PC7 / A15 | DIGITAL | Bus de direcciones (Bits altos) |
| **31** | PC6 / A14 | DIGITAL | Bus de direcciones (Bits altos) |
| **32** | PC5 / A13 | DIGITAL | Bus de direcciones (Bits altos) |
| **33** | PC4 / A12 | DIGITAL | Bus de direcciones (Bits altos) |
| **34** | PC3 / A11 | DIGITAL | Bus de direcciones (Bits altos) |
| **35** | PC2 / A10 | DIGITAL | Bus de direcciones (Bits altos) |
| **36** | PC1 / A9 | DIGITAL | Bus de direcciones (Bits altos) |
| **37** | PC0 / A8 | DIGITAL | Bus de direcciones (Bits altos) |
| **38** | PD7 / T0 | DIGITAL | Entrada de reloj externa para Timer 0 |
| **39** | PG2 / ALE | DIGITAL | Address Latch Enable (Habilitación de latch) |
| **40** | PG1 / RD | DIGITAL | Señal de Lectura (Activa en bajo) |
| **41** | PG0 / WR | DIGITAL | Señal de Escritura (Activa en bajo) |
| **42** | PL7 | DIGITAL | Puerto L (Pin 7) |
| **43** | PL6 | DIGITAL | Puerto L (Pin 6) |
| **44** | PL5 / OC5C | DIGITAL | Salida de comparación del Timer 5C |
| **45** | PL4 / OC5B | DIGITAL | Salida de comparación del Timer 5B |
| **46** | PL3 / OC5A | DIGITAL | Salida de comparación del Timer 5A |
| **47** | PL2 / T5 | DIGITAL | Entrada de reloj externa para Timer 5 |
| **48** | PL1 / ICP5 | DIGITAL | Pin de captura de entrada de Timer 5 |
| **49** | PL0 / ICP4 | DIGITAL | Pin de captura de entrada de Timer 4 |
| **50** | PB3 / MISO / PCINT3 | DIGITAL | SPI MISO / Interrupción por cambio de pin |
| **51** | PB2 / MOSI / PCINT2 | DIGITAL | SPI MOSI / Interrupción por cambio de pin |
| **52** | PB1 / SCK / PCINT1 | DIGITAL | SPI Clock / Interrupción por cambio de pin |
| **53** | PB0 / SS / PCINT0 | DIGITAL | SPI Slave Select (Activo en bajo) |

---

## 4. Pines de Entrada Analógica (Analog In)

*Nota: Tenga en cuenta que el orden físico de los pines A4 a A7 en el esquema se presenta de manera invertida para optimizar el ruteo de las señales JTAG.*

| Pin | Mapeo del Microcontrolador (Puertos y Funciones) | Grupo | Función Secundaria |
| :--- | :--- | :--- | :--- |
| **A0** | PF0 / ADC0 | ANALOG IN | Entrada analógica 0 |
| **A1** | PF1 / ADC1 | ANALOG IN | Entrada analógica 1 |
| **A2** | PF2 / ADC2 | ANALOG IN | Entrada analógica 2 |
| **A3** | PF3 / ADC3 | ANALOG IN | Entrada analógica 3 |
| **A7** | PF7 / ADC7 / TDI | ANALOG IN | Entrada analógica 7 / JTAG Test Data In |
| **A6** | PF6 / ADC6 / TDO | ANALOG IN | Entrada analógica 6 / JTAG Test Data Out |
| **A5** | PF5 / ADC5 / TMS | ANALOG IN | Entrada analógica 5 / JTAG Test Mode Select |
| **A4** | PF4 / ADC4 / TCK | ANALOG IN | Entrada analógica 4 / JTAG Test Clock |
| **A8** | PK0 / ADC8 / PCINT16 | ANALOG IN | Entrada analógica 8 / Interrupción PCINT16 |
| **A9** | PK1 / ADC9 / PCINT17 | ANALOG IN | Entrada analógica 9 / Interrupción PCINT17 |
| **A10** | PK2 / ADC10 / PCINT18 | ANALOG IN | Entrada analógica 10 / Interrupción PCINT18 |
| **A11** | PK3 / ADC11 / PCINT19 | ANALOG IN | Entrada analógica 11 / Interrupción PCINT19 |
| **A12** | PK4 / ADC12 / PCINT20 | ANALOG IN | Entrada analógica 12 / Interrupción PCINT20 |
| **A13** | PK5 / ADC13 / PCINT21 | ANALOG IN | Entrada analógica 13 / Interrupción PCINT21 |
| **A14** | PK6 / ADC14 / PCINT22 | ANALOG IN | Entrada analógica 14 / Interrupción PCINT22 |
| **A15** | PK7 / ADC15 / PCINT23 | ANALOG IN | Entrada analógica 15 / Interrupción PCINT23 |

---

## 5. Uso en domotic-home

| Subsistema | Pines Arduino | Puerto MCU |
| :--- | :--- | :--- |
| LCD RS/E/D4-D7 | D22-D27 | PA0-PA5 |
| Teclado filas R1-R4 | D34-D37 | PC3-PC0 |
| Teclado columnas C1-C4 | A8-A11 | PK0-PK3 (PCINT16-19) |
| RFID SPI | D50-D53 | PB3-PB0 |

---
*Documento generado automáticamente con la consolidación completa de las tres secciones esquemáticas.*
