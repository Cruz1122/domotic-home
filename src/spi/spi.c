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

    SPCR = (1 << SPE) | (1 << MSTR);
    SPSR = 0x00;
}

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

void SPI_Select(void) {
    GPIO_WritePin(PIN_SPI_SS, GPIO_LOW);
}

void SPI_Deselect(void) {
    GPIO_WritePin(PIN_SPI_SS, GPIO_HIGH);
}
