#include "servo_pwm.h"
#include "../gpio/gpio.h"
#include "../common/Definiciones.h"
#include "../uart/uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>

/* Safe servo timing, in microseconds, for 50 Hz PWM on Timer1. */
#define SERVO_PIN               PIN_PWM_SERVO_GARAGE
#define SERVO_PERIOD_US         20000U
#define SERVO_PULSE_CLOSED_US    1000U
#define SERVO_PULSE_OPEN_US      2000U
#define SERVO_TICKS_PER_US          2U

#define SERVO_US_TO_TICKS(us)   ((uint16_t)((uint32_t)(us) * SERVO_TICKS_PER_US))
#define SERVO_50HZ_TOP          (SERVO_US_TO_TICKS(SERVO_PERIOD_US) - 1U)
#define SERVO_PULSE_MIN         SERVO_US_TO_TICKS(SERVO_PULSE_CLOSED_US)
#define SERVO_PULSE_MAX         SERVO_US_TO_TICKS(SERVO_PULSE_OPEN_US)

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
    OCR1B = target_pulse;
    TIMSK1 |= (1 << OCIE1A) | (1 << OCIE1B);

    UART_WriteEvent(SER_SISTEMA, "Servo Timer1 init OK");
}

void ServoPwm_SetAngle(uint8_t degrees) {
    uint16_t pulse = angle_to_pulse(degrees);
    uint8_t sreg = SREG;
    cli();
    target_pulse = pulse;
    SREG = sreg;
    UART_WriteString(SER_SISTEMA "Servo angulo=");
    UART_WriteDecimal(degrees);
    UART_WriteString(" pulse=");
    UART_WriteDecimal(pulse);
    UART_Newline();
}

void ServoPwm_Open(void) {
    UART_WriteEvent(SER_SISTEMA, "Servo OPEN 90deg");
    ServoPwm_SetAngle(90);
}

void ServoPwm_Close(void) {
    UART_WriteEvent(SER_SISTEMA, "Servo CLOSE 0deg");
    ServoPwm_SetAngle(0);
}

ISR(TIMER1_COMPA_vect) {
    GPIO_WritePin(SERVO_PIN, GPIO_HIGH);
    OCR1B = target_pulse;
}

ISR(TIMER1_COMPB_vect) {
    GPIO_WritePin(SERVO_PIN, GPIO_LOW);
}
