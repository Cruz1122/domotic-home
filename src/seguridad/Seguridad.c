/*
 * Módulo: Seguridad — implementación
 * Lee PIR1/PIR2 (GPIO) y MQ-2 (ADC no bloqueante) y gestiona dos alarmas.
 * Alarma de acceso: disparada por flanco de un PIR cuando está armada.
 * Alarma de incendio: disparada por humo sobre el umbral (con antirrebote).
 * Salidas: buzzer (acceso) y LED de fuego (incendio).
 */
#include "Seguridad.h"
#include "../gpio/gpio.h"
#include "../adc/adc.h"
#include "../timer/timer.h"
#include "../uart/uart.h"

#define SMOKE_THRESHOLD_DEFAULT 400
#define ADC_READ_INTERVAL_MS    250
#define SMOKE_DEBOUNCE_COUNT    3   /* lecturas consecutivas sobre umbral para disparar */

/* Estados de la alarma de acceso. */
typedef enum {
    ACC_INACTIVE  = 0,
    ACC_ACTIVE    = 1,
    ACC_TRIGGERED = 2
} access_state_t;

/* Estados de la alarma de incendio. */
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
static uint8_t  adc_pending = 0;

static security_state_t st;

/* Actualiza las salidas físicas según el estado de cada alarma. */
static void update_alarm_outputs(void) {
    GPIO_WritePin(PIN_ALARM_BUZZER, (acc_state == ACC_TRIGGERED) ? GPIO_HIGH : GPIO_LOW);
    GPIO_WritePin(PIN_FIRE_LED,   (fire_st   == FIRE_TRIGGERED) ? GPIO_HIGH : GPIO_LOW);
}

static void publish_smoke_event(uint16_t adc_val, uint8_t percent) {
    UART_WriteString(SER_FUEGO);
    UART_WriteString("Humo detectado raw=");
    UART_WriteDecimal(adc_val);
    UART_WriteString(" pct=");
    UART_WriteDecimal(percent);
    UART_Newline();
}

void Seguridad_Init(void) {
    GPIO_SetPinMode(PIN_PIR_1, GPIO_IN);
    GPIO_SetPinMode(PIN_PIR_2, GPIO_IN);
    GPIO_SetPinMode(PIN_ALARM_BUZZER, GPIO_OUT);
    GPIO_WritePin(PIN_ALARM_BUZZER, GPIO_LOW);
    GPIO_SetPinMode(PIN_FIRE_LED, GPIO_OUT);
    GPIO_WritePin(PIN_FIRE_LED, GPIO_LOW);
    acc_state = ACC_INACTIVE;
    fire_st   = FIRE_INACTIVE;
    smoke_count = 0;
    adc_pending = 0;
    adc_tick = Timer_GetMs() - ADC_READ_INTERVAL_MS;
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

/* Arma/desarma la alarma de acceso. Al desarmar se silencia y limpia el disparo. */
void Seguridad_SetAccessAlarm(uint8_t active) {
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
            update_alarm_outputs();
            UART_WriteEvent(SER_ALARMA, "Alarma acceso desactivada");
        }
    }
}

/* Arma/desarma la alarma de incendio. Al armar reinicia el antirrebote del humo. */
void Seguridad_SetFireAlarm(uint8_t active) {
    if (active) {
        if (fire_st == FIRE_INACTIVE) {
            fire_st = FIRE_ACTIVE;
            st.fire_enabled = 1;
            smoke_count = 0;
            adc_tick = Timer_GetMs() - ADC_READ_INTERVAL_MS;
            UART_WriteEvent(SER_FUEGO, "Alarma incendio activada");
        }
    } else {
        if (fire_st != FIRE_INACTIVE) {
            fire_st = FIRE_INACTIVE;
            st.fire_enabled = 0;
            st.fire_triggered = 0;
            smoke_count = 0;
            update_alarm_outputs();
            UART_WriteEvent(SER_FUEGO, "Alarma incendio desactivada");
        }
    }
}

uint8_t Seguridad_GetAccessState(void) {
    return (uint8_t)acc_state;
}

uint8_t Seguridad_GetFireState(void) {
    return (uint8_t)fire_st;
}

uint8_t Seguridad_IsAccessTriggered(void) {
    return (acc_state == ACC_TRIGGERED) ? 1 : 0;
}

uint8_t Seguridad_IsFireTriggered(void) {
    return (fire_st == FIRE_TRIGGERED) ? 1 : 0;
}

void Seguridad_SetSmokeThreshold(uint16_t adc_val) {
    if (adc_val > 1023U) {
        adc_val = 1023U;
    }
    smoke_threshold = adc_val;
}

const security_state_t* Seguridad_GetState(void) {
    return &st;
}

/* Detecta flanco ascendente en los PIR y dispara la alarma de acceso si está armada. */
static void check_pir(void) {
    uint8_t pir1 = GPIO_ReadPin(PIN_PIR_1);
    uint8_t pir2 = GPIO_ReadPin(PIN_PIR_2);

    st.pir1_active = pir1;
    st.pir2_active = pir2;

    if (acc_state == ACC_ACTIVE) {
        if (pir1 && !last_pir1) {
            acc_state = ACC_TRIGGERED;
            st.access_triggered = 1;
            update_alarm_outputs();
            UART_WriteEvent(SER_ALARMA, "Intrusion PIR1");
        }
        if (pir2 && !last_pir2) {
            acc_state = ACC_TRIGGERED;
            st.access_triggered = 1;
            update_alarm_outputs();
            UART_WriteEvent(SER_ALARMA, "Intrusion PIR2");
        }
    }

    last_pir1 = pir1;
    last_pir2 = pir2;
}

/* Lee el MQ-2 sin bloquear: arranca la conversión y la resuelve en ciclos.
 * Dispara la alarma de incendio tras SMOKE_DEBOUNCE_COUNT lecturas sobre el umbral. */
static void check_smoke(uint32_t now_ms) {
    uint16_t adc_val = 0;
    uint8_t  valid   = 0;

    (void)now_ms;

    /* Resolver conversión pendiente */
    if (adc_pending) {
        if (ADC_Poll(&adc_val)) {
            adc_pending = 0;
            valid = 1;
        } else if (!ADC_IsBusy()) {
            adc_pending = 0;
        }
    }

    /* Iniciar nueva conversion si el intervalo expiro */
    if (!adc_pending && !valid && Timer_Expired(adc_tick, ADC_READ_INTERVAL_MS)) {
        if (!ADC_IsBusy()) {
            ADC_Start(ADC_MQ2);
            adc_pending = 1;
            if (ADC_Poll(&adc_val)) {
                adc_pending = 0;
                valid = 1;
            }
        }
    }

    if (!valid) return;

    if (adc_val > 1023U) adc_val = 1023U;
    st.smoke_adc = adc_val;
    st.smoke_percent = (uint8_t)((uint32_t)adc_val * 100U / 1023U);

    if (fire_st == FIRE_ACTIVE) {
        if (adc_val > smoke_threshold) {
            smoke_count++;
            if (smoke_count >= SMOKE_DEBOUNCE_COUNT) {
                fire_st = FIRE_TRIGGERED;
                st.fire_triggered = 1;
                update_alarm_outputs();
                publish_smoke_event(adc_val, st.smoke_percent);
            }
        } else {
            smoke_count = 0;
        }
    }
}

/* Tarea periódica: revisa PIR y MQ-2. No bloquea. */
void Seguridad_Task(uint32_t now_ms) {
    check_pir();
    check_smoke(now_ms);
}
