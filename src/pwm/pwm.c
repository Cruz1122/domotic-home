/*
 * Módulo: PWM — implementación
 * Timer4 en modo Fast PWM de 8 bits, prescaler /64.
 * Canal: OC4B=iluminación.
 * El duty se asigna por pin para que el resto del código hable en pines, no en registros.
 */
#include "pwm.h"
#include "../gpio/gpio.h"
#include "../common/Definiciones.h"
#include <avr/io.h>

void PWM_Init(void) {
    TCCR4A = (1 << COM4B1) | (1 << WGM40);
    TCCR4B = (1 << WGM42) | (1 << CS41)  | (1 << CS40);

    GPIO_SetPinMode(PIN_PWM_LIGHT, GPIO_OUT);
}

void PWM_SetDuty(uint8_t pin, uint8_t duty) {
    uint16_t duty_clamped = duty;

    if (duty_clamped > 255U) {
        duty_clamped = 255U;
    }

    if (pin == PIN_PWM_LIGHT) {
        OCR4B = (uint8_t)duty_clamped;
    }
}
