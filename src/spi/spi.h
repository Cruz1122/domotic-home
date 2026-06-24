/*
 * Módulo: SPI
 * Driver SPI maestro en C para el ATmega2560 (SS=53, MOSI=51, MISO=50, SCK=52).
 * Nota: el RC522 activo usa SPI.h de Arduino vía rfid_rc522_lib.cpp; este driver
 * queda en el repo como capa propia reutilizable.
 */
#ifndef SPI_H
#define SPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     SPI_Init(void);
uint8_t  SPI_Transfer(uint8_t data, uint8_t *received, uint16_t timeout_ticks);
void     SPI_Select(void);
void     SPI_Deselect(void);

#ifdef __cplusplus
}
#endif

#endif
