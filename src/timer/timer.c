/*
 * Módulo: Timer — implementación
 * Timer5 genera un tick de 1 ms que todas las tareas usan como reloj.
 * Ese tick alimenta los temporizadores no bloqueantes del sistema.
 */
#include "timer.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

/* Contador global de milisegundos. Volátil: la ISR lo incrementa. */
static volatile uint32_t current_tick;

/* ISR de comparación de Timer5: corre cada 1 ms y suma un tick. */
ISR(TIMER5_COMPA_vect) {
    current_tick++;
}

void Timer_Init(void) {
    uint8_t sreg = SREG;
    cli();

    current_tick = 0;
    TCCR5A = 0;
    TCCR5B = 0;
    TCNT5 = 0;
    /* CTC: cuenta hasta OCR5A=249. Con prescaler /64 y F_CPU=16 MHz -> 1 ms. */
    OCR5A = 249;
    TIFR5 = (1 << OCF5A);
    TIMSK5 = (1 << OCIE5A);
    TCCR5B = (1 << WGM52) | (1 << CS51) | (1 << CS50);

    SREG = sreg;
    sei();
}

void Timer_Task(void) {
    /* El tick lo mantiene la ISR; aquí no hay nada que hacer. */
}

/* Lee current_tick de forma atómica para evitar lecturas partidas de 32 bits. */
uint32_t Timer_Millis(void) {
    uint32_t tick;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        tick = current_tick;
    }
    return tick;
}

uint32_t Timer_GetMs(void) {
    return Timer_Millis();
}

/* Devuelve 1 si ya pasó duration_ms desde start_ms (usa resta modular). */
uint8_t Timer_Expired(uint32_t start_ms, uint32_t duration_ms) {
    return (uint8_t)((Timer_Millis() - start_ms) >= duration_ms);
}
