/*
 * Módulo: Seguridad
 * Alarmas de acceso (PIR) e incendio (MQ-2 por ADC).
 * El código admin solo se pide para activar/desactivar las alarmas.
 * Eventos críticos se reportan por UART0.
 */
#ifndef SEGURIDAD_H
#define SEGURIDAD_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void     Seguridad_Init(void);
void     Seguridad_Task(uint32_t now_ms);
/* Estado de cada alarma: 0=inactiva, 1=armada, 2=disparada (queda hasta desactivar). */
void     Seguridad_SetAccessAlarm(uint8_t active);
void     Seguridad_SetFireAlarm(uint8_t active);
uint8_t  Seguridad_GetAccessState(void);
uint8_t  Seguridad_GetFireState(void);
uint8_t  Seguridad_IsAccessTriggered(void);
uint8_t  Seguridad_IsFireTriggered(void);
/* Pulsador físico de prueba (PIN_FIRE_TEST_SWITCH, activo bajo).
 * En Proteus, botón a GND simula detección de humo cuando la alarma está armada. */
/* Umbral de humo en cuentas ADC (0..1023). */
void     Seguridad_SetSmokeThreshold(uint16_t adc_val);
const security_state_t* Seguridad_GetState(void);

#ifdef __cplusplus
}
#endif

#endif
