/*
 * Módulo: ADC — implementación
 * ADC de 10 bits, prescaler /128 (REFS0 = AVcc). Arranque/polling no bloqueante:
 * el módulo llama ADC_Start y revisa ADC_Poll en ciclos posteriores.
 */
#include "adc.h"
#include <avr/io.h>

static volatile uint8_t adc_busy = 0;

static uint8_t adc_channel_valid(uint8_t channel) {
    return (channel <= 15U);
}

void ADC_Init(void) {
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    adc_busy = 0;
}

/* Arranca una conversión en el canal indicado. No espera el resultado. */
void ADC_Start(uint8_t channel) {
    if (!adc_channel_valid(channel)) {
        return;
    }

    if (ADCSRA & (1 << ADSC)) {
        adc_busy = 1;
        return;
    }

    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    adc_busy = 1;
    ADCSRA |= (1 << ADSC);
}

/* Devuelve 1 si la conversión terminó y entrega el valor en *out_value. */
uint8_t ADC_Poll(uint16_t *out_value) {
    if (!adc_busy) {
        return 0;
    }

    if (ADCSRA & (1 << ADSC)) {
        return 0;
    }

    if (out_value != 0) {
        *out_value = ADC;
    }

    adc_busy = 0;
    return 1;
}

uint8_t ADC_IsBusy(void) {
    return (adc_busy != 0U) || ((ADCSRA & (1 << ADSC)) != 0U);
}
