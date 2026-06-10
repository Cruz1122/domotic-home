#include "Confort.h"
#include "../gpio/gpio.h"
#include "../adc/adc.h"
#include "../pwm/pwm.h"
#include "../timer/timer.h"
#include "../uart/uart.h"

#define LIGHT_INTERVAL_MS   200
#define VOLUME_INTERVAL_MS  200
#define TEMP_INTERVAL_MS   2000
#define TEMP_DEFAULT         22
#define TEMP_AMBIENT         20

static uint8_t  light_percent = 0;
static uint32_t light_tick = 0;

static uint8_t  volume_percent = 0;
static uint8_t  sound_enabled = 1;
static uint32_t volume_tick = 0;

static uint16_t current_temp_c = TEMP_AMBIENT;
static uint16_t target_temp_c  = TEMP_DEFAULT;
static uint8_t  heater_active = 0;
static uint8_t  fan_active = 0;
static uint32_t temp_tick = 0;

void confort_init(void) {
    GPIO_SetPinMode(PIN_HEATER_LED, GPIO_OUT);
    GPIO_SetPinMode(PIN_FAN_LED, GPIO_OUT);
    GPIO_WritePin(PIN_HEATER_LED, GPIO_LOW);
    GPIO_WritePin(PIN_FAN_LED, GPIO_LOW);

    UART_WriteEvent(SER_SISTEMA, "Confort iniciado");
}

void confort_task(void) {
    uint32_t now_ms = Timer_GetMs();

    if (Timer_Expired(light_tick, LIGHT_INTERVAL_MS)) {
        light_tick = now_ms;
        uint16_t raw = ADC_Read(ADC_LIGHT_POT);
        light_percent = (uint8_t)((uint32_t)raw * 100 / 1023);
        PWM_SetDuty(PIN_LIGHT_PWM, (uint8_t)(raw >> 2));
    }

    if (Timer_Expired(volume_tick, VOLUME_INTERVAL_MS)) {
        volume_tick = now_ms;
        uint16_t raw = ADC_Read(ADC_VOLUME_POT);
        volume_percent = (uint8_t)((uint32_t)raw * 100 / 1023);
        if (sound_enabled) {
            PWM_SetDuty(PIN_SOUND_PWM, (uint8_t)(raw >> 2));
        } else {
            PWM_SetDuty(PIN_SOUND_PWM, 0);
        }
    }

    if (Timer_Expired(temp_tick, TEMP_INTERVAL_MS)) {
        temp_tick = now_ms;
        if (current_temp_c < target_temp_c) {
            current_temp_c++;
        } else if (current_temp_c > target_temp_c) {
            current_temp_c--;
        }

        if (current_temp_c < target_temp_c) {
            GPIO_WritePin(PIN_HEATER_LED, GPIO_HIGH);
            GPIO_WritePin(PIN_FAN_LED, GPIO_LOW);
            heater_active = 1;
            fan_active = 0;
        } else if (current_temp_c > target_temp_c) {
            GPIO_WritePin(PIN_HEATER_LED, GPIO_LOW);
            GPIO_WritePin(PIN_FAN_LED, GPIO_HIGH);
            heater_active = 0;
            fan_active = 1;
        } else {
            GPIO_WritePin(PIN_HEATER_LED, GPIO_LOW);
            GPIO_WritePin(PIN_FAN_LED, GPIO_LOW);
            heater_active = 0;
            fan_active = 0;
        }
    }
}

uint8_t confort_get_light_percent(void) {
    return light_percent;
}

void confort_set_target_temp(uint8_t celsius) {
    target_temp_c = celsius;
}

uint16_t confort_get_current_temp(void) {
    return current_temp_c;
}

uint16_t confort_get_target_temp(void) {
    return target_temp_c;
}

uint8_t confort_is_heater_active(void) {
    return heater_active;
}

uint8_t confort_is_fan_active(void) {
    return fan_active;
}

uint8_t confort_get_volume_percent(void) {
    return volume_percent;
}

void confort_set_sound_enabled(uint8_t enabled) {
    sound_enabled = enabled ? 1 : 0;
    if (!sound_enabled) {
        PWM_SetDuty(PIN_SOUND_PWM, 0);
    }
}

uint8_t confort_is_sound_enabled(void) {
    return sound_enabled;
}
