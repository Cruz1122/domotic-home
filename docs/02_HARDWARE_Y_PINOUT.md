# Hardware, simulaciones y pinout preliminar

## Hardware objetivo

La plataforma final del proyecto es ATmega2560 / Arduino Mega 2560. La placa observada en la fotografía de pruebas no cambia el objetivo final: el firmware debe mapearse y probarse sobre ATmega2560.

## Elementos físicos confirmados

| Elemento | Cantidad | Uso |
|---|---:|---|
| ATmega2560 / Arduino Mega 2560 | 1 | Controlador principal |
| LCD | 1 | Salida local |
| Teclado matricial 4x4 | 1 | Interfaz de usuario |
| RFID-RC522 | 1 | Lectura de tarjetas |
| PIR HC-SR501 | 2 | Detección de presencia/intrusión |
| MQ-2 | 1 | Detección de humo/gas |
| Servomotor | 1 | Puerta de garaje |
| Potenciómetro | 1 | Dimmer de iluminación |
| LEDs | Varios | Simulación de actuadores |
| Relé | Opcional | Simulación de carga/horno/ventilador |
| UART/USB | 1 | Reporte por serial |

## Simulaciones declaradas

| Subsistema | Representación |
|---|---|
| Imán de puerta principal | LED indicador de cerradura/solenoide |
| Horno | LED o relé, temporizado por software |
| Calefactor | LED o relé |
| Ventilador | LED o relé |
| Habitación de juegos | Estado lógico + LCD + evento serial |
| Equipo de sonido | PWM proporcional al volumen + LCD |

## UART usada por la aplicación

| Puerto | Pines Mega | Uso |
|---|---|---|
| `UART0 / Serial` | D0/D1 | Debug y eventos del sistema |
| `UART1 / Serial1` | D19/D18 | Control remoto / Virtual Terminal |
| `UART2 / Serial2` | D17/D16 | Libre, sin uso funcional |
| `UART3 / Serial3` | D15/D14 | Libre, sin uso funcional |

## Pinout preliminar sugerido

Este pinout es una propuesta inicial. Debe validarse contra la placa física, el LCD disponible y el cableado real. La regla es no pisar los pines SPI del ATmega2560 usados por el RC522.

### RFID-RC522 por SPI

| Señal RC522 | Pin Mega 2560 sugerido | Nota |
|---|---:|---|
| SDA / SS | D53 | Chip select SPI |
| SCK | D52 | SPI clock |
| MOSI | D51 | SPI MOSI |
| MISO | D50 | SPI MISO |
| RST | D49 | Reset RC522 |
| 3.3V | 3.3V | No alimentar con 5V si el módulo no lo tolera |
| GND | GND | Tierra común |

### LCD paralelo 16x2

| Señal LCD | Pin sugerido |
|---|---:|
| RS | D22 |
| E | D23 |
| D4 | D24 |
| D5 | D25 |
| D6 | D26 |
| D7 | D27 |
| RW | GND |
| V0 | Potenciómetro contraste |

### Teclado matricial 4x4

Fuente de verdad: `docs/06_MAPEO_PINES_ATMEGA2560.md`.

| Línea | Pin Arduino | Puerto MCU | PCINT |
|---:|---:|---:|---:|
| Fila 1 | D34 | PC3 | — |
| Fila 2 | D35 | PC2 | — |
| Fila 3 | D36 | PC1 | — |
| Fila 4 | D37 | PC0 | — |
| Columna 1 | A8 / D62 | PK0 | PCINT16 |
| Columna 2 | A9 / D63 | PK1 | PCINT17 |
| Columna 3 | A10 / D64 | PK2 | PCINT18 |
| Columna 4 | A11 / D65 | PK3 | PCINT19 |

### Sensores y actuadores

| Elemento | Pin sugerido | Tipo |
|---|---:|---|
| PIR 1 | D38 | Entrada digital |
| PIR 2 | D39 | Entrada digital |
| MQ-2 AO | A0 | Entrada ADC |
| Pot dimmer | A1 | Entrada ADC |
| LED puerta principal | D43 | Digital |
| LED iluminación | D7 | PWM |
| PWM sonido | D8 | PWM |
| Servo garaje | D9 | PWM/timer |
| Calefactor simulado | D40 | Digital |
| Ventilador simulado | D41 | Digital |
| Horno simulado | D42 | Digital |
| Buzzer alarma acceso | D10 | PWM/digital |
| LED alarma incendio | D11 | Digital |

## Notas de integración

- El RC522 suele trabajar a 3.3 V. Verificar tolerancia de entradas del módulo usado antes de conectarlo directo a señales de 5 V.
- Los sensores PIR entregan salida digital. Usarlos como eventos de intrusión/presencia.
- El MQ-2 debe leerse por ADC para mostrar porcentaje y umbral. Usar salida digital solo como plan B.
- El servo necesita alimentación estable. No alimentarlo desde un pin del microcontrolador.
- Relés, motores y cargas externas requieren transistor/driver, diodo de protección y fuente adecuada. Para la demo, LEDs son más seguros y suficientes.
- Todo GND debe estar común entre ATmega2560, sensores, servo y módulos.

## Macros sugeridas en `Definiciones.h`

```c
#define PIN_RFID_SS              53
#define PIN_RFID_RST             49

#define PIN_LCD_RS               22
#define PIN_LCD_E                23
#define PIN_LCD_D4               24
#define PIN_LCD_D5               25
#define PIN_LCD_D6               26
#define PIN_LCD_D7               27

#define PIN_KEYPAD_R1            34   /* PC3 */
#define PIN_KEYPAD_R2            35   /* PC2 */
#define PIN_KEYPAD_R3            36   /* PC1 */
#define PIN_KEYPAD_R4            37   /* PC0 */
#define PIN_KEYPAD_C1            62   /* PK0 / A8 */
#define PIN_KEYPAD_C2            63   /* PK1 / A9 */
#define PIN_KEYPAD_C3            64   /* PK2 / A10 */
#define PIN_KEYPAD_C4            65   /* PK3 / A11 */

#define PIN_PIR_1                38
#define PIN_PIR_2                39
#define PIN_HEATER_LED           40
#define PIN_FAN_LED              41
#define PIN_OVEN_LED             42

#define PIN_DOOR_LED             43
#define PIN_LIGHT_PWM            7
#define PIN_SOUND_PWM            8
#define PIN_GARAGE_SERVO         9
#define PIN_ALARM_BUZZER         10
#define PIN_FIRE_LED            11

#define ADC_MQ2                  0
#define ADC_LIGHT_POT            1
```

## Validación eléctrica mínima

Antes de integrar software, validar:

1. LCD imprime texto básico.
2. Teclado detecta todas las teclas sin rebote grave.
3. UART transmite mensajes legibles.
4. ADC lee MQ-2 y potenciómetro de iluminación.
5. PWM varía LED de iluminación.
6. Servo se mueve con señal correcta y alimentación externa si hace falta.
7. RC522 responde por SPI.
8. EEPROM escribe y lee un byte de prueba.
