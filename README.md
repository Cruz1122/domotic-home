<p align="center" style="font-size:2rem;"><strong>Domotic Home</strong></p>
<p align="center" style="font-size:1.25rem; margin-top:-1em;"><em>Sistema domótico modular en C puro para ATmega2560</em></p>

<p align="center">
  <strong>Plataforma objetivo</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/MCU-ATmega2560-555555?labelColor=00979D&logo=arduino&logoColor=white" alt="ATmega2560" />
  <img src="https://img.shields.io/badge/Lenguaje-C_puro-555555?labelColor=283593&logo=c&logoColor=white" alt="C puro" />
  <img src="https://img.shields.io/badge/Librerias-Solo_propias-555555?labelColor=37474F" alt="Sin librerías externas" />
  <img src="https://img.shields.io/badge/Arquitectura-Maquina_de_estados-555555?labelColor=6A1B9A" alt="Máquina de estados" />
</p>

<p align="center">
  <strong>Interfaces y subsistemas</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/RFID-RC522-555555?labelColor=1565C0" alt="RFID RC522" />
  <img src="https://img.shields.io/badge/LCD-Interfaz_local-555555?labelColor=2E7D32" alt="LCD" />
  <img src="https://img.shields.io/badge/Keypad-4x4-555555?labelColor=455A64" alt="Teclado matricial" />
  <img src="https://img.shields.io/badge/Serial-Telemetria-555555?labelColor=EF6C00" alt="Serial" />
  <img src="https://img.shields.io/badge/EEPROM-Persistencia-555555?labelColor=5D4037" alt="EEPROM" />
</p>

## Descripción

`domotic-home` es el firmware de un sistema domótico académico para vivienda, implementado sobre **ATmega2560** con enfoque procedural en **C puro**. El sistema integra seguridad, control de accesos RFID, interfaz local por LCD y teclado matricial, persistencia en EEPROM, actuadores reales/simulados y reporte de eventos por puerto serial.

El alcance fue cerrado para evitar vender una casa domótica ficticia: varios actuadores del enunciado se representarán mediante LEDs, PWM, relé, servomotor o estados visibles en LCD/serial. Lo importante de la entrega es demostrar una arquitectura coherente, no una maqueta sobreactuada con funciones inconexas.

## Decisiones cerradas del proyecto

| Decisión | Definición |
|---|---|
| Microcontrolador | **ATmega2560 / Arduino Mega 2560** |
| Lenguaje | C puro, sin librerías externas |
| Librerías propias | Permitidas: drivers propios en `.c/.h` |
| UI principal | LCD + teclado matricial 4x4 |
| Código de administración | Constante compilada, digitada por teclado matricial |
| Persistencia | EEPROM interna del ATmega2560 |
| Comunicación de diagnóstico | UART/Serial hacia PC |
| RFID | RC522 por SPI, con driver propio |
| Alarmas | Intrusión por PIR y fuego/humo por MQ-2 |
| Servicios remotos | Simulados/locales desde menú LCD + teclado |

## Alcance funcional

### Seguridad

- Alarma de acceso activable/desactivable mediante código.
- Alarma de incendio activable/desactivable mediante código.
- Detección de intrusión usando dos sensores PIR HC-SR501.
- Detección de humo usando sensor MQ-2 por ADC.
- Reporte obligatorio por serial ante eventos críticos.
- Visualización de estados y alertas en LCD.

### Accesos RFID

- Lectura de tarjetas RFID mediante RC522.
- Enrolamiento de tarjetas autorizadas.
- Borrado lógico de tarjetas existentes.
- Roles mínimos: padre/administrador e hijo.
- Acceso por puerta principal simulado con LED.
- Acceso por garaje mediante servomotor real.
- Habitación de juegos con cupos por hijo.
- Descuento de cupos después de cada ingreso.
- Recarga de cupos por parte de padres/administradores.
- Persistencia de usuarios y cupos en EEPROM.

### Confort

- Iluminación dimerizada mediante LED controlado por PWM.
- Nivel de iluminación definido por potenciómetro.
- Control de temperatura simulado desde teclado matricial.
- Salidas de calefacción y ventilación representadas por LEDs o relé según disponibilidad.

### Servicios

- Horno configurable desde teclado: temperatura y tiempo.
- Apagado automático del horno al terminar el tiempo programado.
- Equipo de sonido configurable desde menú.
- Volumen por potenciómetro, mostrado como porcentaje en LCD en tiempo real.
- Salida PWM proporcional al volumen solicitado.
- Lista de mercado consultable desde menú LCD.
- Productos de mercado preferiblemente predefinidos, con cantidad configurable.

## Hardware base

| Elemento | Uso en el sistema | Tipo de implementación |
|---|---|---|
| ATmega2560 / Mega 2560 | Controlador principal | Físico |
| RFID-RC522 | Identificación de usuarios | Físico |
| 2x PIR HC-SR501 | Intrusión/presencia | Físico |
| MQ-2 | Humo/incendio | Físico |
| LCD | Interfaz de usuario | Físico |
| Teclado matricial 4x4 | Navegación y captura de código/datos | Físico |
| Servo | Garaje | Físico |
| LED puerta | Imán/cerradura principal | Simulación |
| LED iluminación | Dimmer | Simulación física por PWM |
| LEDs/relé | Calefactor, ventilador, horno | Simulación |
| Potenciómetro | Dimmer y volumen | Físico |
| UART/USB | Reporte serial | Físico |

## Estructura del repositorio

```txt
src/
  app/
    Proyecto_Domotica.ino       Orquestador principal: setup/loop

  common/
    Definiciones.h              Macros, constantes, tipos, mapa de pines

  seguridad/
    Seguridad.h                 Contrato del subsistema de alarmas
    Seguridad.c                 Implementación de PIR, MQ-2 y estados de alarma

  accesos/
    Accesos.h                   Contrato de RFID, usuarios, puerta, garaje y juegos
    Accesos.c                   Implementación de accesos y persistencia lógica

  confort/
    Confort.h                   Contrato de iluminación, temperatura y sonido
    Confort.c                   PWM, ADC, actuadores de confort

  remoto/
    Remoto.h                    Servicios: horno, mercado y operaciones tipo remoto
    Remoto.c                    Gestión de servicios desde UI local

docs/
  00_ALCANCE_Y_DEMO.md          Alcance cerrado y guion de presentación
  01_ARQUITECTURA.md            Máquina de estados, módulos y ciclo no bloqueante
  02_HARDWARE_Y_PINOUT.md       Hardware, simulaciones y asignación preliminar de pines
  03_INTERFAZ_LCD_TECLADO.md    Menús, navegación y pantallas LCD
  04_EEPROM.md                  Modelo de datos persistente
  05_PLAN_IMPLEMENTACION.md     Orden de trabajo y criterios de aceptación
  REFERENCIAS.md                Fuentes técnicas base
```

## Arquitectura recomendada

El firmware debe implementarse como una **máquina de estados centralizada**, con tareas cooperativas no bloqueantes. La función `loop()` no debe quedar atrapada esperando entradas en un menú, ni usando retardos largos para horno, servo, LCD o alarmas.

Flujo mínimo esperado:

```txt
setup()
  -> drivers_init()
  -> seguridad_init()
  -> accesos_init()
  -> confort_init()
  -> servicios_init()
  -> ui_init()

loop()
  -> tick_actualizar()
  -> keypad_scan()
  -> rfid_task()
  -> seguridad_task()
  -> confort_task()
  -> servicios_task()
  -> ui_task()
  -> serial_task()
```

La regla es simple: **ningún módulo debe bloquear al resto**. Si el horno está encendido por 5 minutos, el sistema debe seguir leyendo RFID, PIR, MQ-2, teclado y actualizando LCD.

## Interfaz de usuario

La navegación se realiza con teclado matricial:

| Tecla | Uso recomendado |
|---|---|
| `A` | Seguridad |
| `B` | Accesos RFID |
| `C` | Ambiente/confort |
| `D` | Servicios |
| `#` | Confirmar |
| `*` | Volver/cancelar |
| `0-9` | Datos numéricos |

Pantalla principal propuesta:

```txt
A Seg B RFID
C Amb D Serv
```

## Demo mínima obligatoria

La presentación debe ejecutarse como una secuencia integrada:

1. Iniciar sistema y entrar al menú con código.
2. Activar alarma de acceso, disparar PIR y ver alerta en LCD + serial.
3. Activar alarma de incendio, disparar MQ-2 y ver alerta en LCD + serial.
4. Leer tarjeta RFID autorizada y simular apertura de puerta con LED.
5. Leer tarjeta autorizada y abrir garaje con servo.
6. Leer tarjeta de hijo, ingresar a sala de juegos y descontar cupo.
7. Reiniciar y demostrar que los cupos persisten en EEPROM.
8. Controlar iluminación con potenciómetro y PWM.
9. Configurar horno por teclado y ver cuenta regresiva no bloqueante.
10. Encender sonido, variar volumen por potenciómetro y ver porcentaje en LCD.
11. Agregar/consultar productos de mercado.

## Reglas de implementación

- No usar librerías externas como `MFRC522`, `LiquidCrystal`, `Servo`, `Keypad`, `EEPROM` o `SPI`.
- Sí se permiten módulos propios: `spi.c`, `uart.c`, `lcd.c`, `keypad.c`, `adc.c`, `pwm.c`, `timer.c`, `eeprom.c`, `rfid_rc522.c`.
- Evitar `delay()` en cualquier lógica funcional.
- Usar `uint8_t`, `uint16_t`, `uint32_t` y tipos explícitos.
- Mantener el `.ino` como orquestador mínimo si se compila con Arduino IDE.
- Aislar el acceso a registros en drivers propios.
- Reportar por serial todo evento relevante de seguridad, acceso, horno y errores.
- Mantener los datos persistentes en EEPROM con cabecera de validez.

## Documentación rápida

- [Alcance y demo](docs/00_ALCANCE_Y_DEMO.md)
- [Arquitectura](docs/01_ARQUITECTURA.md)
- [Hardware y pinout](docs/02_HARDWARE_Y_PINOUT.md)
- [Interfaz LCD/teclado](docs/03_INTERFAZ_LCD_TECLADO.md)
- [EEPROM](docs/04_EEPROM.md)
- [Plan de implementación](docs/05_PLAN_IMPLEMENTACION.md)
- [Referencias](docs/REFERENCIAS.md)
