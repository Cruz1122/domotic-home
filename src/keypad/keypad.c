#include "keypad.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/*
 * Filas:   D34-D37 = PORTC PC3-PC0
 * Columnas: A8-A11  = PORTK PK0-PK3 = PCINT16-19
 *
 * ARQUITECTURA (herencia del proyecto domotic-home industrial):
 *
 *   Rotacion continua del 0 por filas (tres 1, un 0 rotando).
 *   PCINT2 detecta cambio en columnas (flanco de pulsacion).
 *
 *   Maquina de estados:
 *     IDLE             - rotacion normal, espera bandera PCINT
 *     DEBOUNCE_PRESS   - antirrebote 20 ms, luego scan completo
 *     PRESSED          - tecla reportada, espera liberacion
 *     DEBOUNCE_RELEASE - antirrebote 20 ms tras liberar
 *
 *   Un solo evento por pulsacion mantenida.
 */

/* =========================================================
   CONFIGURACION
   ========================================================= */

#define DEBOUNCE_MS             20U

/* Filas: PC3-PC0 */
#define ROW_R1                  3U
#define ROW_R2                  2U
#define ROW_R3                  1U
#define ROW_R4                  0U
#define ROW_MASK      ((1U<<ROW_R1)|(1U<<ROW_R2)|(1U<<ROW_R3)|(1U<<ROW_R4))

/* Columnas: PK0-PK3 = PCINT16-19 */
#define COL_C1                  0U
#define COL_C2                  1U
#define COL_C3                  2U
#define COL_C4                  3U
#define COL_MASK      ((1U<<COL_C1)|(1U<<COL_C2)|(1U<<COL_C3)|(1U<<COL_C4))

/* =========================================================
   MAQUINA DE ESTADOS
   ========================================================= */

#define KEYPAD_IDLE               0U
#define KEYPAD_DEBOUNCE_PRESS     1U
#define KEYPAD_PRESSED            2U
#define KEYPAD_DEBOUNCE_RELEASE   3U

/* =========================================================
   DATOS ESTATICOS
   ========================================================= */

static const uint8_t g_row_patterns[4] = {
    (uint8_t)(ROW_MASK & ~(1U << ROW_R1)),
    (uint8_t)(ROW_MASK & ~(1U << ROW_R2)),
    (uint8_t)(ROW_MASK & ~(1U << ROW_R3)),
    (uint8_t)(ROW_MASK & ~(1U << ROW_R4))
};

static const char g_keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static volatile uint8_t g_edge_flag = 0;

static uint8_t  g_state            = KEYPAD_IDLE;
static uint8_t  g_active_row       = 0;
static uint8_t  g_key_available    = 0;
static char     g_key_value        = 0;

/* Temporizadores (usamos now_ms en Keypad_Scan) */
static uint32_t g_last_row_ms      = 0;
static uint32_t g_debounce_start   = 0;

/* =========================================================
   FUNCIONES INTERNAS
   ========================================================= */

static void KEYPAD_drive_row(uint8_t row)
{
    if (row > 3U) {
        row = 0;
    }
    PORTC = (PORTC & ~ROW_MASK) | g_row_patterns[row];
    g_active_row = row;
}

/*
 * Barrido completo: prueba una fila a la vez (LOW), lee columnas.
 * Guarda y restaura la fila activa para no interferir con la rotacion.
 */
static char KEYPAD_scan_key(void)
{
    uint8_t saved_row = g_active_row;
    char found = 0;

    for (uint8_t row = 0; row < 4U; row++) {
        PORTC = (PORTC & ~ROW_MASK) | g_row_patterns[row];
        __asm__ __volatile__("nop\n\t""nop\n\t""nop\n\t");
        uint8_t cols = PINK & COL_MASK;
        if (cols != COL_MASK) {
            uint8_t col = 0xFFU;
            if (!(cols & (1U << COL_C1))) col = 0U;
            else if (!(cols & (1U << COL_C2))) col = 1U;
            else if (!(cols & (1U << COL_C3))) col = 2U;
            else if (!(cols & (1U << COL_C4))) col = 3U;
            if (col < 4U) {
                found = g_keymap[row][col];
                break;
            }
        }
    }

    KEYPAD_drive_row(saved_row);
    return found;
}

/*
 * Consume y retorna la bandera de interrupcion de forma atomica.
 */
static uint8_t KEYPAD_take_edge(void)
{
    uint8_t edge;
    uint8_t sreg = SREG;
    cli();
    edge = g_edge_flag;
    g_edge_flag = 0;
    SREG = sreg;
    return edge;
}

/* =========================================================
   INTERRUPCION PCINT2
   Detecta cambio en columnas PK0-PK3 (PCINT16-19).
   ========================================================= */

ISR(PCINT2_vect)
{
    g_edge_flag = 1;
}

/* =========================================================
   API PUBLICA
   ========================================================= */

void Keypad_Init(void)
{
    uint8_t jtd;

    cli();

    /* JTAG disable: PORTC PC2-PC3 comparten con JTAG */
    jtd = MCUCR | (1U << JTD);
    MCUCR = jtd;
    MCUCR = jtd;

    /* Filas como salidas, HIGH en reposo */
    DDRC  |= ROW_MASK;
    PORTC |= ROW_MASK;

    /* Columnas como entradas con pull-up */
    DDRK  &= ~COL_MASK;
    PORTK |= COL_MASK;

    /* Rotacion inicial: fila 0 LOW */
    g_active_row = 0;
    KEYPAD_drive_row(0);

    g_state          = KEYPAD_IDLE;
    g_edge_flag      = 0;
    g_key_available  = 0;
    g_key_value      = 0;
    g_last_row_ms    = 0;
    g_debounce_start = 0;

    /* PCINT2 en PK0-PK3 (PCINT16-19) */
    PCIFR  |= (1U << PCIF2);
    PCMSK2 |= COL_MASK;
    PCICR  |= (1U << PCIE2);

    sei();
}

void Keypad_Scan(uint32_t now_ms)
{
    uint32_t elapsed;

    /* ---- Rotacion continua (todos los estados) ---- */
    if ((uint32_t)(now_ms - g_last_row_ms) >= 2U) {
        g_last_row_ms = now_ms;
        uint8_t next = (uint8_t)((g_active_row + 1U) & 0x03U);
        KEYPAD_drive_row(next);
    }

    /* ---- Maquina de estados ---- */
    switch (g_state) {
        case KEYPAD_IDLE:
            if (KEYPAD_take_edge()) {
                g_debounce_start = now_ms;
                g_state = KEYPAD_DEBOUNCE_PRESS;
            }
            break;

        case KEYPAD_DEBOUNCE_PRESS:
            elapsed = (uint32_t)(now_ms - g_debounce_start);
            if (elapsed >= DEBOUNCE_MS) {
                char key = KEYPAD_scan_key();
                if (key != 0) {
                    g_key_value = key;
                    g_key_available = 1;
                    g_state = KEYPAD_PRESSED;
                } else {
                    g_state = KEYPAD_IDLE;
                }
            }
            break;

        case KEYPAD_PRESSED:
            if (KEYPAD_scan_key() == 0) {
                g_debounce_start = now_ms;
                g_state = KEYPAD_DEBOUNCE_RELEASE;
            }
            break;

        case KEYPAD_DEBOUNCE_RELEASE:
            elapsed = (uint32_t)(now_ms - g_debounce_start);
            if (elapsed >= DEBOUNCE_MS) {
                if (KEYPAD_scan_key() == 0) {
                    g_state = KEYPAD_IDLE;
                } else {
                    g_state = KEYPAD_PRESSED;
                }
            }
            break;

        default:
            g_state = KEYPAD_IDLE;
            break;
    }
}

char Keypad_GetKey(void)
{
    char key = g_key_value;
    g_key_value = 0;
    g_key_available = 0;
    return key;
}
