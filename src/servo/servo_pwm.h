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
