#ifndef RFID_RC522_H
#define RFID_RC522_H

#include <stdint.h>
#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RFID_OK = 0,
    RFID_NO_CARD,
    RFID_TIMEOUT,
    RFID_COLLISION,
    RFID_CRC_ERROR,
    RFID_UNSUPPORTED_UID,
    RFID_HW_ERROR
} rfid_status_t;

void     RFID_Init(void);
void     RFID_Task(uint32_t now_ms);
uint8_t  RFID_UIDAvailable(void);
rfid_status_t RFID_ReadUIDEx(uint8_t *uid_out, uint8_t *uid_len);
rfid_status_t RFID_GetLastError(void);
uint8_t  RFID_ReadUID(uint8_t *uid_out);

#ifdef __cplusplus
}
#endif

#endif
