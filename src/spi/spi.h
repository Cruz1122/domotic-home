/*
 * Módulo: SPI
 * Driver SPI maestro para hablar con el lector RFID RC522.
 * Pines: SS=53, MOSI=51, MISO=50, SCK=52 (ver Definiciones.h).
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
