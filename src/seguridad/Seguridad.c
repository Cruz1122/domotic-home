#include "Seguridad.h"
#include "../gpio/gpio.h"
#include "../adc/adc.h"
#include "../timer/timer.h"
#include "../uart/uart.h"

#define SMOKE_THRESHOLD_DEFAULT 400
#define ADC_READ_INTERVAL_MS    250
#define SMOKE_DEBOUNCE_COUNT    3

typedef enum {
    ACC_INACTIVE  = 0,
    ACC_ACTIVE    = 1,
    ACC_TRIGGERED = 2
} access_state_t;

typedef enum {
    FIRE_INACTIVE  = 0,
    FIRE_ACTIVE    = 1,
    FIRE_TRIGGERED = 2
} fire_state_t;

static access_state_t acc_state = ACC_INACTIVE;
static fire_state_t   fire_st   = FIRE_INACTIVE;

static uint8_t  last_pir1 = 0;
static uint8_t  last_pir2 = 0;
static uint16_t smoke_threshold = SMOKE_THRESHOLD_DEFAULT;
static uint8_t  smoke_count = 0;
static uint32_t adc_tick = 0;

static security_state_t st;

static void update_buzzer(void) {
    if (acc_state == ACC_TRIGGERED || fire_st == FIRE_TRIGGERED) {
        GPIO_WritePin(PIN_ALARM_BUZZER, GPIO_HIGH);
    } else {
        GPIO_WritePin(PIN_ALARM_BUZZER, GPIO_LOW);
    }
}

void seguridad_init(void) {
    GPIO_SetPinMode(PIN_PIR_1, GPIO_IN);
    GPIO_SetPinMode(PIN_PIR_2, GPIO_IN);
    GPIO_SetPinMode(PIN_ALARM_BUZZER, GPIO_OUT);
    GPIO_WritePin(PIN_ALARM_BUZZER, GPIO_LOW);
    acc_state = ACC_INACTIVE;
    fire_st   = FIRE_INACTIVE;
    smoke_count = 0;
    last_pir1  = GPIO_ReadPin(PIN_PIR_1);
    last_pir2  = GPIO_ReadPin(PIN_PIR_2);

    st.access_enabled  = 0;
    st.fire_enabled    = 0;
    st.access_triggered = 0;
    st.fire_triggered   = 0;
    st.pir1_active = last_pir1;
    st.pir2_active = last_pir2;
    st.smoke_adc   = 0;
    st.smoke_percent = 0;

    UART_WriteEvent(SER_SISTEMA, "Seguridad iniciado");
}

void seguridad_set_access_alarm(uint8_t active) {
    if (active) {
        if (acc_state == ACC_INACTIVE) {
            acc_state = ACC_ACTIVE;
            st.access_enabled = 1;
            UART_WriteEvent(SER_ALARMA, "Alarma acceso activada");
        }
    } else {
        if (acc_state != ACC_INACTIVE) {
            acc_state = ACC_INACTIVE;
            st.access_enabled = 0;
            st.access_triggered = 0;
            update_buzzer();
            UART_WriteEvent(SER_ALARMA, "Alarma acceso desactivada");
        }
    }
}

void seguridad_set_fire_alarm(uint8_t active) {
    if (active) {
        if (fire_st == FIRE_INACTIVE) {
            fire_st = FIRE_ACTIVE;
            st.fire_enabled = 1;
            smoke_count = 0;
            UART_WriteEvent(SER_FUEGO, "Alarma incendio activada");
        }
    } else {
        if (fire_st != FIRE_INACTIVE) {
            fire_st = FIRE_INACTIVE;
            st.fire_enabled = 0;
            st.fire_triggered = 0;
            smoke_count = 0;
            update_buzzer();
            UART_WriteEvent(SER_FUEGO, "Alarma incendio desactivada");
        }
    }
}

uint8_t seguridad_get_access_state(void) {
    return (uint8_t)acc_state;
}

uint8_t seguridad_get_fire_state(void) {
    return (uint8_t)fire_st;
}

uint8_t seguridad_is_access_triggered(void) {
    return (acc_state == ACC_TRIGGERED) ? 1 : 0;
}

uint8_t seguridad_is_fire_triggered(void) {
    return (fire_st == FIRE_TRIGGERED) ? 1 : 0;
}

void seguridad_set_smoke_threshold(uint16_t adc_val) {
    smoke_threshold = adc_val;
}

const security_state_t* seguridad_get_state(void) {
    return &st;
}

static void check_pir(void) {
    uint8_t pir1 = GPIO_ReadPin(PIN_PIR_1);
    uint8_t pir2 = GPIO_ReadPin(PIN_PIR_2);

    st.pir1_active = pir1;
    st.pir2_active = pir2;

    if (acc_state == ACC_ACTIVE) {
        if (pir1 && !last_pir1) {
            acc_state = ACC_TRIGGERED;
            st.access_triggered = 1;
            update_buzzer();
            UART_WriteEvent(SER_ALARMA, "Intrusion PIR1");
        }
        if (pir2 && !last_pir2) {
            acc_state = ACC_TRIGGERED;
            st.access_triggered = 1;
            update_buzzer();
            UART_WriteEvent(SER_ALARMA, "Intrusion PIR2");
        }
    }

    last_pir1 = pir1;
    last_pir2 = pir2;
}

static void check_smoke(uint32_t now_ms) {
    if (!Timer_Expired(adc_tick, ADC_READ_INTERVAL_MS)) return;
    adc_tick = now_ms;

    uint16_t adc_val = ADC_Read(ADC_MQ2);
    st.smoke_adc = adc_val;
    st.smoke_percent = (uint8_t)((uint32_t)adc_val * 100 / 1023);

    if (fire_st == FIRE_ACTIVE) {
        if (adc_val > smoke_threshold) {
            smoke_count++;
            if (smoke_count >= SMOKE_DEBOUNCE_COUNT) {
                fire_st = FIRE_TRIGGERED;
                st.fire_triggered = 1;
                update_buzzer();
                UART_WriteEvent(SER_FUEGO, "Humo detectado!");
            }
        } else {
            smoke_count = 0;
        }
    }
}

void seguridad_task(void) {
    uint32_t now_ms = Timer_GetMs();
    check_pir();
    check_smoke(now_ms);
}
