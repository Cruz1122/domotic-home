/*
 * Módulo: PWM
 * PWM por hardware (Timer4) para iluminación: PIN_PWM_LIGHT (OC4B).
 * De 8 bits: duty 0-255. El servo del garaje usa su propio timer (servo_pwm).
 */
#ifndef PWM_H
#define PWM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void PWM_Init(void);
void PWM_SetDuty(uint8_t pin, uint8_t duty);

#ifdef __cplusplus
}
#endif

#endif
