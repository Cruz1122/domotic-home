#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     ADC_Init(void);
uint16_t ADC_Read(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif
