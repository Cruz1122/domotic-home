/* ============================================================
 *  keypad.c — Driver de teclado matricial 4x4 con PCINT
 *
 *  Plataforma  : ATmega2560 / Arduino Mega 2560
 *  Interrupción: PCINT2 (columnas en PORTK PK0-PK3)
 *  Filas       : PORTL PL0-PL3 (D34-D37)
 *  Columnas    : PORTK PK0-PK3 (A8-A11)
 *  Lenguaje    : C puro, sin Keypad library, sin delay()
 *
 *  Arquitectura:
 *    - La ISR solo marca una bandera volatile
 *    - keypad_scan() procesa la bandera fuera de la ISR
 *    - Barrido de filas no bloqueante
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
 *  Rows:  D34=PL0, D35=PL1, D36=PL2, D37=PL3
 *  Cols:  D62=PK0, D63=PK1, D64=PK2, D65=PK3  (A8-A11)
 * ============================================================ */

/* Filas en PORTL (PL0-PL3, bits 0-3) — reposo LOW para permitir interrupción */
#define ROW_MASK      0x0F

/* Columnas en PORTK (PK0-PK3, bits 0-3) — entradas con pull-up */
#define COL_MASK      0x0F

/* Índices de fila: PL0 es bit 0 */
#define ROW_START_BIT  0

/* Tiempo de antirrebote en milisegundos */
#define KEYPAD_DEBOUNCE_MS  30


/* ============================================================
 *  MAPA DE TECLAS
 *
 *  Organización física del teclado 4x4:
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
 *
 *  Todas static — no expuestas fuera de este .c
 * ============================================================ */

/* Bandera volatile modificada desde ISR */
static volatile uint8_t scan_requested;

/* Control de antirrebote */
static uint8_t  debounce_active;
static uint32_t debounce_start_ms;

/* Máquina de eventos de tecla */
static uint8_t  key_was_pressed;   /* 1 = tecla detectada en barrido anterior */
static uint8_t  key_reported;      /* 1 = ya generamos evento para esta presión */
static char     pending_key;       /* Evento listo para keypad_get_key() */


/* ============================================================
 *  SCAN_MATRIX  —  Barrido interno de la matriz
 *
 *  Activa una fila a la vez (LOW), espera asentamiento,
 *  lee columnas, restaura fila a HIGH. Retorna el carácter
 *  de la tecla presionada, o '\0' si ninguna.
 *
 *  Al salir, todas las filas vuelven a LOW (reposo que
 *  permite detectar la siguiente pulsación por interrupción).
 *
 *  Cada fila se mantiene activa ~250 μs para que la
 *  conmutación sea visible en osciloscopio. Tiempo total
 *  de barrido ≈ 2 ms por escaneo.
 * ============================================================ */

static void settle_us(void) {
    /* ~250 μs a 16 MHz */
    for (volatile uint16_t i = 0; i < 2000; i++) {
        __asm__ __volatile__("nop" ::);
    }
}

static char scan_matrix(void) {
    uint8_t row;
    uint8_t col_val;

    /* Iniciar barrido: todas las filas HIGH temporalmente */
    PORTL |= ROW_MASK;
    settle_us();

    for (row = 0; row < 4; row++) {
        /* Activar esta fila: ponerla LOW */
        PORTL &= ~(1 << (ROW_START_BIT + row));
        settle_us();

        /* Leer columnas (PINK = PORTK input) */
        col_val = PINK & COL_MASK;

        /* Restaurar fila a HIGH */
        PORTL |= (1 << (ROW_START_BIT + row));
        settle_us();

        /* Si alguna columna está LOW, hay tecla presionada */
        if (col_val != COL_MASK) {
            uint8_t col;
            for (col = 0; col < 4; col++) {
                if (!(col_val & (1 << col))) {
                    /* Volver a reposo (filas LOW) antes de retornar */
                    PORTL &= ~ROW_MASK;
                    return key_map[row][col];
                }
            }
        }
    }

    /* Volver a reposo: filas LOW para permitir interrupción */
    PORTL &= ~ROW_MASK;
    return '\0';
}


/* ============================================================
 *  ISR — PCINT2_vect
 *
 *  Se dispara por cambio en PK0-PK3 (columnas A8-A11).
 *  Solo marca bandera. Sin barrido, sin antirrebote,
 *  sin Serial, sin LCD, sin lógica de menú.
 * ============================================================ */

ISR(PCINT2_vect) {
    scan_requested = 1;
}


/* ============================================================
 *  FUNCIONES PÚBLICAS
 * ============================================================ */

/* ----------------------------------------------------------
 *  keypad_init  —  Configura pines e interrupción PCINT2
 *
 *  1. Filas D34-D37 (PORTL PL0-PL3) como salidas, LOW
 *  2. Columnas D62-D65 (PORTK PK0-PK3) como entradas con pull-up
 *  3. Habilita PCINT2 para PK0-PK3 (PCINT16-PCINT19)
 *  4. Limpia banderas pendientes
 * ---------------------------------------------------------- */

void keypad_init(void) {
    /* --- Filas (D34-D37 = PL0-PL3) como salidas, LOW (reposo) --- */
    DDRL  |= ROW_MASK;       /* PL0-PL3 como salidas */
    PORTL &= ~ROW_MASK;      /* PL0-PL3 LOW → permite interrupción por cambio */

    /* --- Columnas (D62-D65 / A8-A11 = PK0-PK3) como entradas con pull-up --- */
    DDRK  &= ~COL_MASK;      /* PK0-PK3 como entradas */
    PORTK |= COL_MASK;       /* Pull-up interno activado → estado HIGH */

    /* --- Inicializar variables de estado --- */
    scan_requested   = 0;
    debounce_active  = 0;
    debounce_start_ms = 0;
    key_was_pressed  = 0;
    key_reported     = 0;
    pending_key      = '\0';

    /* --- Configurar PCINT2 para columnas (PK0-PK3) --- */

    /* Limpiar bandera de interrupción pendiente para PCINT2 */
    PCIFR |= (1 << PCIF2);

    /* Habilitar PCINT en PK0-PK3 (PCINT16-PCINT19) */
    PCMSK2 |= (1 << PCINT16)   /* PK0 */
            | (1 << PCINT17)   /* PK1 */
            | (1 << PCINT18)   /* PK2 */
            | (1 << PCINT19);  /* PK3 */

    /* Habilitar vector PCINT2 en PCICR */
    PCICR |= (1 << PCIE2);

    /* Asegurar interrupciones globales habilitadas */
    sei();
}


/* ----------------------------------------------------------
 *  keypad_scan  —  Procesa cambios de teclado
 *
 *  Solo escanea cuando scan_requested == 1 (ISR o forzado externo).
 *
 *  Flujo:
 *    1. Si scan_requested == 0 → salir.
 *    2. Si antirrebote no iniciado → iniciarlo y salir.
 *    3. Si antirrebote en curso → esperar.
 *    4. Si antirrebote cumplido → escanear matriz.
 *    5. Si tecla detectada y no reportada → guardar evento.
 *    6. Si tecla liberada → resetear estado.
 * ---------------------------------------------------------- */

void keypad_scan(uint32_t now_ms) {
    char current_key;

    /* --- Paso 1: solo si hay solicitud (ISR o forzado) --- */
    if (!scan_requested) {
        return;
    }

    /* --- Paso 2: iniciar antirrebote si no está activo --- */
    if (!debounce_active) {
        debounce_active   = 1;
        debounce_start_ms = now_ms;
        return;
    }

    /* --- Paso 3: esperar que pase el tiempo de antirrebote --- */
    if ((now_ms - debounce_start_ms) < KEYPAD_DEBOUNCE_MS) {
        return;
    }

    /* --- Paso 4: tiempo cumplido, escanear --- */
    debounce_active = 0;
    scan_requested  = 0;

    current_key = scan_matrix();

    /* --- Paso 5 y 6: máquina de eventos --- */
    if (current_key != '\0') {
        /* Tecla presionada */
        if (!key_was_pressed) {
            /* Nueva pulsación */
            key_was_pressed = 1;
            key_reported    = 0;
        }

        if (!key_reported) {
            /* Generar evento (solo una vez por pulsación) */
            pending_key   = current_key;
            key_reported  = 1;
        }
        /* Si ya reportado, no hacer nada (evita repetición) */
    } else {
        /* Tecla liberada */
        key_was_pressed = 0;
        key_reported    = 0;
    }
}


/* ----------------------------------------------------------
 *  keypad_get_key  —  Consume y retorna el evento de tecla
 *
 *  Retorna el carácter de la tecla ('0'-'9', 'A'-'D',
 *  '*', '#') o '\0' si no hay evento pendiente.
 *
 *  El evento se consume al leerlo.
 * ---------------------------------------------------------- */

char keypad_get_key(void) {
    char key = pending_key;
    pending_key = '\0';
    return key;
}
