#ifndef EEPROM_DRIVER_H
#define EEPROM_DRIVER_H

#include <stdint.h>
#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EEPROM_HEADER_ADDR      0x000
#define EEPROM_USERS_ADDR       0x010
#define EEPROM_USER_SIZE        ((uint8_t)sizeof(user_record_t))
#define EEPROM_TOTAL_SIZE       0x1000

void     EEPROM_Init(void);
void     EEPROM_Task(void);
uint8_t  EEPROM_ReadByte(uint16_t addr);
void     EEPROM_WriteByte(uint16_t addr, uint8_t data);
void     EEPROM_ReadBlock(uint16_t addr, uint8_t *buf, uint8_t len);
void     EEPROM_WriteBlock(uint16_t addr, const uint8_t *buf, uint8_t len);

uint8_t  EEPROM_IsValid(void);
void     EEPROM_Format(void);
uint8_t  EEPROM_GetUserCount(void);
uint8_t  EEPROM_LoadUser(uint8_t index, user_record_t *out);
uint8_t  EEPROM_SaveUser(uint8_t index, const user_record_t *user);
uint8_t  EEPROM_FindUserByUid(const uint8_t *uid, uint8_t *index_out);
uint8_t  EEPROM_FindFreeSlot(uint8_t *index_out);
uint8_t  EEPROM_DeleteUser(uint8_t index);
uint8_t  EEPROM_UpdateGameCredits(uint8_t index, uint8_t credits);

#ifdef __cplusplus
}
#endif

#endif
