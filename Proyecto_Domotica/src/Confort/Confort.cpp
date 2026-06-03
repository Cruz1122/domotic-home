#include "../Definiciones.h"
#include "Confort.h"

void inicializarConfort() {
    DDR_CONFORT |= (1 << BIT_CALEFACTOR) | (1 << BIT_VENTILADOR);
    PORT_CONFORT &= ~((1 << BIT_CALEFACTOR) | (1 << BIT_VENTILADOR));

    pinMode(PIN_LED_DIMMER, OUTPUT);

    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t leerADC(uint8_t canal) {
    ADMUX = (ADMUX & 0xE0) | (canal & 0x07);

    if (canal > 7) {
        ADCSRB |= (1 << MUX5);
    } else {
        ADCSRB &= ~(1 << MUX5);
    }

    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));

    return ADC;
}

void actualizarClimatizacion() {
    uint16_t valorTemperatura = leerADC(0);

    if (valorTemperatura > 700) {
        PORT_CONFORT |= (1 << BIT_VENTILADOR);
        PORT_CONFORT &= ~(1 << BIT_CALEFACTOR);
    }
    else if (valorTemperatura < 400) {
        PORT_CONFORT |= (1 << BIT_CALEFACTOR);
        PORT_CONFORT &= ~(1 << BIT_VENTILADOR);
    }
    else {
        PORT_CONFORT &= ~((1 << BIT_CALEFACTOR) | (1 << BIT_VENTILADOR));
    }
}

void actualizarIluminacion() {
    uint16_t valorLuz = leerADC(1);
    uint8_t cicloTrabajo = valorLuz / 4;
    analogWrite(PIN_LED_DIMMER, cicloTrabajo);
}
