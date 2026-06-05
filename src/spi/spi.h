#ifndef SPI_H
#define SPI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     SPI_Init(void);
uint8_t  SPI_Transfer(uint8_t data);
void     SPI_Select(void);
void     SPI_Deselect(void);

#ifdef __cplusplus
}
#endif

#endif
