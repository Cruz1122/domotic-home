/*
 * Módulo: Timer
 * Base de tiempo del sistema en milisegundos, basada en Timer5.
 * Permite tareas no bloqueantes: nadie usa delay(), todo se mide contra Timer_Millis().
 */
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void     Timer_Init(void);
void     Timer_Task(void);   /* no-op de compatibilidad; el tick lo da la ISR */
uint32_t Timer_Millis(void);
uint32_t Timer_GetMs(void);  /* alias de compatibilidad */
uint8_t  Timer_Expired(uint32_t start_ms, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif
