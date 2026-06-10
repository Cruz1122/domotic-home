#include "spi.h"
#include "../gpio/gpio.h"
#include "../common/Definiciones.h"
#include <avr/io.h>

void SPI_Init(void) {
    GPIO_SetPinMode(PIN_RFID_SS, GPIO_OUT);
    GPIO_WritePin(PIN_RFID_SS, GPIO_HIGH);
    GPIO_SetPinMode(51, GPIO_OUT);
    GPIO_SetPinMode(52, GPIO_OUT);
    GPIO_SetPinMode(50, GPIO_IN);
    SPCR = (1 << SPE) | (1 << MSTR);
}

uint8_t SPI_Transfer(uint8_t data) {
    SPDR = data;
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

void SPI_Select(void) {
    GPIO_WritePin(PIN_RFID_SS, GPIO_LOW);
}

void SPI_Deselect(void) {
    GPIO_WritePin(PIN_RFID_SS, GPIO_HIGH);
}
