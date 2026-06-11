#ifndef ACCESOS_H
#define ACCESOS_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ACCESS_MODE_NORMAL = 0,
    ACCESS_MODE_MAIN_DOOR,
    ACCESS_MODE_GARAGE,
    ACCESS_MODE_GAME_ROOM,
    ACCESS_MODE_ENROLL_PARENT,
    ACCESS_MODE_ENROLL_CHILD,
    ACCESS_MODE_DELETE_USER,
    ACCESS_MODE_RECHARGE_CHILD
} access_mode_t;

void     accesos_init(void);
void     accesos_task(uint32_t now_ms);
void     accesos_set_mode(access_mode_t mode);
access_mode_t accesos_get_mode(void);
void     accesos_set_pending_credits(uint8_t credits);
uint8_t  accesos_get_result_msg(char *buf, uint8_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* ACCESOS_H */
