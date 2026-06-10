#ifndef CONFORT_H
#define CONFORT_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void     confort_init(void);
void     confort_task(void);

uint8_t  confort_get_light_percent(void);

void     confort_set_target_temp(uint8_t celsius);
uint16_t confort_get_current_temp(void);
uint16_t confort_get_target_temp(void);
uint8_t  confort_is_heater_active(void);
uint8_t  confort_is_fan_active(void);

uint8_t  confort_get_volume_percent(void);
void     confort_set_sound_enabled(uint8_t enabled);
uint8_t  confort_is_sound_enabled(void);

#ifdef __cplusplus
}
#endif

#endif
