/*
 * Módulo: RFID (RC522) — implementación
 * Dos fuentes de UID que convergen en el mismo buffer (rf_uid):
 *   FUENTE 1 (RFID_UART_INJECT): inyección por UART0 para Proteus.
 *   FUENTE 2 (RFID_USE_RC522): RC522 real vía librería MFRC522 (SPI).
 * RFID_Task no bloquea: sondea la fuente activa y deja el UID listo si lo resuelve.
 */
#include "rfid_rc522.h"
#include "../uart/uart.h"

/* Estado del UID resuelto (compartido por ambas fuentes). */
static uint8_t  rf_uid[RFID_UID_MAX];
static uint8_t  rf_uid_len;
static uint8_t  rf_uid_ready;
static rfid_status_t rf_last_error;

static void rf_clear_uid(void) {
    rf_uid_ready = 0;
    rf_uid_len = 0;
    for (uint8_t i = 0; i < RFID_UID_MAX; i++) rf_uid[i] = 0;
}

/* Guarda un UID resuelto en el buffer compartido y marca "listo". */
static void rf_commit_uid(const uint8_t *uid, uint8_t len) {
    if (len > RFID_UID_MAX) len = RFID_UID_MAX;
    for (uint8_t i = 0; i < len; i++) rf_uid[i] = uid[i];
    for (uint8_t i = len; i < RFID_UID_MAX; i++) rf_uid[i] = 0;
    rf_uid_len = len;
    rf_uid_ready = 1;
    rf_last_error = RFID_OK;
}

/* ==================================================================
 *  FUENTE 1 — Inyección de UID por UART0 (simulación Proteus / pruebas)
 *  Formato aceptado: pares hex + Enter, p.ej.  "AABBCCDD\n"
 * ================================================================== */
#if RFID_UART_INJECT

static uint8_t sim_state;
static uint8_t sim_uid[RFID_UID_MAX];
static uint8_t sim_uid_count;
static uint8_t sim_nibble;

/* Convierte un carácter hex a su valor 0-15, o 0xFF si no es hex. */
static uint8_t sim_hex_val(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0xFF;
}

static void sim_reset(void) {
    sim_state = 0;
    sim_uid_count = 0;
    sim_nibble = 0;
}

/* Lee caracteres de UART0 y arma el UID. Acepta 4, 7 o 10 bytes (ISO14443A). */
static void rf_poll_uart_inject(void) {
    while (UART_Available()) {
        char c = UART_ReadChar();
        uint8_t hv = sim_hex_val(c);

        if (hv != 0xFF) {
            if (sim_state == 0 || sim_state == 2) {
                sim_nibble = hv;
                sim_state = 1;
            } else {
                if (sim_uid_count < RFID_UID_MAX) {
                    sim_uid[sim_uid_count++] = (uint8_t)((sim_nibble << 4) | hv);
                }
                sim_state = 2;
            }
        } else if (c == '\n' || c == '\r') {
            /* Acepta 4, 7 o 10 bytes (longitudes ISO14443A validas) */
            if (sim_uid_count == 4 || sim_uid_count == 7 || sim_uid_count == 10) {
                rf_commit_uid(sim_uid, sim_uid_count);
            }
            sim_reset();
            if (rf_uid_ready) return;   /* dejar el resto para el proximo ciclo */
        } else {
            /* caracter no valido: descartar la captura en curso */
            sim_reset();
        }
    }
}

#endif /* RFID_UART_INJECT */

/* ==================================================================
 *  FUENTE 2 — RC522 real por SPI (librería MFRC522)
 * ================================================================== */
#if RFID_USE_RC522

#include "rfid_rc522_lib.h"

static uint8_t rf_poll_rc522(uint32_t now_ms) {
    uint8_t uid[RFID_UID_MAX] = {0};
    uint8_t uid_len = 0;
    rfid_status_t st = RFID_NO_CARD;

    if (!rfid_lib_poll(now_ms, uid, &uid_len, &st)) {
        rf_last_error = st;
        return 0;
    }

    rf_commit_uid(uid, uid_len);
    return 1;
}

#endif /* RFID_USE_RC522 */

/* ==================================================================
 *  API pública
 * ================================================================== */

void RFID_Init(void) {
    rf_last_error = RFID_NO_CARD;
    rf_clear_uid();

#if RFID_UART_INJECT
    sim_reset();
#endif
#if RFID_USE_RC522
    rfid_lib_init();
#endif
}

void RFID_Task(uint32_t now_ms) {
    if (rf_uid_ready) return;

#if RFID_USE_RC522
    if (rf_poll_rc522(now_ms)) return;
#else
    (void)now_ms;
#endif

#if RFID_UART_INJECT
    rf_poll_uart_inject();
#endif
}

uint8_t RFID_UIDAvailable(void) {
    return rf_uid_ready;
}

rfid_status_t RFID_ReadUIDEx(uint8_t *uid_out, uint8_t *uid_len) {
    if (!rf_uid_ready) return RFID_NO_CARD;
    if (!uid_out) return RFID_HW_ERROR;
    if (uid_len) *uid_len = rf_uid_len;
    for (uint8_t i = 0; i < rf_uid_len; i++) uid_out[i] = rf_uid[i];
    for (uint8_t i = rf_uid_len; i < RFID_UID_MAX; i++) uid_out[i] = 0;
    rf_uid_ready = 0;
    rf_last_error = RFID_OK;
#if RFID_USE_RC522
    rfid_lib_clear_cooldown();
#endif
    return RFID_OK;
}

rfid_status_t RFID_GetLastError(void) {
    return rf_last_error;
}

uint8_t RFID_ReadUID(uint8_t *uid_out) {
    return RFID_ReadUIDEx(uid_out, 0) == RFID_OK;
}
