#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     ADC_Init(void);
void     ADC_Start(uint8_t channel);
uint8_t  ADC_Poll(uint16_t *out_value);
uint8_t  ADC_IsBusy(void);
uint16_t ADC_Read(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif
