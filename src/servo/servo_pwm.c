#include "servo_pwm.h"
#include "../gpio/gpio.h"
#include "../common/Definiciones.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define SERVO_PIN       PIN_GARAGE_SERVO
#define SERVO_50HZ_TOP  39999
#define SERVO_PULSE_MIN 2000
#define SERVO_PULSE_MAX 4000

static volatile uint16_t target_pulse = SERVO_PULSE_MIN;

static uint16_t angle_to_pulse(uint8_t degrees) {
    if (degrees > 180) degrees = 180;
    return SERVO_PULSE_MIN + ((uint32_t)(SERVO_PULSE_MAX - SERVO_PULSE_MIN) * degrees / 180);
}

void ServoPwm_Init(void) {
    GPIO_SetPinMode(SERVO_PIN, GPIO_OUT);
    GPIO_WritePin(SERVO_PIN, GPIO_LOW);

    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11);
    OCR1A = SERVO_50HZ_TOP;
    TIMSK1 = (1 << OCIE1A);
}

void ServoPwm_SetAngle(uint8_t degrees) {
    uint16_t pulse = angle_to_pulse(degrees);
    uint8_t sreg = SREG;
    cli();
    target_pulse = pulse;
    SREG = sreg;
}

void ServoPwm_Open(void) {
    ServoPwm_SetAngle(90);
}

void ServoPwm_Close(void) {
    ServoPwm_SetAngle(0);
}

ISR(TIMER1_COMPA_vect) {
    TIMSK1 &= ~(1 << OCIE1B);
    GPIO_WritePin(SERVO_PIN, GPIO_HIGH);
    OCR1B = target_pulse;
    TIMSK1 |= (1 << OCIE1B);
}

ISR(TIMER1_COMPB_vect) {
    TIMSK1 &= ~(1 << OCIE1B);
    GPIO_WritePin(SERVO_PIN, GPIO_LOW);
}
