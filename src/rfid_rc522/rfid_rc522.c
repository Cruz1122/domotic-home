#include "rfid_rc522.h"

static uint8_t  rf_uid[RFID_UID_LEN];
static uint8_t  rf_uid_len;
static uint8_t  rf_uid_ready;
static rfid_status_t rf_last_error;

#if RFID_SIMULATED

#include "../uart/uart.h"

static uint8_t sim_state;
static uint8_t sim_uid[RFID_UID_LEN];
static uint8_t sim_uid_count;
static uint8_t sim_nibble;

static uint8_t sim_hex_val(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
    return 0xFF;
}

#else

#include "../spi/spi.h"
#include "../gpio/gpio.h"
#include "../uart/uart.h"

#define RC522_REG_COMMAND      0x01
#define RC522_REG_COM_IRQ      0x04
#define RC522_REG_DIV_IRQ      0x05
#define RC522_REG_ERROR        0x06
#define RC522_REG_STATUS2      0x08
#define RC522_REG_FIFO_DATA    0x09
#define RC522_REG_FIFO_LEVEL   0x0A
#define RC522_REG_CONTROL      0x0C
#define RC522_REG_BIT_FRAMING  0x0D
#define RC522_REG_COLL         0x0E
#define RC522_REG_TX_CONTROL   0x14
#define RC522_REG_TX_ASK       0x15
#define RC522_REG_CRC_RESULT_L 0x22
#define RC522_REG_CRC_RESULT_H 0x21
#define RC522_REG_RF_CFG       0x26
#define RC522_REG_TMODE        0x2A
#define RC522_REG_TPRESCALER   0x2B
#define RC522_REG_TRELOAD_H    0x2C
#define RC522_REG_TRELOAD_L    0x2D
#define RC522_REG_VERSION      0x37

#define RC522_CMD_IDLE         0x00
#define RC522_CMD_CALCCRC      0x03
#define RC522_CMD_TRANSCEIVE   0x0C
#define RC522_CMD_SOFTRESET    0x0F

#define RC522_PICC_REQA        0x26
#define RC522_PICC_WUPA        0x52
#define RC522_PICC_ANTICOLL    0x93
#define RC522_PICC_SELECT      0x70

#define RC522_IRQ_RX           0x20
#define RC522_IRQ_IDLE         0x10
#define RC522_IRQ_TIMER        0x01

#define RC522_ERR_COLLISION    0x08
#define RC522_ERR_PARITY       0x02
#define RC522_ERR_PROTOCOL     0x01

#define RFID_POLL_MS           200U
#define RFID_COOLDOWN_MS       250U
#define RFID_LOG_GAP_MS       2000U
#define RC522_TIMEOUT_TICKS   4000U

static uint8_t  rf_enabled;
static uint32_t rf_poll_tick;
static uint32_t rf_cooldown_until;
static uint32_t rf_last_log_ms;

static void rc522_delay(volatile uint16_t ticks) {
    while (ticks--) {
        __asm__ __volatile__("nop");
    }
}

static uint8_t spi_xfer(uint8_t data, uint8_t *out) {
    return SPI_Transfer(data, out, RC522_TIMEOUT_TICKS);
}

static uint8_t rc522_write_reg(uint8_t addr, uint8_t val) {
    uint8_t dummy;
    SPI_Select();
    if (!spi_xfer((uint8_t)((addr << 1) & 0x7E), &dummy)) { SPI_Deselect(); return 0; }
    if (!spi_xfer(val, &dummy)) { SPI_Deselect(); return 0; }
    SPI_Deselect();
    return 1;
}

static uint8_t rc522_read_reg(uint8_t addr, uint8_t *val) {
    uint8_t dummy;
    SPI_Select();
    if (!spi_xfer((uint8_t)(((addr << 1) & 0x7E) | 0x80), &dummy)) { SPI_Deselect(); return 0; }
    if (!spi_xfer(0x00, val)) { SPI_Deselect(); return 0; }
    SPI_Deselect();
    return 1;
}

static uint8_t rc522_clear_fifo(void) {
    return rc522_write_reg(RC522_REG_FIFO_LEVEL, 0x80);
}

static uint8_t rc522_fifo_write(const uint8_t *data, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (!rc522_write_reg(RC522_REG_FIFO_DATA, data[i])) return 0;
    }
    return 1;
}

static uint8_t rc522_fifo_read(uint8_t *data, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        if (!rc522_read_reg(RC522_REG_FIFO_DATA, &data[i])) return 0;
    }
    return 1;
}

static uint8_t rc522_calc_crc(const uint8_t *data, uint8_t len, uint8_t out[2]) {
    uint8_t irq;
    if (!rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_IDLE)) return 0;
    if (!rc522_write_reg(RC522_REG_DIV_IRQ, 0x04)) return 0;
    if (!rc522_clear_fifo()) return 0;
    if (!rc522_fifo_write(data, len)) return 0;
    if (!rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_CALCCRC)) return 0;

    for (uint16_t t = 0; t < RC522_TIMEOUT_TICKS; t++) {
        if (!rc522_read_reg(RC522_REG_DIV_IRQ, &irq)) return 0;
        if (irq & 0x04) {
            if (!rc522_read_reg(RC522_REG_CRC_RESULT_L, &out[0])) return 0;
            if (!rc522_read_reg(RC522_REG_CRC_RESULT_H, &out[1])) return 0;
            return 1;
        }
    }
    return 0;
}

static uint8_t rc522_transceive(const uint8_t *tx, uint8_t tx_len, uint8_t tx_last_bits, uint8_t *rx, uint8_t *rx_len, uint8_t *err_reg, rfid_status_t *status) {
    uint8_t irq = 0;
    uint8_t err = 0;
    uint8_t fifo_len = 0;

    if (!rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_IDLE)) { *status = RFID_HW_ERROR; return 0; }
    if (!rc522_write_reg(RC522_REG_COM_IRQ, 0x7F)) { *status = RFID_HW_ERROR; return 0; }
    if (!rc522_clear_fifo()) { *status = RFID_HW_ERROR; return 0; }
    if (!rc522_write_reg(RC522_REG_BIT_FRAMING, (uint8_t)(tx_last_bits & 0x07))) { *status = RFID_HW_ERROR; return 0; }
    if (!rc522_fifo_write(tx, tx_len)) { *status = RFID_HW_ERROR; return 0; }
    if (!rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE)) { *status = RFID_HW_ERROR; return 0; }
    if (!rc522_write_reg(RC522_REG_BIT_FRAMING, (uint8_t)((tx_last_bits & 0x07) | 0x80))) { *status = RFID_HW_ERROR; return 0; }

    for (uint16_t t = 0; t < RC522_TIMEOUT_TICKS; t++) {
        if (!rc522_read_reg(RC522_REG_COM_IRQ, &irq)) { *status = RFID_HW_ERROR; return 0; }
        if (irq & (RC522_IRQ_RX | RC522_IRQ_IDLE | RC522_IRQ_TIMER)) break;
        rc522_delay(1);
    }
    if (!(irq & (RC522_IRQ_RX | RC522_IRQ_IDLE | RC522_IRQ_TIMER))) { *status = RFID_TIMEOUT; return 0; }

    if (!rc522_read_reg(RC522_REG_ERROR, &err)) { *status = RFID_HW_ERROR; return 0; }
    if (err_reg) *err_reg = err;
    if (err & RC522_ERR_COLLISION) { *status = RFID_COLLISION; return 0; }
    if (err & (RC522_ERR_PARITY | RC522_ERR_PROTOCOL)) { *status = RFID_CRC_ERROR; return 0; }

    if (!rc522_read_reg(RC522_REG_FIFO_LEVEL, &fifo_len)) { *status = RFID_HW_ERROR; return 0; }
    fifo_len &= 0x7F;
    if (fifo_len == 0) { *status = RFID_NO_CARD; return 0; }
    if (fifo_len > *rx_len) fifo_len = *rx_len;
    if (!rc522_fifo_read(rx, fifo_len)) { *status = RFID_HW_ERROR; return 0; }
    *rx_len = fifo_len;
    *status = RFID_OK;
    return 1;
}

static void rf_print_hex8(uint8_t v) {
    static const char hex[] = "0123456789ABCDEF";
    UART_WriteChar(hex[(v >> 4) & 0x0F]);
    UART_WriteChar(hex[v & 0x0F]);
}

static void rf_log_status(uint32_t now_ms, rfid_status_t st, uint8_t err_reg) {
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
    UART_WriteString(" ERR=0x");
    rf_print_hex8(err_reg);
    UART_Newline();
}

static rfid_status_t rf_request(uint8_t cmd, uint32_t now_ms) {
    uint8_t tx = cmd;
    uint8_t rx[2] = {0, 0};
    uint8_t rx_len = sizeof(rx);
    uint8_t err_reg = 0;
    rfid_status_t st = RFID_OK;
    if (!rc522_transceive(&tx, 1, 0x07, rx, &rx_len, &err_reg, &st)) {
        if (st == RFID_TIMEOUT) return RFID_NO_CARD;
        rf_log_status(now_ms, st, err_reg);
        return st;
    }
    return (rx_len >= 2) ? RFID_OK : RFID_NO_CARD;
}

static rfid_status_t rf_anticoll(uint8_t uid[4], uint32_t now_ms) {
    uint8_t tx[2] = { RC522_PICC_ANTICOLL, 0x20 };
    uint8_t rx[5] = {0};
    uint8_t rx_len = sizeof(rx);
    uint8_t err_reg = 0;
    rfid_status_t st = RFID_OK;
    if (!rc522_transceive(tx, 2, 0x00, rx, &rx_len, &err_reg, &st)) {
        rf_log_status(now_ms, st, err_reg);
        return st;
    }
    if (rx_len != 5) return RFID_TIMEOUT;
    if ((uint8_t)(rx[0] ^ rx[1] ^ rx[2] ^ rx[3]) != rx[4]) return RFID_CRC_ERROR;
    for (uint8_t i = 0; i < 4; i++) uid[i] = rx[i];
    return RFID_OK;
}

static rfid_status_t rf_select(uint8_t uid[4], uint32_t now_ms) {
    uint8_t tx[9] = {0};
    uint8_t rx[3] = {0};
    uint8_t rx_len = sizeof(rx);
    uint8_t err_reg = 0;
    rfid_status_t st = RFID_OK;

    tx[0] = RC522_PICC_ANTICOLL;
    tx[1] = RC522_PICC_SELECT;
    tx[2] = uid[0];
    tx[3] = uid[1];
    tx[4] = uid[2];
    tx[5] = uid[3];
    tx[6] = (uint8_t)(uid[0] ^ uid[1] ^ uid[2] ^ uid[3]);
    if (!rc522_calc_crc(tx, 7, &tx[7])) return RFID_HW_ERROR;

    if (!rc522_transceive(tx, 9, 0x00, rx, &rx_len, &err_reg, &st)) {
        rf_log_status(now_ms, st, err_reg);
        return st;
    }
    if (rx_len == 0) return RFID_TIMEOUT;
    if (rx[0] & 0x04) return RFID_UNSUPPORTED_UID;
    return RFID_OK;
}

#endif

static void rf_clear_uid(void) {
    rf_uid_ready = 0;
    rf_uid_len = 0;
    for (uint8_t i = 0; i < RFID_UID_LEN; i++) rf_uid[i] = 0;
}

void RFID_Init(void) {
    rf_last_error = RFID_NO_CARD;
    rf_clear_uid();

#if RFID_SIMULATED
    sim_state = 0;
    sim_uid_count = 0;
    sim_nibble = 0;
#else
    rf_enabled = 1;
    rf_poll_tick = 0;
    rf_cooldown_until = 0;
    rf_last_log_ms = 0;

    GPIO_SetPinMode(PIN_RFID_RST, GPIO_OUT);
    GPIO_WritePin(PIN_RFID_RST, GPIO_LOW);
    rc522_delay(5000);
    GPIO_WritePin(PIN_RFID_RST, GPIO_HIGH);
    rc522_delay(5000);

    SPI_Init();
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_SOFTRESET);
    rc522_delay(50000);
    rc522_write_reg(RC522_REG_TMODE, 0x8D);
    rc522_write_reg(RC522_REG_TPRESCALER, 0x3E);
    rc522_write_reg(RC522_REG_TRELOAD_H, 0x00);
    rc522_write_reg(RC522_REG_TRELOAD_L, 0x0F);
    rc522_write_reg(RC522_REG_TX_ASK, 0x40);
    rc522_write_reg(RC522_REG_RF_CFG, 0x7F);
    rc522_write_reg(RC522_REG_STATUS2, 0x08);
    rc522_write_reg(RC522_REG_COLL, 0x80);

    uint8_t version = 0;
    if (!rc522_read_reg(RC522_REG_VERSION, &version)) {
        rf_last_error = RFID_HW_ERROR;
        rf_enabled = 0;
        UART_WriteEvent(SER_RFID, "RC522 VersionReg read failed");
        return;
    }
    UART_WriteString(SER_RFID "VersionReg=0x");
    rf_print_hex8(version);
    UART_Newline();
    if (version == 0x00 || version == 0xFF) {
        rf_last_error = RFID_HW_ERROR;
        rf_enabled = 0;
        UART_WriteEvent(SER_RFID, "RC522 version invalida, RFID deshabilitado");
        return;
    }

    rc522_write_reg(RC522_REG_TX_CONTROL, 0x03);
#endif
}

void RFID_Task(uint32_t now_ms) {
#if RFID_SIMULATED
    if (rf_uid_ready) return;

    while (UART_Available()) {
        char c = UART_ReadChar();
        uint8_t hv = sim_hex_val(c);

        if (hv != 0xFF) {
            if (sim_state == 0 || sim_state == 2) {
                sim_nibble = hv;
                sim_state = 1;
            } else {
                if (sim_uid_count < RFID_UID_LEN) {
                    sim_uid[sim_uid_count++] = (uint8_t)((sim_nibble << 4) | hv);
                }
                sim_state = 2;
            }
        } else if (c == '\n' || c == '\r') {
            if (sim_uid_count >= RFID_UID_LEN) {
                for (uint8_t i = 0; i < RFID_UID_LEN; i++) rf_uid[i] = sim_uid[i];
                rf_uid_len = RFID_UID_LEN;
                rf_uid_ready = 1;
                rf_last_error = RFID_OK;
            }
            sim_uid_count = 0;
            sim_state = 0;
        }
    }
#else
    if (!rf_enabled || rf_uid_ready) return;
    if (rf_cooldown_until != 0 && now_ms < rf_cooldown_until) return;
    if (rf_cooldown_until != 0 && now_ms >= rf_cooldown_until) rf_cooldown_until = 0;
    if (rf_poll_tick != 0 && (now_ms - rf_poll_tick) < RFID_POLL_MS) return;
    rf_poll_tick = now_ms;

    rfid_status_t st = rf_request(RC522_PICC_REQA, now_ms);
    if (st != RFID_OK) {
        rf_last_error = st;
        return;
    }

    uint8_t uid[4] = {0};
    st = rf_anticoll(uid, now_ms);
    if (st != RFID_OK) {
        rf_last_error = st;
        return;
    }

    st = rf_select(uid, now_ms);
    if (st != RFID_OK) {
        rf_last_error = st;
        rf_log_status(now_ms, st, 0x00);
        return;
    }

    for (uint8_t i = 0; i < 4 && i < RFID_UID_LEN; i++) rf_uid[i] = uid[i];
    for (uint8_t i = 4; i < RFID_UID_LEN; i++) rf_uid[i] = 0;
    rf_uid_len = 4;
    rf_uid_ready = 1;
    rf_last_error = RFID_OK;
    rf_cooldown_until = now_ms + RFID_COOLDOWN_MS;
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
    for (uint8_t i = rf_uid_len; i < RFID_UID_LEN; i++) uid_out[i] = 0;
    rf_uid_ready = 0;
    rf_last_error = RFID_OK;
#if !RFID_SIMULATED
    rf_cooldown_until = 0;
#endif
    return RFID_OK;
}

rfid_status_t RFID_GetLastError(void) {
    return rf_last_error;
}

uint8_t RFID_ReadUID(uint8_t *uid_out) {
    return RFID_ReadUIDEx(uid_out, 0) == RFID_OK;
}
