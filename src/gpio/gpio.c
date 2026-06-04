#include "gpio.h"
#include <Arduino.h>

void GPIO_Init(void) {
}

void GPIO_SetPinMode(uint8_t pin, uint8_t mode) {
    uint8_t port = digitalPinToPort(pin);
    if (port == NOT_A_PORT) return;
    volatile uint8_t *ddr = portModeRegister(port);
    uint8_t bit = digitalPinToBitMask(pin);
    if (mode == GPIO_OUT) {
        *ddr |= bit;
    } else {
        *ddr &= ~bit;
    }
}

void GPIO_WritePin(uint8_t pin, uint8_t value) {
    uint8_t port = digitalPinToPort(pin);
    if (port == NOT_A_PORT) return;
    volatile uint8_t *out = portOutputRegister(port);
    uint8_t bit = digitalPinToBitMask(pin);
    if (value) {
        *out |= bit;
    } else {
        *out &= ~bit;
    }
}

uint8_t GPIO_ReadPin(uint8_t pin) {
    uint8_t port = digitalPinToPort(pin);
    if (port == NOT_A_PORT) return 0;
    volatile uint8_t *in = portInputRegister(port);
    uint8_t bit = digitalPinToBitMask(pin);
    return (*in & bit) ? 1 : 0;
}

void GPIO_TogglePin(uint8_t pin) {
    uint8_t port = digitalPinToPort(pin);
    if (port == NOT_A_PORT) return;
    volatile uint8_t *out = portOutputRegister(port);
    uint8_t bit = digitalPinToBitMask(pin);
    *out ^= bit;
}
