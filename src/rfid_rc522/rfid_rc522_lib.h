/*
 * Adaptador C sobre la librería Arduino MFRC522 (miguelbalboa/rfid).
 * Solo se compila cuando RFID_USE_RC522=1.
 */
#ifndef RFID_RC522_LIB_H
#define RFID_RC522_LIB_H

#include <stdint.h>
#include "rfid_rc522.h"

#ifdef __cplusplus
extern "C" {
#endif

void     rfid_lib_init(void);
uint8_t  rfid_lib_poll(uint32_t now_ms, uint8_t *uid, uint8_t *uid_len, rfid_status_t *status);
void     rfid_lib_clear_cooldown(void);

#ifdef __cplusplus
}
#endif

#endif
