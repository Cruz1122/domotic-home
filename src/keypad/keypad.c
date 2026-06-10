#include "keypad.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

/*
 * Filas:   D34-D37 = PORTC PC3-PC0
 * Columnas: A8-A11  = PORTK PK0-PK3 = PCINT16-19
 *
 * IDLE:
 *   filas    = HIGH (1) -> 5V en D34-D37
 *   columnas = HIGH (1) por pull-up interno (sin cambios)
 *
 * DETECCION PCINT:
 *   cada keypad_scan() pulsa filas LOW ~3us
 *   si tecla presionada: columna cae a 0 -> PCINT2 dispara
 *   si no: columna queda en 1 -> nada
 *
 * ESCANEO (trigger PCINT):
 *   bajar UNA fila a LOW, leer columnas
 *   si columna LOW -> tecla en (fila, columna)
 *   restaurar fila a HIGH, repetir
 *
 * RELEASE:
 *   cuando key_down = 1, cada KEYPAD_SCAN_MS se ejecuta
 *   scan_matrix para detectar liberacion.
 *   Si scan_matrix devuelve '\0', key_down se resetea.
 */

#define ROW_MASK 0x0F    /* PORTC PC0-PC3 */
#define COL_MASK 0x0F    /* PORTK PK0-PK3 */

#define KEYPAD_DEBOUNCE_MS 25
#define KEYPAD_SCAN_MS      20

static const uint8_t row_bit[4] = {
    3, 2, 1, 0
};

static const char keymap[4][4] = {
    { '1', '2', '3', 'A' },
    { '4', '5', '6', 'B' },
    { '7', '8', '9', 'C' },
    { '*', '0', '#', 'D' }
};

static volatile uint8_t keypad_irq = 0;

static uint8_t  debounce_active = 0;
static uint32_t debounce_start_ms = 0;
static uint32_t last_scan_ms = 0;

static uint8_t key_down = 0;
static char pending_key = '\0';


static void settle(void) {
    for (volatile uint8_t i = 0; i < 40; i++) {
        __asm__ __volatile__("nop");
    }
}


/* Pulsa todas las filas LOW ~3us para que PCINT detecte caida */
static void strobe(void) {
    PORTC &= ~ROW_MASK;
    settle();
    PORTC |= ROW_MASK;
}


/* Escaneo: una fila LOW a la vez, detecta columnas LOW */
static char scan_matrix(void) {
    uint8_t row;
    uint8_t col;
    uint8_t cols;

    for (row = 0; row < 4; row++) {
        PORTC &= ~(1 << row_bit[row]);
        settle();

        cols = PINK & COL_MASK;

        if (cols != COL_MASK) {
            for (col = 0; col < 4; col++) {
                if ((cols & (1 << col)) == 0) {
                    PORTC |= (1 << row_bit[row]);
                    return keymap[row][col];
                }
            }
        }

        PORTC |= (1 << row_bit[row]);
    }

    return '\0';
}


ISR(PCINT2_vect) {
    keypad_irq = 1;
}


void keypad_init(void) {
    uint8_t jtd;

    /* JTAG disable: D34-D37 comparten con JTAG */
    jtd = MCUCR | (1 << JTD);
    MCUCR = jtd;
    MCUCR = jtd;

    /* Filas salida, HIGH reposo */
    DDRC  |= ROW_MASK;
    PORTC |= ROW_MASK;

    /* Columnas entrada con pull-up (SIN CAMBIOS) */
    DDRK  &= ~COL_MASK;
    PORTK |= COL_MASK;

    keypad_irq        = 0;
    debounce_active   = 0;
    debounce_start_ms = 0;
    last_scan_ms      = 0;
    key_down          = 0;
    pending_key       = '\0';

    /* PCINT2 en PK0-PK3 */
    PCIFR  |= (1 << PCIF2);
    PCMSK2 |= COL_MASK;
    PCICR  |= (1 << PCIE2);

    sei();
}


void keypad_scan(uint32_t now_ms) {
    char key;
    uint8_t do_scan = 0;

    /*
     * Strobe: pulsa filas LOW ~3us para que PCINT detecte
     * si hay tecla presionada. Pasa en CADA llamada.
     */
    strobe();

    /*
     * Tres caminos:
     * 1) PCINT disparo + key_down = 0  -> tecla nueva, antirrebote
     * 2) PCINT disparo + key_down = 1  -> tecla repetida, ignorar
     * 3) key_down = 1 + paso KEYPAD_SCAN_MS -> release check
     */

    if (keypad_irq) {
        if (!debounce_active) {
            debounce_active   = 1;
            debounce_start_ms = now_ms;
            return;
        }

        if ((uint32_t)(now_ms - debounce_start_ms) < KEYPAD_DEBOUNCE_MS) {
            return;
        }

        /* Antirrebote completo -> escanear */
        debounce_active = 0;
        keypad_irq     = 0;
        do_scan        = 1;
    }

    /*
     * Release check: si hay tecla presionada pero no llega PCINT
     * (porque la soltaron), escanea periodicamente para detectar
     * la liberacion.
     */
    if (!do_scan && key_down) {
        if ((uint32_t)(now_ms - last_scan_ms) >= KEYPAD_SCAN_MS) {
            do_scan = 1;
        }
    }

    if (!do_scan) {
        return;
    }

    /* Escaneo con PCINT pause para evitar basura */
    PCICR &= ~(1 << PCIE2);

    key = scan_matrix();

    PCIFR |= (1 << PCIF2);
    PCICR |= (1 << PCIE2);

    last_scan_ms = now_ms;

    if (key == '\0') {
        key_down = 0;
        return;
    }

    if (!key_down) {
        pending_key = key;
        key_down    = 1;
    }
}


char keypad_get_key(void) {
    char key = pending_key;
    pending_key = '\0';
    return key;
}
