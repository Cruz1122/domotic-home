/*
 * Módulo: Servo (garaje)
 * Genera PWM de 50 Hz para un servomotor en PIN_PWM_SERVO_GARAGE (pin 9).
 * Se usa para abrir/cerrar el garaje por RFID. Timer1 + ISRs generan el pulso.
 */
#ifndef SERVO_PWM_H
#define SERVO_PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ServoPwm_Init(void);
void ServoPwm_SetAngle(uint8_t degrees);
void ServoPwm_Open(void);
void ServoPwm_Close(void);

#ifdef __cplusplus
}
#endif

#endif
