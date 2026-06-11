#ifndef CONFORT_H
#define CONFORT_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void     Confort_Init(void);
void     Confort_Task(void);

uint8_t  Confort_GetLightPercent(void);

void     Confort_SetTargetTemp(uint8_t celsius);
uint16_t Confort_GetCurrentTemp(void);
uint16_t Confort_GetTargetTemp(void);
uint8_t  Confort_IsHeaterActive(void);
uint8_t  Confort_IsFanActive(void);

uint8_t  Confort_GetVolumePercent(void);
void     Confort_SetSoundEnabled(uint8_t enabled);
void     Confort_SetVolumePercent(uint8_t pct);
uint8_t  Confort_IsSoundEnabled(void);

#ifdef __cplusplus
}
#endif

#endif
