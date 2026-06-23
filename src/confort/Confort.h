/*
 * Módulo: Confort
 * Iluminación dimerizada por potenciómetro (ADC + PWM) y clima simulado
 * (calefactor/ventilador con LEDs). El equipo de sonido se controla por UART1:
 * ON/OFF y volumen 0-100 con PWM proporcional. El potenciómetro NO controla sonido.
 */
#ifndef CONFORT_H
#define CONFORT_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void     Confort_Init(void);
void     Confort_Task(uint32_t now_ms);

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
