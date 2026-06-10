#include "pwm.h"
#include "../gpio/gpio.h"
#include "../common/Definiciones.h"
#include <avr/io.h>

void PWM_Init(void) {
    TCCR4A = (1 << COM4A1) | (1 << COM4B1) | (1 << COM4C1) | (1 << WGM40);
    TCCR4B = (1 << WGM42) | (1 << CS41)  | (1 << CS40);

    GPIO_SetPinMode(PIN_DOOR_LED,    GPIO_OUT);
    GPIO_SetPinMode(PIN_LIGHT_PWM,   GPIO_OUT);
    GPIO_SetPinMode(PIN_SOUND_PWM,   GPIO_OUT);
}

void PWM_SetDuty(uint8_t pin, uint8_t duty) {
    switch (pin) {
        case 6:  OCR4A = duty; break;
        case 7:  OCR4B = duty; break;
        case 8:  OCR4C = duty; break;
        default: break;
    }
}
