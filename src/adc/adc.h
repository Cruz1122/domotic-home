/*
 * Módulo: ADC
 * Driver del ADC del ATmega2560, no bloqueante.
 * Se arranca una conversión con ADC_Start y se consulta con ADC_Poll.
 * Canales usados: ADC0 (MQ-2 humo) y ADC1 (potenciómetro de iluminación).
 */
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
uint8_t  ADC_ReadSync(uint8_t channel, uint16_t *out_value);

#ifdef __cplusplus
}
#endif

#endif
