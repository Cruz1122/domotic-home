# Reporte de eventos por UART

## Roles de puertos

- `UART0 / Serial`: debug interno, boot, eventos del sistema y trazas.
- `UART1 / Serial1`: control remoto / Virtual Terminal.

`UART2 / Serial2` y `UART3 / Serial3` no participan del flujo funcional actual.

Las respuestas de comandos remotos salen por `UART1` con prefijos `[OK]`, `[ERR]` o `[STATUS]`, y se replican también en `UART0` (vía `UART1_Task`). Las trazas y mensajes de depuración internos se observan solo por `UART0`.

### Bridge UART0 → UART1

Para demos con un solo cable USB, `UART_Bridge_Task()` reenvía lo tecleado en el Monitor Serie (UART0 RX) al parser de comandos remotos (buffer RX de UART1). Así se pueden enviar `HELP`, `RADIO ON`, etc. desde el mismo terminal donde aparecen los eventos `[BOOT]`, `[RFID]`, etc.

Reglas del bridge:

- Solo reenvía caracteres imprimibles y `\r`/`\n`.
- No hace eco local en UART0 (el IDE ya muestra lo tecleado).
- Las respuestas `[OK]`/`[ERR]`/`[STATUS]` salen por UART1 y se copian a UART0.

Para el detalle completo de comandos remotos disponibles, ver `docs/07_COMANDOS_USART.md`.

## Formato canónico

```
[TAG] mensaje\n
```

Todas las etiquetas se definen como macros en `src/common/Definiciones.h` con el prefijo `SER_`.

## Macros disponibles

| Macro | Etiqueta | Uso |
|---|---|---|
| `SER_BOOT` | `[BOOT]` | Arranque del sistema |
| `SER_ALARMA` | `[ALARMA ACCESO]` | Intrusión detectada |
| `SER_FUEGO` | `[ALARMA INCENDIO]` | Humo sobre umbral |
| `SER_RFID` | `[RFID]` | Lectura de tarjeta |
| `SER_JUEGOS` | `[JUEGOS]` | Sala de juegos |
| `SER_HORNO` | `[HORNO]` | Horno |
| `SER_SONIDO` | `[SONIDO]` | Equipo de sonido |
| `SER_MERCADO` | `[MERCADO]` | Lista de mercado |
| `SER_EEPROM` | `[EEPROM]` | Errores de persistencia |
| `SER_SISTEMA` | `[SISTEMA]` | Heartbeat y estado del sistema |
| `SER_TECLADO` | `[TECLADO]` | Eventos de teclado |

## API recomendada

```c
#include "src/uart/uart.h"
#include "src/common/Definiciones.h"

// Evento simple
UART_WriteEvent(SER_ALARMA, "Intrusion detectada por PIR1");

// Texto + número
UART_WriteString(SER_FUEGO "Humo: ");
UART_WriteDecimal(73);
UART_WriteString("%");
UART_Newline();

// Formato: Humo sobre umbral: 73%
UART_WriteString(SER_FUEGO "Humo sobre umbral: ");
UART_WriteDecimal(smoke_percent);
UART_WriteString("%");
UART_Newline();
```

## Eventos obligatorios por módulo

### Seguridad

- `UART_WriteEvent(SER_ALARMA, "Intrusion detectada por PIR1")`
- `UART_WriteEvent(SER_ALARMA, "Intrusion detectada por PIR2")`
- `UART_WriteEvent(SER_ALARMA, "Codigo valido")`
- `UART_WriteEvent(SER_ALARMA, "Codigo invalido")`
- `UART_WriteEvent(SER_ALARMA, "Alarma desactivada")`
- `UART_WriteEvent(SER_FUEGO, "Humo sobre umbral: ")` + decimal + `"%"` + Newline
- `UART_WriteEvent(SER_FUEGO, "Codigo valido")`
- `UART_WriteEvent(SER_FUEGO, "Codigo invalido")`

### Accesos

- `UART_WriteEvent(SER_RFID, "UID reconocido: tipo=HIJO cupos=2")`
- `UART_WriteEvent(SER_RFID, "Acceso juegos permitido. Restantes=1")`
- `UART_WriteEvent(SER_RFID, "Acceso denegado: tarjeta no registrada")`
- `UART_WriteEvent(SER_RFID, "Usuario enrolado")`
- `UART_WriteEvent(SER_RFID, "Usuario borrado")`
- `UART_WriteEvent(SER_JUEGOS, "Cupo descontado. Restantes=1")`
- `UART_WriteEvent(SER_JUEGOS, "Cupos recargados: 10")`

### Confort

- `UART_WriteEvent(SER_SONIDO, "ON volumen=45%")`
- `UART_WriteEvent(SER_SONIDO, "OFF")`

### Remoto

- `UART_WriteEvent(SER_HORNO, "Encendido: temp=180C tiempo=300s")`
- `UART_WriteEvent(SER_HORNO, "Finalizado")`
- `UART_WriteEvent(SER_MERCADO, "Item agregado: ARROZ x2")`

### Sistema

- `UART_WriteEvent(SER_SISTEMA, "Heartbeat OK")`
- `UART_WriteEvent(SER_EEPROM, "Error de checksum")`
- `UART_WriteEvent(SER_EEPROM, "Datos invalidos — reiniciando")`

## Inyección de UID RFID por UART (simulación Proteus / pruebas)

El driver `rfid_rc522` es híbrido:

- **Fuente 1:** RC522 real por SPI con librería MFRC522 (`rfid_rc522_lib.cpp`). Si al arrancar no detecta el chip, se autodesactiva.
- **Fuente 2:** inyección de UID por UART0 (simulación Proteus): se teclea el UID en hex + Enter en el Monitor Serie.

El mismo firmware sirve en placa real y en Proteus (definir `RFID_SIMULATED=1` deshabilita el RC522 y deja solo la inyección por UART).

```
AABBCCDD<Enter>                  → UID corto de 4 bytes
04A1B2C3D4E5F6<Enter>            → UID de 7 bytes (NTAG / MIFARE actuales)
```

El RC522 real resuelve automáticamente UIDs de 4/7/10 bytes recorriendo la
cascada de anticolisión, y envía HALT tras leer para no re-leer la misma
tarjeta hasta que se retire del campo.

- La detección corre en paralelo con los menús: primero entrar al menú RFID
  (`B`), elegir la operación (p.ej. `4` Enrolar → `1` Padre / `2` Hijo) y luego
  inyectar el UID por la consola.
- Banderas de compilación (en `rfid_rc522.h`): `RFID_USE_RC522` y
  `RFID_UART_INJECT`. Definir `RFID_SIMULATED=1` deshabilita el SPI real
  (modo Proteus puro).

## Reglas

1. Los módulos funcionales NO tocan registros UART directamente.
2. Solo llaman funciones de `uart.h`: `UART_WriteString`, `UART_WriteDecimal`, `UART_Newline`, `UART_WriteEvent` (UART0) o las equivalentes `UART1_*` (respuestas remotas).
3. No incluir `\n` manual en los mensajes — usar `UART_Newline()` o `UART_WriteEvent()`.
4. UART0 y UART1 se atienden por sondeo en `UART_Task()` / `UART1_Task()` (buffers circulares, sin ISRs dedicadas en el driver propio).
5. La función `UART_WriteEvent(tag, msg)` equivale a: `WriteString(tag) + WriteString(msg) + Newline()`.
