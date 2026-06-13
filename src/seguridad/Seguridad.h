#ifndef SEGURIDAD_H
#define SEGURIDAD_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void     Seguridad_Init(void);
void     Seguridad_Task(uint32_t now_ms);
/* 0=inactive, 1=armed, 2=triggered latched until explicit disable. */
void     Seguridad_SetAccessAlarm(uint8_t active);
void     Seguridad_SetFireAlarm(uint8_t active);
uint8_t  Seguridad_GetAccessState(void);
uint8_t  Seguridad_GetFireState(void);
uint8_t  Seguridad_IsAccessTriggered(void);
uint8_t  Seguridad_IsFireTriggered(void);
/* Clamped to ADC range 0..1023. */
void     Seguridad_SetSmokeThreshold(uint16_t adc_val);
const security_state_t* Seguridad_GetState(void);

#ifdef __cplusplus
}
#endif

#endif
