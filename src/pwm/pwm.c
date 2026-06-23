/*
 * Módulo: PWM — implementación
 * Timer4 en modo Fast PWM de 8 bits, prescaler /64.
 * Canales: OC4A=LED puerta, OC4B=iluminación, OC4C=sonido.
 * El duty se asigna por pin para que el resto del código hable en pines, no en registros.
 */
#include "pwm.h"
#include "../gpio/gpio.h"
#include "../common/Definiciones.h"
#include <avr/io.h>

void PWM_Init(void) {
    /* PWM de 8 bits, no invertido, en los tres canales A/B/C. */
    TCCR4A = (1 << COM4A1) | (1 << COM4B1) | (1 << COM4C1) | (1 << WGM40);
    TCCR4B = (1 << WGM42) | (1 << CS41)  | (1 << CS40);

    GPIO_SetPinMode(PIN_PWM_DOOR_LED, GPIO_OUT);
    GPIO_SetPinMode(PIN_PWM_LIGHT,    GPIO_OUT);
    GPIO_SetPinMode(PIN_PWM_SOUND,    GPIO_OUT);
}

/* Ajusta el duty (0-255) del canal correspondiente al pin. */
void PWM_SetDuty(uint8_t pin, uint8_t duty) {
    uint16_t duty_clamped = duty;

    if (duty_clamped > 255U) {
        duty_clamped = 255U;
    }

    if (pin == PIN_PWM_DOOR_LED) {
        OCR4A = (uint8_t)duty_clamped;
    } else if (pin == PIN_PWM_LIGHT) {
        OCR4B = (uint8_t)duty_clamped;
    } else if (pin == PIN_PWM_SOUND) {
        OCR4C = (uint8_t)duty_clamped;
    }
}
