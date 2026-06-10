#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_IN    0
#define GPIO_OUT   1
#define GPIO_LOW   0
#define GPIO_HIGH  1

void     GPIO_Init(void);
void     GPIO_SetPinMode(uint8_t pin, uint8_t mode);
void     GPIO_WritePin(uint8_t pin, uint8_t value);
uint8_t  GPIO_ReadPin(uint8_t pin);
void     GPIO_TogglePin(uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif
