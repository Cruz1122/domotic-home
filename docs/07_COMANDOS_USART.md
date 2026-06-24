# Comandos USART Remotos

## Objetivo

Este documento resume todos los comandos remotos disponibles por `UART1 / Serial1`, su sintaxis, ejemplos y respuesta esperada.

Se usa durante la demo para operar el sistema desde un `Virtual Terminal` limpio, separado del canal de debug. También funciona desde el Monitor Serie de Arduino IDE gracias al bridge UART0→UART1.

## Roles de puertos

- `UART0 / Serial`: debug interno, boot, trazas y eventos del sistema. También acepta comandos remotos vía bridge (ver abajo).
- `UART1 / Serial1`: control remoto del usuario (Virtual Terminal en Proteus).

`UART2 / Serial2` y `UART3 / Serial3` quedan libres, sin uso funcional en la aplicación.

## Configuración del terminal remoto

- Baud: `9600`
- Formato: `8N1`
- Terminal TX -> `RX1 / PD2 / pin 19`
- Terminal RX <- `TX1 / PD3 / pin 18`
- GND común

## Formato de respuestas

Todas las respuestas observables por `UART1` usan prefijo:

- `[OK]` operación exitosa
- `[ERR]` error de uso o validación
- `[STATUS]` consulta o listado

Formato general:

```text
[PREFIJO][MODULO] mensaje
```

Ejemplos:

```text
[OK][RADIO] ON
[ERR][HORNO] Temp fuera de rango
[STATUS][MERCADO] 2 items
```

## Reglas del parser

- No distingue entre mayúsculas y minúsculas.
- Tolera espacios extra entre tokens.
- Procesa una línea al recibir `\r` o `\n`.
- Si la línea es demasiado larga, responde:

```text
[ERR][SISTEMA] Linea demasiado larga
```

## Comandos disponibles

### HELP

Muestra el resumen de comandos disponibles.

Sintaxis:

```text
HELP
```

Ejemplo:

```text
HELP
```

Respuesta esperada:

```text
[STATUS][SISTEMA] Comandos:
[STATUS][SISTEMA] RADIO ON|OFF|VOL <0-100>|STATUS
[STATUS][SISTEMA] HORNO ON <temp> <min>|OFF|STATUS
[STATUS][SISTEMA] MERCADO PRODUCTS|ADD <id> <qty>|LIST|CLEAR|STATUS
```

### RADIO ON

Enciende la salida de sonido (estado lógico). Si ya existe un volumen remoto almacenado, el LCD lo muestra al consultar.

Sintaxis:

```text
RADIO ON
```

Respuesta esperada:

```text
[OK][RADIO] ON
```

### RADIO OFF

Apaga la salida de sonido (estado lógico). El último volumen remoto almacenado se conserva para consultas.

Sintaxis:

```text
RADIO OFF
```

Respuesta esperada:

```text
[OK][RADIO] OFF
```

### RADIO STATUS

Consulta si la radio está encendida y el porcentaje de volumen remoto actual.

Sintaxis:

```text
RADIO STATUS
```

Respuesta esperada:

```text
[STATUS][RADIO] ON VOL=73
```

o

```text
[STATUS][RADIO] OFF VOL=73
```

### RADIO VOL

Actualiza el setpoint remoto de volumen. El rango válido es `0` a `100`. Se refleja en LCD y eventos `[SONIDO]` por UART0; no hay salida PWM física de volumen.

Sintaxis:

```text
RADIO VOL 80
```

Respuesta esperada:

```text
[OK][RADIO] VOL=80
```

Errores posibles:

```text
RADIO VOL
[ERR][RADIO] Uso RADIO VOL <0-100>

RADIO VOL 120
[ERR][RADIO] Volumen fuera de rango
```

### HORNO ON

Enciende el horno con temperatura y tiempo configurados por terminal.

Sintaxis:

```text
HORNO ON <temp> <min>
```

Rangos válidos:

- temperatura: `10` a `300`
- tiempo: `1` a `180`

Ejemplo:

```text
HORNO ON 180 2
```

Respuesta esperada:

```text
[OK][HORNO] ON TEMP=180 MIN=2
```

Notas:

- En demo, `1 minuto lógico = 5 segundos reales`.
- Al encender, se activa `PIN_OVEN_LED`.

### HORNO OFF

Apaga el horno manualmente.

Sintaxis:

```text
HORNO OFF
```

Respuesta esperada:

```text
[OK][HORNO] OFF
```

### HORNO STATUS

Consulta el estado del horno, la temperatura objetivo y el tiempo restante.

Sintaxis:

```text
HORNO STATUS
```

Respuesta esperada:

```text
[STATUS][HORNO] ON TEMP=180 RESTANTE=2min
```

o

```text
[STATUS][HORNO] OFF TEMP=0 RESTANTE=0min
```

### Errores posibles de HORNO ON

Uso incompleto:

```text
HORNO ON 180
```

Respuesta:

```text
[ERR][HORNO] Uso HORNO ON <temp> <min>
```

Temperatura inválida:

```text
HORNO ON 500 2
```

Respuesta:

```text
[ERR][HORNO] Temp fuera de rango
```

Tiempo inválido:

```text
HORNO ON 180 0
```

Respuesta:

```text
[ERR][HORNO] Tiempo fuera de rango
```

Parámetros no numéricos:

```text
HORNO ON abc 2
```

Respuesta:

```text
[ERR][HORNO] Parametros invalidos
```

Fin automático:

```text
[OK][HORNO] Finalizado
```

### MERCADO PRODUCTS

Lista todos los productos válidos con su ID.

Sintaxis:

```text
MERCADO PRODUCTS
```

Respuesta esperada:

```text
[STATUS][MERCADO] Productos:
[STATUS][MERCADO] 1 Pan
[STATUS][MERCADO] 2 Leche
[STATUS][MERCADO] 3 Huevos
[STATUS][MERCADO] 4 Arroz
[STATUS][MERCADO] 5 Cafe
[STATUS][MERCADO] 6 Azucar
[STATUS][MERCADO] 7 Aceite
[STATUS][MERCADO] 8 Fruta
```

### MERCADO ADD

Agrega un producto a la lista o acumula cantidad si ya existe.

Sintaxis:

```text
MERCADO ADD <id> <qty>
```

Rangos válidos:

- `id`: `1` a `8`
- `qty`: `1` a `99`

Ejemplos:

```text
MERCADO ADD 1 2
MERCADO ADD 1 3
MERCADO ADD 4 1
```

Respuestas esperadas:

```text
[OK][MERCADO] Pan x2
[OK][MERCADO] Pan x5
[OK][MERCADO] Arroz x1
```

Errores posibles:

```text
MERCADO ADD 9 1
[ERR][MERCADO] Producto invalido

MERCADO ADD 1 0
[ERR][MERCADO] Cantidad invalida

MERCADO ADD 1
[ERR][MERCADO] Uso MERCADO ADD <id> <qty>
```

### MERCADO LIST

Muestra el contenido actual de la lista.

Sintaxis:

```text
MERCADO LIST
```

Respuesta esperada con elementos:

```text
[STATUS][MERCADO] 2 items
[STATUS][MERCADO] 1) Pan x5
[STATUS][MERCADO] 2) Arroz x1
```

Respuesta esperada vacía:

```text
[STATUS][MERCADO] 0 items
[STATUS][MERCADO] Lista vacia
```

### MERCADO STATUS

Es alias de `MERCADO LIST`.

Sintaxis:

```text
MERCADO STATUS
```

### MERCADO CLEAR

Vacía la lista completa y guarda el cambio en EEPROM.

Sintaxis:

```text
MERCADO CLEAR
```

Respuesta esperada:

```text
[OK][MERCADO] Lista limpia
```

## Comandos inválidos

Cualquier comando desconocido responde:

```text
[ERR][SISTEMA] Comando desconocido. Escriba HELP
```

Ejemplo:

```text
XYZ
```

## Guion corto de demo

```text
HELP
RADIO STATUS
RADIO ON
RADIO STATUS
RADIO VOL 80
RADIO OFF
HORNO STATUS
HORNO ON 180 2
HORNO STATUS
MERCADO PRODUCTS
MERCADO ADD 1 2
MERCADO ADD 1 3
MERCADO ADD 4 1
MERCADO LIST
MERCADO CLEAR
MERCADO LIST
```

## Qué debe verse en UART0

`UART0` muestra:

- boot
- trazas internas (`[SISTEMA]`, `[RFID]`, etc.)
- eventos del sistema
- logs de debug de acciones remotos (`[DBG][RADIO]`, etc.)
- respuestas de comandos remotos (replicadas desde UART1)

Los comandos remotos también pueden enviarse por UART0: el bridge los reenvía al parser. En Proteus con Virtual Terminal en UART1, las respuestas limpias aparecen en ambos canales.

## Archivo relacionado

- `docs/06_UART_EVENTOS.md`: formato de eventos UART y roles de puertos
