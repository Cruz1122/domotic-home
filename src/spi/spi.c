/*
 * Módulo: SPI — implementación
 * Configura el ATmega2560 como maestro SPI (fclk/4) para el RC522.
 * SS se maneja a mano con SPI_Select/SPI_Deselect.
 */
#include "spi.h"
#include "../gpio/gpio.h"
#include "../common/Definiciones.h"
#include <avr/io.h>

void SPI_Init(void) {
    GPIO_SetPinMode(PIN_SPI_SS, GPIO_OUT);
    GPIO_SetPinMode(PIN_SPI_MOSI, GPIO_OUT);
    GPIO_SetPinMode(PIN_SPI_SCK, GPIO_OUT);
    GPIO_SetPinMode(PIN_SPI_MISO, GPIO_IN);
    GPIO_WritePin(PIN_SPI_SS, GPIO_HIGH);

    /* SPE=habilita SPI, MSTR=maestro. Sin interrupciones. */
    SPCR = (1 << SPE) | (1 << MSTR);
    SPSR = 0x00;
}

/* Transfiere 1 byte (full-duplex). Timeout en ticks para no colgarse si el RC522 no responde. */
uint8_t SPI_Transfer(uint8_t data, uint8_t *received, uint16_t timeout_ticks) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF))) {
        if (timeout_ticks == 0) {
            if (received) *received = 0x00;
            return 0;
        }
        timeout_ticks--;
    }
    if (received) *received = SPDR;
    return 1;
}

/* Selecciona el RC522 (SS LOW) y lo suelta (SS HIGH). */
void SPI_Select(void) {
    GPIO_WritePin(PIN_SPI_SS, GPIO_LOW);
}

void SPI_Deselect(void) {
    GPIO_WritePin(PIN_SPI_SS, GPIO_HIGH);
}
