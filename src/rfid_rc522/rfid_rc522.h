#ifndef RFID_RC522_H
#define RFID_RC522_H

#include <stdint.h>
#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void     RFID_Init(void);
void     RFID_Task(uint32_t now_ms);
uint8_t  RFID_UIDAvailable(void);
uint8_t  RFID_ReadUID(uint8_t *uid_out);

#ifdef __cplusplus
}
#endif

#endif
