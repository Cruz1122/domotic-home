#include "eeprom.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>

static uint8_t eeprom_user_count;

static uint8_t eeprom_calc_checksum(const eeprom_header_t *h) {
    return h->magic0 ^ h->magic1 ^ h->version ^ h->user_count;
}

static uint8_t eeprom_header_is_valid(const eeprom_header_t *h) {
    if (h->magic0 != EEPROM_MAGIC_0 || h->magic1 != EEPROM_MAGIC_1) return 0;
    if (h->version != EEPROM_VERSION) return 0;
    if (h->user_count > MAX_USERS) return 0;
    return (h->checksum == eeprom_calc_checksum(h));
}

uint8_t EEPROM_ReadByte(uint16_t addr) {
    while (EECR & (1 << EEPE));
    EEARH = (uint8_t)(addr >> 8);
    EEARL = (uint8_t)(addr);
    EECR |= (1 << EERE);
    return EEDR;
}

void EEPROM_WriteByte(uint16_t addr, uint8_t data) {
    while (EECR & (1 << EEPE));
    EEARH = (uint8_t)(addr >> 8);
    EEARL = (uint8_t)(addr);
    EEDR = data;
    uint8_t sreg = SREG;
    cli();
    EECR |= (1 << EEMPE);
    EECR |= (1 << EEPE);
    SREG = sreg;
}

void EEPROM_ReadBlock(uint16_t addr, uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = EEPROM_ReadByte(addr + i);
    }
}

void EEPROM_WriteBlock(uint16_t addr, const uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        EEPROM_WriteByte(addr + i, buf[i]);
    }
}

uint8_t EEPROM_IsValid(void) {
    eeprom_header_t h;
    EEPROM_ReadBlock(EEPROM_HEADER_ADDR, (uint8_t *)&h, sizeof(h));
    return eeprom_header_is_valid(&h);
}

void EEPROM_Format(void) {
    for (uint8_t i = 0; i < MAX_USERS; i++) {
        EEPROM_WriteByte(EEPROM_USERS_ADDR + i * EEPROM_USER_SIZE, 0);
    }
    eeprom_header_t h;
    h.magic0 = EEPROM_MAGIC_0;
    h.magic1 = EEPROM_MAGIC_1;
    h.version = EEPROM_VERSION;
    h.user_count = 0;
    h.checksum = eeprom_calc_checksum(&h);
    EEPROM_WriteBlock(EEPROM_HEADER_ADDR, (const uint8_t *)&h, sizeof(h));
    eeprom_user_count = 0;
}

void EEPROM_Init(void) {
    if (EEPROM_IsValid()) {
        eeprom_header_t h;
        EEPROM_ReadBlock(EEPROM_HEADER_ADDR, (uint8_t *)&h, sizeof(h));
        eeprom_user_count = h.user_count;
    } else {
        EEPROM_Format();
    }
}

void EEPROM_Task(void) {
}

uint8_t EEPROM_GetUserCount(void) {
    return eeprom_user_count;
}

uint8_t EEPROM_LoadUser(uint8_t index, user_record_t *out) {
    if (index >= MAX_USERS) return 0;
    uint16_t addr = EEPROM_USERS_ADDR + index * EEPROM_USER_SIZE;
    EEPROM_ReadBlock(addr, (uint8_t *)out, sizeof(user_record_t));
    return out->active ? 1 : 0;
}

uint8_t EEPROM_SaveUser(uint8_t index, const user_record_t *user) {
    if (index >= MAX_USERS) return 0;
    uint16_t addr = EEPROM_USERS_ADDR + index * EEPROM_USER_SIZE;
    uint8_t was_active = EEPROM_ReadByte(addr);
    EEPROM_WriteBlock(addr, (const uint8_t *)user, sizeof(user_record_t));
    if (user->active && !was_active) {
        eeprom_user_count++;
        eeprom_header_t h;
        EEPROM_ReadBlock(EEPROM_HEADER_ADDR, (uint8_t *)&h, sizeof(h));
        h.user_count = eeprom_user_count;
        h.checksum = eeprom_calc_checksum(&h);
        EEPROM_WriteBlock(EEPROM_HEADER_ADDR, (const uint8_t *)&h, sizeof(h));
    }
    return 1;
}

uint8_t EEPROM_FindUserByUid(const uint8_t *uid, uint8_t uid_len, uint8_t *index_out) {
    if (uid_len == 0 || uid_len > RFID_UID_MAX) return 0;
    for (uint8_t i = 0; i < MAX_USERS; i++) {
        user_record_t u;
        if (!EEPROM_LoadUser(i, &u)) continue;
        if (u.uid_len != uid_len) continue;
        uint8_t match = 1;
        for (uint8_t j = 0; j < uid_len; j++) {
            if (u.uid[j] != uid[j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            *index_out = i;
            return 1;
        }
    }
    return 0;
}

uint8_t EEPROM_FindFreeSlot(uint8_t *index_out) {
    for (uint8_t i = 0; i < MAX_USERS; i++) {
        user_record_t u;
        EEPROM_ReadBlock(EEPROM_USERS_ADDR + i * EEPROM_USER_SIZE,
                         (uint8_t *)&u, sizeof(u));
        if (!u.active) {
            *index_out = i;
            return 1;
        }
    }
    return 0;
}

uint8_t EEPROM_DeleteUser(uint8_t index) {
    if (index >= MAX_USERS) return 0;
    uint16_t addr = EEPROM_USERS_ADDR + index * EEPROM_USER_SIZE;
    if (!EEPROM_ReadByte(addr)) return 0;
    EEPROM_WriteByte(addr, 0);
    if (eeprom_user_count > 0) eeprom_user_count--;
    eeprom_header_t h;
    EEPROM_ReadBlock(EEPROM_HEADER_ADDR, (uint8_t *)&h, sizeof(h));
    h.user_count = eeprom_user_count;
    h.checksum = eeprom_calc_checksum(&h);
    EEPROM_WriteBlock(EEPROM_HEADER_ADDR, (const uint8_t *)&h, sizeof(h));
    return 1;
}

uint8_t EEPROM_UpdateGameCredits(uint8_t index, uint8_t credits) {
    if (index >= MAX_USERS) return 0;
    uint16_t addr = EEPROM_USERS_ADDR + index * EEPROM_USER_SIZE
                    + offsetof(user_record_t, game_credits);
    EEPROM_WriteByte(addr, credits);
    return 1;
}
