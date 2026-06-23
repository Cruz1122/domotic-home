/*
 * Módulo: GPIO — implementación
 * Traduce un número de pin de Mega 2560 al par (registro, máscara) correspondiente.
 * Solo cubre los pines usados por el proyecto; el resto se descarta sin error.
 */
#include "gpio.h"
#include "../common/Definiciones.h"
#include <avr/io.h>

/* Relación pin -> registros DDR/PORT/PIN y bit dentro del puerto. */
typedef struct {
    volatile uint8_t *ddr;
    volatile uint8_t *port;
    volatile uint8_t *pin;
    uint8_t mask;
} gpio_map_t;

/* Resuelve el mapa de un pin. Devuelve 1 si está soportado, 0 si no. */
static uint8_t gpio_map_pin(uint8_t pin, gpio_map_t *map) {
    if (map == 0) return 0;

#define GPIO_CASE(PIN_NUM, DDR_REG, PORT_REG, PIN_REG, BIT_NUM) \
    case PIN_NUM: \
        map->ddr = &(DDR_REG); \
        map->port = &(PORT_REG); \
        map->pin = &(PIN_REG); \
        map->mask = (uint8_t)_BV(BIT_NUM); \
        return 1

    switch (pin) {
        GPIO_CASE(PIN_LIGHT_PWM,  DDRH, PORTH, PINH, 4);
        GPIO_CASE(PIN_SOUND_PWM,  DDRH, PORTH, PINH, 5);
        GPIO_CASE(PIN_GARAGE_SERVO, DDRH, PORTH, PINH, 6);
        GPIO_CASE(PIN_ALARM_BUZZER, DDRB, PORTB, PINB, 4);
        GPIO_CASE(PIN_FIRE_LED,     DDRB, PORTB, PINB, 5);
        GPIO_CASE(PIN_FIRE_TEST_SWITCH,   DDRL, PORTL, PINL, 5);
        GPIO_CASE(PIN_ACCESS_TEST_SWITCH, DDRL, PORTL, PINL, 4);
        GPIO_CASE(PIN_ACCESS_LED,         DDRL, PORTL, PINL, 3);
        GPIO_CASE(PIN_LCD_RS,     DDRA, PORTA, PINA, 0);
        GPIO_CASE(PIN_LCD_E,      DDRA, PORTA, PINA, 1);
        GPIO_CASE(PIN_LCD_D4,     DDRA, PORTA, PINA, 2);
        GPIO_CASE(PIN_LCD_D5,     DDRA, PORTA, PINA, 3);
        GPIO_CASE(PIN_LCD_D6,     DDRA, PORTA, PINA, 4);
        GPIO_CASE(PIN_LCD_D7,     DDRA, PORTA, PINA, 5);
        GPIO_CASE(PIN_KEYPAD_ROW1, DDRC, PORTC, PINC, 3);
        GPIO_CASE(PIN_KEYPAD_ROW2, DDRC, PORTC, PINC, 2);
        GPIO_CASE(PIN_KEYPAD_ROW3, DDRC, PORTC, PINC, 1);
        GPIO_CASE(PIN_KEYPAD_ROW4, DDRC, PORTC, PINC, 0);
        GPIO_CASE(PIN_PIR1,       DDRD, PORTD, PIND, 7);
        GPIO_CASE(PIN_PIR2,       DDRG, PORTG, PING, 2);
        GPIO_CASE(PIN_HEATER,     DDRG, PORTG, PING, 1);
        GPIO_CASE(PIN_FAN,        DDRG, PORTG, PING, 0);
        GPIO_CASE(PIN_OVEN,       DDRL, PORTL, PINL, 7);
        GPIO_CASE(PIN_DOOR_LED,   DDRL, PORTL, PINL, 6);
        GPIO_CASE(PIN_RFID_RST,   DDRL, PORTL, PINL, 0);
        GPIO_CASE(PIN_SPI_MISO,   DDRB, PORTB, PINB, 3);
        GPIO_CASE(PIN_SPI_MOSI,   DDRB, PORTB, PINB, 2);
        GPIO_CASE(PIN_SPI_SCK,    DDRB, PORTB, PINB, 1);
        GPIO_CASE(PIN_SPI_SS,     DDRB, PORTB, PINB, 0);
        GPIO_CASE(PIN_KEYPAD_COL1, DDRK, PORTK, PINK, 0);
        GPIO_CASE(PIN_KEYPAD_COL2, DDRK, PORTK, PINK, 1);
        GPIO_CASE(PIN_KEYPAD_COL3, DDRK, PORTK, PINK, 2);
        GPIO_CASE(PIN_KEYPAD_COL4, DDRK, PORTK, PINK, 3);
        default:
            return 0;
    }

#undef GPIO_CASE
}

void GPIO_Init(void) {
    /* Sin configuración global: cada módulo pide su modo con GPIO_SetPinMode. */
}

void GPIO_SetPinMode(uint8_t pin, uint8_t mode) {
    gpio_map_t map;
    if (!gpio_map_pin(pin, &map)) return;
    if (mode == GPIO_OUT) {
        *map.ddr |= map.mask;
    } else if (mode == GPIO_IN_PULLUP) {
        *map.ddr &= (uint8_t)~map.mask;
        *map.port |= map.mask;
    } else {
        *map.ddr &= (uint8_t)~map.mask;
        *map.port &= (uint8_t)~map.mask;
    }
}

void GPIO_WritePin(uint8_t pin, uint8_t value) {
    gpio_map_t map;
    if (!gpio_map_pin(pin, &map)) return;
    if (value) {
        *map.port |= map.mask;
    } else {
        *map.port &= (uint8_t)~map.mask;
    }
}

uint8_t GPIO_ReadPin(uint8_t pin) {
    gpio_map_t map;
    if (!gpio_map_pin(pin, &map)) return 0;
    return (*map.pin & map.mask) ? 1 : 0;
}

void GPIO_TogglePin(uint8_t pin) {
    gpio_map_t map;
    if (!gpio_map_pin(pin, &map)) return;
    *map.port ^= map.mask;
}
