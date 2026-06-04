# Reporte de eventos por UART

## Formato canónico

```
[TAG] mensaje\n
```

Todas las etiquetas se definen como macros en `src/common/Definiciones.h` con el prefijo `SER_`.

## Macros disponibles

| Macro | Etiqueta | Uso |
|---|---|---|
| `SER_BOOT` | `[BOOT]` | Arranque del sistema |
| `SER_LOGIN` | `[LOGIN]` | Resultado de login |
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
- `UART_WriteEvent(SER_ALARMA, "Alarma desactivada")`
- `UART_WriteEvent(SER_FUEGO, "Humo sobre umbral: ")` + decimal + `"%"` + Newline

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

## Reglas

1. Los módulos funcionales NO tocan registros UART ni la ISR.
2. Solo llaman funciones de `uart.h`: `UART_WriteString`, `UART_WriteDecimal`, `UART_Newline`, `UART_WriteEvent`.
3. No incluir `\n` manual en los mensajes — usar `UART_Newline()` o `UART_WriteEvent()`.
4. Los eventos se encolan en un buffer circular TX (128 bytes) y se transmiten por interrupción.
5. La función `UART_WriteEvent(tag, msg)` equivale a: `WriteString(tag) + WriteString(msg) + Newline()`.
