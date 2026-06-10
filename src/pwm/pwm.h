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
