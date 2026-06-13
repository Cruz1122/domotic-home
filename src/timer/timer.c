#include "timer.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

static volatile uint32_t current_tick;

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
    OCR5A = 249;
    TIFR5 = (1 << OCF5A);
    TIMSK5 = (1 << OCIE5A);
    TCCR5B = (1 << WGM52) | (1 << CS51) | (1 << CS50);

    SREG = sreg;
    sei();
}

void Timer_Task(void) {
}

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

uint8_t Timer_Expired(uint32_t start_ms, uint32_t duration_ms) {
    return (uint8_t)((Timer_Millis() - start_ms) >= duration_ms);
}
