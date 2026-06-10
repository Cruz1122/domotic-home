#ifndef SEGURIDAD_H
#define SEGURIDAD_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void     seguridad_init(void);
void     seguridad_task(void);
void     seguridad_set_access_alarm(uint8_t active);
void     seguridad_set_fire_alarm(uint8_t active);
uint8_t  seguridad_get_access_state(void);
uint8_t  seguridad_get_fire_state(void);
uint8_t  seguridad_is_access_triggered(void);
uint8_t  seguridad_is_fire_triggered(void);
void     seguridad_set_smoke_threshold(uint16_t adc_val);
const security_state_t* seguridad_get_state(void);

#ifdef __cplusplus
}
#endif

#endif
