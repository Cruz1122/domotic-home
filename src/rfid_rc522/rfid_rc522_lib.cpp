/*
 * Adaptador MFRC522: usa SPI.h + librería MFRC522 para lectura de UID.
 * Mantiene sondeo no bloqueante, cooldown y HALT como el driver anterior.
 */
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#include "rfid_rc522_lib.h"
#include "../common/Definiciones.h"
#include "../uart/uart.h"

static MFRC522 mfrc522(PIN_RFID_SS, PIN_RFID_RST);

static uint8_t  rf_enabled;
static uint32_t rf_poll_tick;
static uint32_t rf_cooldown_until;
static uint32_t rf_last_log_ms;

#define RFID_POLL_MS      200U
#define RFID_COOLDOWN_MS  250U
#define RFID_LOG_GAP_MS  2000U

static void rf_print_hex8(uint8_t v) {
    static const char hex[] = "0123456789ABCDEF";
    UART_WriteChar(hex[(v >> 4) & 0x0F]);
    UART_WriteChar(hex[v & 0x0F]);
}

static void rf_log_status(uint32_t now_ms, rfid_status_t st) {
    if (st == RFID_NO_CARD) return;
    if ((now_ms - rf_last_log_ms) < RFID_LOG_GAP_MS) return;
    rf_last_log_ms = now_ms;
    UART_WriteString(SER_RFID "Error ");
    switch (st) {
        case RFID_TIMEOUT: UART_WriteString("TIMEOUT"); break;
        case RFID_COLLISION: UART_WriteString("COLLISION"); break;
        case RFID_CRC_ERROR: UART_WriteString("CRC"); break;
        case RFID_UNSUPPORTED_UID: UART_WriteString("UNSUPPORTED_UID"); break;
        case RFID_HW_ERROR: UART_WriteString("HW"); break;
        default: UART_WriteString("UNKNOWN"); break;
    }
    UART_Newline();
}

void rfid_lib_init(void) {
    rf_enabled = 1;
    rf_poll_tick = 0;
    rf_cooldown_until = 0;
    rf_last_log_ms = 0;

    SPI.begin();
    mfrc522.PCD_Init();

    uint8_t version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (version == 0x00 || version == 0xFF) {
        rf_enabled = 0;
        UART_WriteEvent(SER_RFID, "RC522 ausente, lectura por UART habilitada");
        return;
    }

    UART_WriteString(SER_RFID "VersionReg=0x");
    rf_print_hex8(version);
    UART_Newline();
}

uint8_t rfid_lib_poll(uint32_t now_ms, uint8_t *uid, uint8_t *uid_len, rfid_status_t *status) {
    if (!status) return 0;
    *status = RFID_NO_CARD;

    if (!rf_enabled || !uid || !uid_len) return 0;

    if (rf_cooldown_until != 0 && now_ms < rf_cooldown_until) return 0;
    if (rf_cooldown_until != 0 && now_ms >= rf_cooldown_until) rf_cooldown_until = 0;
    if (rf_poll_tick != 0 && (now_ms - rf_poll_tick) < RFID_POLL_MS) return 0;
    rf_poll_tick = now_ms;

    if (!mfrc522.PICC_IsNewCardPresent()) return 0;

    if (!mfrc522.PICC_ReadCardSerial()) {
        *status = RFID_HW_ERROR;
        rf_log_status(now_ms, RFID_HW_ERROR);
        return 0;
    }

    uint8_t sz = mfrc522.uid.size;
    if (sz == 0 || sz > RFID_UID_MAX) {
        mfrc522.PICC_HaltA();
        *status = RFID_UNSUPPORTED_UID;
        rf_log_status(now_ms, RFID_UNSUPPORTED_UID);
        return 0;
    }

    for (uint8_t i = 0; i < sz; i++) uid[i] = mfrc522.uid.uidByte[i];
    *uid_len = sz;
    mfrc522.PICC_HaltA();
    rf_cooldown_until = now_ms + RFID_COOLDOWN_MS;
    *status = RFID_OK;
    return 1;
}

void rfid_lib_clear_cooldown(void) {
    rf_cooldown_until = 0;
}
