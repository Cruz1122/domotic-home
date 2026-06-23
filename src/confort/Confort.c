#include "Confort.h"
#include "../gpio/gpio.h"
#include "../adc/adc.h"
#include "../pwm/pwm.h"
#include "../timer/timer.h"
#include "../uart/uart.h"

#define LIGHT_INTERVAL_MS   200
#define TEMP_INTERVAL_MS    50
#define TEMP_DEFAULT         22
#define TEMP_AMBIENT         20
#define TEMP_HYSTERESIS_C     1

typedef enum {
    ADC_REQ_NONE = 0,
    ADC_REQ_LIGHT
} adc_req_t;

static uint8_t  light_percent = 0;
static uint32_t light_tick = 0;
static uint8_t  light_reported_percent = 0;

static uint8_t  volume_percent = 0;
static uint8_t  sound_enabled = 0;

static uint8_t  volume_reported_percent = 0;
static uint8_t  sound_reported_enabled = 0;
static adc_req_t adc_request = ADC_REQ_NONE;

static uint16_t current_temp_c = TEMP_AMBIENT;
static uint16_t target_temp_c  = TEMP_DEFAULT;
static uint8_t  heater_active = 0;
static uint8_t  fan_active = 0;
static uint32_t temp_tick = 0;
static uint8_t  heater_reported_active = 0;
static uint8_t  fan_reported_active = 0;
static uint16_t current_temp_reported = TEMP_AMBIENT;
static uint16_t target_temp_reported = TEMP_DEFAULT;

static char *Confort_AppendU8(char *dst, uint8_t value) {
    char tmp[3];
    uint8_t len = 0;

    do {
        tmp[len++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value > 0U && len < sizeof(tmp));

    while (len > 0U) {
        *dst++ = tmp[--len];
    }

    return dst;
}

static char *Confort_AppendU16(char *dst, uint16_t value) {
    char tmp[5];
    uint8_t len = 0;

    do {
        tmp[len++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value > 0U && len < sizeof(tmp));

    while (len > 0U) {
        *dst++ = tmp[--len];
    }

    return dst;
}

static uint8_t Confort_PctToDuty(uint8_t pct) {
    if (pct > 100U) {
        pct = 100U;
    }
    return (uint8_t)(((uint16_t)pct * 255U) / 100U);
}

static void Confort_LogPercent(const char *tag, const char *label, uint8_t pct) {
    char msg[32];
    char *p = msg;

    while (*label != '\0') {
        *p++ = *label++;
    }
    *p++ = ' ';
    p = Confort_AppendU8(p, pct);
    *p++ = '%';
    *p = '\0';
    UART_WriteEvent(tag, msg);
}

static void Confort_LogTemp(const char *label, uint16_t value) {
    char msg[32];
    char *p = msg;

    while (*label != '\0') {
        *p++ = *label++;
    }
    *p++ = ' ';
    p = Confort_AppendU16(p, value);
    *p++ = 'C';
    *p = '\0';
    UART_WriteEvent(SER_SISTEMA, msg);
}

static void Confort_ApplyVolumePwm(void) {
    uint8_t duty = sound_enabled ? Confort_PctToDuty(volume_percent) : 0U;
    PWM_SetDuty(PIN_SOUND_PWM, duty);
}

static void Confort_ApplyLightPwm(void) {
    PWM_SetDuty(PIN_LIGHT_PWM, Confort_PctToDuty(light_percent));
}

static void Confort_ApplyClimateOutputs(void) {
    uint8_t heater = heater_active ? 1U : 0U;
    uint8_t fan = fan_active ? 1U : 0U;

    if (heater != 0U && fan != 0U) {
        fan = 0U;
    }

    heater_active = heater;
    fan_active = fan;

    GPIO_WritePin(PIN_HEATER_LED, heater_active ? GPIO_HIGH : GPIO_LOW);
    GPIO_WritePin(PIN_FAN_LED, fan_active ? GPIO_HIGH : GPIO_LOW);

    if (heater_reported_active != heater_active) {
        heater_reported_active = heater_active;
        UART_WriteEvent(SER_SISTEMA, heater_active ? "Calefaccion ON" : "Calefaccion OFF");
    }
    if (fan_reported_active != fan_active) {
        fan_reported_active = fan_active;
        UART_WriteEvent(SER_SISTEMA, fan_active ? "Ventilador ON" : "Ventilador OFF");
    }
}

static void Confort_AdvanceClimate(uint32_t now_ms) {
    if (!Timer_Expired(temp_tick, TEMP_INTERVAL_MS)) {
        return;
    }

    temp_tick = now_ms;

    if (heater_active != 0U) {
        if (current_temp_c < target_temp_c) {
            current_temp_c++;
        }
        if (current_temp_c >= target_temp_c) {
            heater_active = 0U;
        }
    } else if (fan_active != 0U) {
        if (current_temp_c > target_temp_c) {
            current_temp_c--;
        }
        if (current_temp_c <= target_temp_c) {
            fan_active = 0U;
        }
    } else {
        if ((uint16_t)(current_temp_c + TEMP_HYSTERESIS_C) <= target_temp_c) {
            heater_active = 1U;
        } else if (current_temp_c >= (uint16_t)(target_temp_c + TEMP_HYSTERESIS_C)) {
            fan_active = 1U;
        }
    }

    Confort_ApplyClimateOutputs();

    if (current_temp_reported != current_temp_c) {
        current_temp_reported = current_temp_c;
        Confort_LogTemp("Temp", current_temp_c);
    }
}

static void Confort_ReevaluateClimate(void) {
    if (heater_active != 0U && current_temp_c >= target_temp_c) {
        heater_active = 0U;
    }
    if (fan_active != 0U && current_temp_c <= target_temp_c) {
        fan_active = 0U;
    }

    if (heater_active == 0U && fan_active == 0U) {
        if ((uint16_t)(current_temp_c + TEMP_HYSTERESIS_C) <= target_temp_c) {
            heater_active = 1U;
        } else if (current_temp_c >= (uint16_t)(target_temp_c + TEMP_HYSTERESIS_C)) {
            fan_active = 1U;
        }
    }

    Confort_ApplyClimateOutputs();
}

static void Confort_ProcessAdcSample(uint8_t channel, uint16_t raw) {
    uint8_t pct = (uint8_t)(((uint32_t)raw * 100U) / 1023U);

    if (channel == ADC_LIGHT_POT) {
        light_percent = pct;
        Confort_ApplyLightPwm();
        if (light_reported_percent != light_percent) {
            light_reported_percent = light_percent;
            Confort_LogPercent(SER_SISTEMA, "Luz", light_percent);
        }
        return;
    }

}

static void Confort_ServiceAdc(uint32_t now_ms) {
    uint16_t raw = 0U;

    if (adc_request != ADC_REQ_NONE) {
        if (ADC_Poll(&raw)) {
            uint8_t channel = ADC_LIGHT_POT;
            adc_request = ADC_REQ_NONE;
            Confort_ProcessAdcSample(channel, raw);
            light_tick = now_ms;
        }
        return;
    }

    if (ADC_IsBusy() != 0U) {
        return;
    }

    if (Timer_Expired(light_tick, LIGHT_INTERVAL_MS)) {
        ADC_Start(ADC_LIGHT_POT);
        adc_request = ADC_REQ_LIGHT;
    }
}

void Confort_Init(void) {
    GPIO_SetPinMode(PIN_HEATER_LED, GPIO_OUT);
    GPIO_SetPinMode(PIN_FAN_LED, GPIO_OUT);
    GPIO_WritePin(PIN_HEATER_LED, GPIO_LOW);
    GPIO_WritePin(PIN_FAN_LED, GPIO_LOW);
    PWM_SetDuty(PIN_LIGHT_PWM, 0U);
    PWM_SetDuty(PIN_SOUND_PWM, 0U);

    UART_WriteEvent(SER_SISTEMA, "Confort iniciado");
}

void Confort_Task(uint32_t now_ms) {
    Confort_ServiceAdc(now_ms);
    Confort_AdvanceClimate(now_ms);
}

uint8_t Confort_GetLightPercent(void) {
    return light_percent;
}

void Confort_SetTargetTemp(uint8_t celsius) {
    target_temp_c = celsius;
    if (target_temp_reported != target_temp_c) {
        target_temp_reported = target_temp_c;
        Confort_LogTemp("Objetivo", target_temp_c);
    }
    Confort_ReevaluateClimate();
}

uint16_t Confort_GetCurrentTemp(void) {
    return current_temp_c;
}

uint16_t Confort_GetTargetTemp(void) {
    return target_temp_c;
}

uint8_t Confort_IsHeaterActive(void) {
    return heater_active;
}

uint8_t Confort_IsFanActive(void) {
    return fan_active;
}

uint8_t Confort_GetVolumePercent(void) {
    return volume_percent;
}

void Confort_SetSoundEnabled(uint8_t enabled) {
    sound_enabled = enabled ? 1U : 0U;
    if (sound_reported_enabled != sound_enabled) {
        sound_reported_enabled = sound_enabled;
        UART_WriteEvent(SER_SONIDO, sound_enabled ? "Sonido ON" : "Sonido OFF");
    }
    Confort_ApplyVolumePwm();
}

void Confort_SetVolumePercent(uint8_t pct) {
    if (pct > 100U) pct = 100U;
    if (volume_percent != pct) {
        volume_percent = pct;
        volume_reported_percent = pct;
        Confort_LogPercent(SER_SONIDO, "Volumen remoto", pct);
    }
    Confort_ApplyVolumePwm();
}

uint8_t Confort_IsSoundEnabled(void) {
    return sound_enabled;
}
