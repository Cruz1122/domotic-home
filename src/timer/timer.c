#include "timer.h"

extern unsigned long millis(void);

static uint32_t current_tick;

void Timer_Init(void) {
    current_tick = millis();
}

void Timer_Task(void) {
    current_tick = millis();
}

uint32_t Timer_GetMs(void) {
    return current_tick;
}

uint8_t Timer_Expired(uint32_t start_ms, uint32_t duration_ms) {
    return (Timer_GetMs() - start_ms) >= duration_ms;
}
