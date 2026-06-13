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

uint16_t ADC_Read(uint8_t channel) {
    uint16_t value = 0;
    uint16_t guard = 0xFFFFU;

    ADC_Start(channel);
    while (guard-- > 0U) {
        if (ADC_Poll(&value)) {
            return value;
        }
    }

    return 0xFFFFU;
}
