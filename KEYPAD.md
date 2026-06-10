# Driver de Teclado Matricial 4×4 — `src/keypad/`

> Concatenación de toda referencia a KEYPAD en el repositorio.
> Fuentes: `KEYPAD.md` original, `src/keypad/keypad.c`, `src/keypad/keypad.h`,
> `src/common/Definiciones.h`, `docs/01_ARQUITECTURA.md`, `docs/02_HARDWARE_Y_PINOUT.md`,
> `rules.md`, `README.md`.

---

## Resumen

Driver de teclado matricial 4×4 para ATmega2560, implementado en **C puro** sin librería `Keypad`, sin `delay()` y sin bloqueos.

**Arquitectura:**
- **PCINT2** (`PCINT2_vect`) detecta cambio en columnas → marca `scan_requested`
- `keypad_scan()` en `loop()` barre filas **solo cuando hay tecla**
- Antirrebote por tiempo (30 ms)
- Un solo evento por pulsación
- Filas vuelven a LOW tras barrido (reposo)

---

## Referencias cruzadas en el repo

| Archivo | Líneas | Contenido |
|---------|--------|-----------|
| `src/keypad/keypad.c` | 1–184 | Implementación completa (PCINT) |
| `src/keypad/keypad.h` | 1–49 | Header / contrato público |
| `src/common/Definiciones.h` | 24–33 | `#define PIN_KEYPAD_*` |
| `docs/01_ARQUITECTURA.md` | 29, 123 | Árbol `src/` y `keypad_task()` en `loop()` |
| `docs/02_HARDWARE_Y_PINOUT.md` | 118–125 | Pinout KEYPAD |
| `rules.md` | 148, 160, 275, 406 | Prohibición de `Keypad`, drivers permitidos, `Keypad_Task()`, restricción UI |
| `README.md` | 22, 208–209 | Badge shields.io, regla de implementación |

---

## Mapa de teclas

```
         C1   C2   C3   C4
R1        1    2    3    A
R2        4    5    6    B
R3        7    8    9    C
R4        *    0    #    D
```

---

## Pinout

| Línea | Puerto | Pin Arduino | Notas |
|-------|--------|:-----------:|-------|
| Fila 1 (R1) | PC3 | D34 | Salida, LOW reposo |
| Fila 2 (R2) | PC2 | D35 | Salida, LOW reposo |
| Fila 3 (R3) | PC1 | D36 | Salida, LOW reposo |
| Fila 4 (R4) | PC0 | D37 | Salida, LOW reposo |
| Columna 1 (C1) | PK0 | A8 / D62 | Entrada pull-up, PCINT16 |
| Columna 2 (C2) | PK1 | A9 / D63 | Entrada pull-up, PCINT17 |
| Columna 3 (C3) | PK2 | A10 / D64 | Entrada pull-up, PCINT18 |
| Columna 4 (C4) | PK3 | A11 / D65 | Entrada pull-up, PCINT19 |

*(Fuente: `src/common/Definiciones.h` líneas 24–33, `docs/02_HARDWARE_Y_PINOUT.md` líneas 118–125)*

---

## API pública

```c
void keypad_init(void);
void keypad_scan(uint32_t now_ms);
char keypad_get_key(void);
```

*(Fuente: `src/keypad/keypad.h`)*

### `keypad_init()`

Configura pines y PCINT. Se llama una vez en `setup()`.

1. **Filas** D34–D37 (`PORTC` PC0–PC3) como **salidas**, inicialmente **LOW** (reposo)
2. **Columnas** A8–A11 (`PORTK` PK0–PK3) como **entradas con pull-up**
3. Habilita **PCINT2** para `PCINT16..PCINT19` (PK0–PK3 / columnas)
4. Habilita interrupciones (`sei()`)

### `keypad_scan(now_ms)`

Tarea cooperativa llamada desde `loop()`. Procesa detección **solo cuando PCINT marcó `scan_requested`**.

**Flujo:**
1. Si `scan_requested == 0` → salir (sin acción)
2. Si antirrebote no iniciado → iniciar timer de 30 ms
3. Si antirrebote en curso → esperar
4. Si antirrebote cumplido → ejecutar `scan_matrix()` (barrido completo 4 filas)
5. Si detecta tecla y no reportada → guardar evento en `pending_key`
6. Si no hay tecla → resetear banderas

### `keypad_get_key()`

Retorna el carácter de la tecla presionada (`'0'`–`'9'`, `'A'`–`'D'`, `'*'`, `'#'`) y **consume** el evento. Si no hay evento pendiente, retorna `'\0'`.

---

## Arquitectura

```
                     PCINT2 (PK0-PK3)
┌─────────────┐  ┌──────────────────┐
│  Teclado     │  │   ATmega2560     │
│  4×4         │  │                  │
│  Filas ──────▶│  PORTC (PC0-PC3)  │  salidas, LOW reposo
│  Cols  ◀──────│  PORTK (PK0-PK3)  │  entradas pull-up
└─────────────┘  │  PCINT2_vect      │  detecta cambio
                 └──────────────────┘
```

### Reposo (idle)

- **Filas**: LOW (GND) — `PORTC &= ~ROW_MASK`
- **Columnas**: HIGH (pull-up interno) — `PORTK |= COL_MASK`

### Pulsación de tecla

Al presionar una tecla, conecta fila (LOW) con columna (pull-up).
La columna baja a LOW → dispara **PCINT2** → `scan_requested = 1`.

### PCINT ISR (`PCINT2_vect`)

```c
ISR(PCINT2_vect) {
    scan_requested = 1;
}
```

Mínimo: solo marca bandera. Sin barrido, sin esperas.

### Barrido completo (`scan_matrix()`)

Solo se ejecuta cuando `scan_requested` está activo y el antirrebote ha expirado.

```
1. Subir TODAS filas a HIGH
2. scan_settle()  (~2000 NOPs)
3. Para cada fila (0..3):
   a. Bajar fila i a LOW
   b. scan_settle()
   c. Leer columnas (PINK & 0x0F)
   d. Subir fila i a HIGH
   e. scan_settle()
   f. Si columna LOW → tecla detectada
4. Si tecla → retornar key_map[row][col]
5. Si no tecla → bajar todas filas a LOW, retornar '\0'
```

### Antirrebote

30 ms por tiempo. Al detectar tecla, inicia timer. Solo si la misma tecla persiste tras 30 ms y barrido completo, se genera el evento.

### Liberación de tecla

Cuando se suelta, la columna vuelve a HIGH. `scan_matrix()` retorna `'\0'` → se resetean `key_was_pressed` y `key_reported`.

---

## Diagrama de flujo

```
PCINT2_vect (cambio en columnas)
  │
  └── scan_requested = 1 (bandera)

loop()
  │
  ├── keypad_scan(now_ms)
  │     │
  │     ├── ¿scan_requested? ──NO──→ return
  │     │
  │     ├── ¿debounce_active? ──NO──→ iniciar 30ms → return
  │     │
  │     ├── ¿cumplió 30ms? ──NO──→ return
  │     │
  │     ├── scan_matrix()
  │     │     ├── Filas HIGH → settle
  │     │     ├── F0: LOW settle → leer → HIGH settle
  │     │     ├── F1: LOW settle → leer → HIGH settle
  │     │     ├── F2: LOW settle → leer → HIGH settle
  │     │     └── F3: LOW settle → leer → HIGH settle → LOW
  │     │
  │     ├── ¿tecla? ──SÍ──→ guardar evento si no reportada
  │     │
  │     └── ¿no tecla? ──→ resetear flags
  │
  ├── keypad_get_key()
  │     └── retorna pending_key, lo consume
  │
  └── otras tareas
```

---

## Código fuente

### `src/keypad/keypad.h`

```c
#ifndef KEYPAD_H
#define KEYPAD_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 *  keypad.h — Driver de teclado matricial 4x4
 *
 *  Plataforma  : ATmega2560 / Arduino Mega 2560
 *  Lenguaje    : C puro, sin Keypad library
 *
 *  Interrupción: PCINT2 (columnas en PORTK PK0-PK3)
 *
 *  Arquitectura:
 *    - Reposo: filas LOW, columnas HIGH (pull-up)
 *    - Pulsar tecla baja columna → PCINT2 marca scan_requested
 *    - keypad_scan() barre filas solo si hay interrupción
 *    - keypad_get_key() consume el evento
 *
 *  Mapa de teclas (filas x columnas):
 *               C1  C2  C3  C4
 *         R1    1   2   3   A
 *         R2    4   5   6   B
 *         R3    7   8   9   C
 *         R4    *   0   #   D
 *
 *  Conexiones:
 *    Filas    D34-D37 (PORTC PC3-PC0) — salidas, LOW reposo
 *    Columnas A8-A11 (PORTK PK0-PK3)  — entradas pull-up
 *
 *  Contrato:
 *    - keypad_init() se llama una vez en setup()
 *    - keypad_scan() se llama desde loop() con millis() actual
 *    - keypad_get_key() consume el evento
 * ============================================================ */

void keypad_init(void);
void keypad_scan(uint32_t now_ms);
char keypad_get_key(void);

#ifdef __cplusplus
}
#endif

#endif /* KEYPAD_H */
```

### `src/keypad/keypad.c`

```c
/* ============================================================
 *  keypad.c — Driver de teclado matricial 4x4 con PCINT
 *
 *  Plataforma  : ATmega2560 / Arduino Mega 2560
 *  Interrupción: PCINT2 (columnas en PORTK PK0-PK3)
 *  Filas       : PORTC PC0-PC3 (D37-D34)
 *  Columnas    : PORTK PK0-PK3 (A8-A11)
 *  Lenguaje    : C puro, sin Keypad library, sin delay()
 *
 *  Arquitectura:
 *    - Reposo: filas LOW, columnas HIGH (pull-up)
 *    - Pulsar tecla baja columna → PCINT2 marca scan_requested
 *    - keypad_scan() barre filas solo si hay interrupción
 *    - Tras barrido: filas vuelven a LOW (reposo)
 *    - Antirrebote por tiempo (30 ms)
 *    - Un solo evento por pulsación
 * ============================================================ */

#include "keypad.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>


/* ============================================================
 *  MAPEO DE PINES A REGISTROS
 *
 *  Rows:  D34=PC3, D35=PC2, D36=PC1, D37=PC0
 *  Cols:  A8=PK0, A9=PK1, A10=PK2, A11=PK3
 * ============================================================ */

#define ROW_MASK      0x0F   /* PC0-PC3 */
#define COL_MASK      0x0F   /* PK0-PK3 */

#define KEYPAD_DEBOUNCE_MS  30

/* R1..R4 en pines 34..37 → PC3..PC0 */
static const uint8_t row_bit[4] = {3, 2, 1, 0};


/* ============================================================
 *  MAPA DE TECLAS
 *
 *               C1   C2   C3   C4
 *         R1     1    2    3    A
 *         R2     4    5    6    B
 *         R3     7    8    9    C
 *         R4     *    0    #    D
 * ============================================================ */

static const char key_map[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};


/* ============================================================
 *  VARIABLES DE ESTADO INTERNAS
 * ============================================================ */

static volatile uint8_t  scan_requested;
static uint8_t  debounce_active;
static uint32_t debounce_start_ms;
static uint8_t  key_was_pressed;
static uint8_t  key_reported;
static char     pending_key;


static void scan_settle(void) {
    for (volatile uint16_t i = 0; i < 2000; i++) {
        __asm__ __volatile__("nop" ::);
    }
}


static char scan_matrix(void) {
    uint8_t row;
    uint8_t col_val;
    uint8_t bit;

    PORTC |= ROW_MASK;
    scan_settle();

    for (row = 0; row < 4; row++) {
        bit = row_bit[row];

        PORTC &= ~(1 << bit);
        scan_settle();

        col_val = PINK & COL_MASK;

        PORTC |= (1 << bit);
        scan_settle();

        if (col_val != COL_MASK) {
            uint8_t col;
            for (col = 0; col < 4; col++) {
                if (!(col_val & (1 << col))) {
                    PORTC &= ~ROW_MASK;
                    return key_map[row][col];
                }
            }
        }
    }

    PORTC &= ~ROW_MASK;
    return '\0';
}


ISR(PCINT2_vect) {
    scan_requested = 1;
}


void keypad_init(void) {
    DDRC  |= ROW_MASK;
    PORTC &= ~ROW_MASK;

    DDRK  &= ~COL_MASK;
    PORTK |= COL_MASK;

    scan_requested    = 0;
    debounce_active   = 0;
    debounce_start_ms = 0;
    key_was_pressed   = 0;
    key_reported      = 0;
    pending_key       = '\0';

    PCIFR  |= (1 << PCIF2);
    PCMSK2 |= (1 << PCINT16)
            | (1 << PCINT17)
            | (1 << PCINT18)
            | (1 << PCINT19);
    PCICR  |= (1 << PCIE2);

    sei();
}


void keypad_scan(uint32_t now_ms) {
    char current_key;

    if (!scan_requested) {
        return;
    }

    if (!debounce_active) {
        debounce_active   = 1;
        debounce_start_ms = now_ms;
        return;
    }

    if ((now_ms - debounce_start_ms) < KEYPAD_DEBOUNCE_MS) {
        return;
    }

    debounce_active = 0;
    scan_requested  = 0;

    current_key = scan_matrix();

    if (current_key != '\0') {
        if (!key_was_pressed) {
            key_was_pressed = 1;
            key_reported    = 0;
        }
        if (!key_reported) {
            pending_key  = current_key;
            key_reported = 1;
        }
    } else {
        key_was_pressed = 0;
        key_reported    = 0;
    }
}


char keypad_get_key(void) {
    char key = pending_key;
    pending_key = '\0';
    return key;
}
```

---

## `src/common/Definiciones.h` — Extracto KEYPAD

```c
/* --- Teclado matricial 4x4 --- */
/* Filas D34-D37 (PORTC PC3-PC0), Columnas A8-A11 (PORTK PK0-PK3) */
#define PIN_KEYPAD_R1             34   /* PC3 */
#define PIN_KEYPAD_R2             35   /* PC2 */
#define PIN_KEYPAD_R3             36   /* PC1 */
#define PIN_KEYPAD_R4             37   /* PC0 */
#define PIN_KEYPAD_C1             62   /* A8 / PK0 / PCINT16 */
#define PIN_KEYPAD_C2             63   /* A9 / PK1 / PCINT17 */
#define PIN_KEYPAD_C3             64   /* A10 / PK2 / PCINT18 */
#define PIN_KEYPAD_C4             65   /* A11 / PK3 / PCINT19 */
```

*(Fuente: `src/common/Definiciones.h` líneas 24–33)*

---

## `docs/01_ARQUITECTURA.md` — Extracto KEYPAD

**Árbol de src/ (línea 29):**
```
  keypad/         teclado matricial 4x4
```

**loop() (línea 123):**
```c
    keypad_task();
```

*(Fuente: `docs/01_ARQUITECTURA.md`)*

---

## `docs/02_HARDWARE_Y_PINOUT.md` — Extracto KEYPAD

```c
#define PIN_KEYPAD_R1            34   /* PC3 */
#define PIN_KEYPAD_R2            35   /* PC2 */
#define PIN_KEYPAD_R3            36   /* PC1 */
#define PIN_KEYPAD_R4            37   /* PC0 */
#define PIN_KEYPAD_C1            62   /* PK0 / A8 */
#define PIN_KEYPAD_C2            63   /* PK1 / A9 */
#define PIN_KEYPAD_C3            64   /* PK2 / A10 */
#define PIN_KEYPAD_C4            65   /* PK3 / A11 */
```

*(Fuente: `docs/02_HARDWARE_Y_PINOUT.md` líneas 118–125)*

---

## `rules.md` — Extracto KEYPAD

**Librerías prohibidas (línea 148):**
```
- `Keypad`
```

**Drivers propios permitidos (línea 160):**
```
- `keypad.c` / `keypad.h`
```

**Task en loop (línea 275):**
```c
    Keypad_Task(now_ms);
```

**Restricción UI (línea 406):**
```
- No usar `Keypad`.
```

*(Fuente: `rules.md`)*

---

## `README.md` — Extracto KEYPAD

**Badge (línea 22):**
```html
<img src="https://img.shields.io/badge/Keypad-4x4-555555?labelColor=455A64" alt="Teclado matricial" />
```

**Regla de implementación (líneas 208–209):**
```markdown
- No usar librerías externas como `MFRC522`, `LiquidCrystal`, `Servo`, `Keypad`, `EEPROM` o `SPI`.
- Sí se permiten módulos propios: `spi.c`, `uart.c`, `lcd.c`, `keypad.c`, `adc.c`, `pwm.c`, `timer.c`, `eeprom.c`, `rfid_rc522.c`.
```

*(Fuente: `README.md`)*

---

## Ejemplo de uso (`domotic-home.ino`)

```cpp
#include <Arduino.h>
#include "src/common/Definiciones.h"
#include "src/keypad/keypad.h"
// ... otros módulos

void setup(void) {
    Serial.begin(9600);
    keypad_init();
    // ...
}

void loop(void) {
    uint32_t now_ms = millis();
    char tecla;

    keypad_scan(now_ms);
    // ... otras tareas

    tecla = keypad_get_key();
    if (tecla != '\0') {
        Serial.print("[TECLADO] Tecla: ");
        Serial.println(tecla);
    }
}
```

---

## Tiempos

| Evento | Tiempo máximo |
|--------|--------------:|
| Pulsar → PCINT → scan_requested | < 1 μs |
| PCINT → keypad_scan() (próximo loop) | variable (~μs) |
| keypad_scan() → antirrebote (wait) | 30 ms |
| Antirrebote → barrido completo | 0 ms (inmediato) |
| Barrido → evento | 0 ms (inmediato) |
| **Total pulsación → evento** | **~30 ms + 1 ciclo loop** |
| Soltar → reset interno | próximo scan |

---

## Criterios de aceptación cumplidos

- [x] Driver propio en C puro
- [x] Sin librería `Keypad`
- [x] Sin `delay()` ni `_delay_ms()`
- [x] Sin `while()` bloqueante
- [x] No bloquea `loop()`
- [x] Un solo evento por pulsación
- [x] No repite tecla mientras presionada
- [x] Permite nueva tecla tras soltar y represionar
- [x] `keypad_get_key()` consume el evento
- [x] Soporta `0-9`, `A-D`, `*` y `#`
- [x] Antirrebote por tiempo (30 ms)
- [x] PCINT para detección asíncrona
- [x] Filas LOW en reposo
- [x] Columnas con pull-up
- [x] Barrido completo bajo demanda
- [x] Prueba por Serial fuera del driver
- [x] La prueba por Serial se hace fuera de `keypad.c`

---

> **Nota:** Este archivo consolida TODAS las referencias a KEYPAD en el repositorio.
> La implementación activa usa **PCINT** (`keypad.c` en disco).  
> La documentación anterior (Timer1 ISR) ha sido reemplazada para reflejar el código real.
