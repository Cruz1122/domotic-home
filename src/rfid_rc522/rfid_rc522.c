#include "rfid_rc522.h"

/* ============================================================
 *  NOTA SOBRE RFID_SIMULATED
 *
 *  Si se define RFID_SIMULATED (en Definiciones.h), este driver
 *  no usa SPI/RC522 real. Lee UIDs desde UART (Virtual Terminal
 *  en Proteus). Formato: "01 02 03 04 05\n" (5 bytes hex + Enter).
 * ============================================================ */

static uint8_t  rf_state;
static uint32_t rf_tick;
static uint32_t rf_poll_tick;
static uint8_t  rf_uid[RFID_UID_LEN];
static uint8_t  rf_uid_len;
static uint8_t  rf_uid_ready;

#ifndef RFID_SIMULATED

/* --- Driver real RC522 por SPI --- */

#include "../spi/spi.h"
#include "../gpio/gpio.h"

#define RC522_REG_COMMAND      0x01
#define RC522_REG_COMIEN       0x02
#define RC522_REG_COMIRQ       0x04
#define RC522_REG_ERROR        0x06
#define RC522_REG_STATUS2      0x08
#define RC522_REG_FIFO_DATA    0x09
#define RC522_REG_FIFO_LEVEL   0x0A
#define RC522_REG_BIT_FRAMING  0x0D
#define RC522_REG_TX_CONTROL   0x14
#define RC522_REG_TX_ASK       0x15
#define RC522_REG_RF_CFG       0x26
#define RC522_REG_TMODE        0x2A
#define RC522_REG_TPRESCALER   0x2B
#define RC522_REG_TRELOAD_H    0x2C
#define RC522_REG_TRELOAD_L    0x2D
#define RC522_REG_VERSION      0x37

#define RC522_CMD_IDLE         0x00
#define RC522_CMD_TRANSCEIVE   0x0C
#define RC522_CMD_SOFTRESET    0x0F

#define RC522_PICC_REQA        0x26
#define RC522_PICC_ANTICOLL    0x93

#define RC522_STATE_IDLE       0
#define RC522_STATE_REQA_WAIT  1
#define RC522_STATE_ANTICOLL   2
#define RC522_STATE_ANTI_WAIT  3
#define RC522_STATE_SELECT     4
#define RC522_STATE_SEL_WAIT   5
#define RC522_STATE_UID_READY  6
#define RC522_STATE_COOLDOWN   7

static void rc522_wait_ms(uint16_t ms) {
    volatile uint32_t n;
    while (ms--) {
        n = 4000;
        while (n--);
    }
}

static void rc522_write_reg(uint8_t addr, uint8_t data) {
    SPI_Select();
    SPI_Transfer((addr << 1) & 0x7E);
    SPI_Transfer(data);
    SPI_Deselect();
}

static uint8_t rc522_read_reg(uint8_t addr) {
    SPI_Select();
    SPI_Transfer(((addr << 1) & 0x7E) | 0x80);
    uint8_t val = SPI_Transfer(0x00);
    SPI_Deselect();
    return val;
}

static void rc522_clear_fifo(void) {
    rc522_write_reg(RC522_REG_FIFO_LEVEL, 0x80);
}

static uint8_t rc522_fifo_available(void) {
    return rc522_read_reg(RC522_REG_FIFO_LEVEL) & 0x7F;
}

static void rc522_fifo_write(const uint8_t *data, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        rc522_write_reg(RC522_REG_FIFO_DATA, data[i]);
    }
}

static void rc522_fifo_read(uint8_t *buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = rc522_read_reg(RC522_REG_FIFO_DATA);
    }
}

static void rc522_start_transceive(uint8_t last_bits) {
    rc522_write_reg(RC522_REG_BIT_FRAMING, last_bits);
    rc522_write_reg(RC522_REG_COMIRQ, 0x7F);
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    if (last_bits) {
        rc522_write_reg(RC522_REG_BIT_FRAMING, last_bits | 0x08);
    } else {
        rc522_write_reg(RC522_REG_BIT_FRAMING, 0x08);
    }
}

static uint8_t rc522_transceive_done(void) {
    uint8_t irq = rc522_read_reg(RC522_REG_COMIRQ);
    return (irq & 0x02) ? 1 : 0;
}

static uint8_t rc522_has_error(void) {
    return rc522_read_reg(RC522_REG_ERROR) & 0x13;
}

static void rc522_antenna_on(void) {
    uint8_t val = rc522_read_reg(RC522_REG_TX_CONTROL);
    if (!(val & 0x03)) {
        rc522_write_reg(RC522_REG_TX_CONTROL, val | 0x03);
    }
}

void RFID_Init(void) {
    GPIO_SetPinMode(PIN_RFID_RST, GPIO_OUT);
    GPIO_WritePin(PIN_RFID_RST, GPIO_LOW);
    rc522_wait_ms(10);
    GPIO_WritePin(PIN_RFID_RST, GPIO_HIGH);
    rc522_wait_ms(10);
    SPI_Init();
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_SOFTRESET);
    rc522_wait_ms(50);
    rc522_write_reg(RC522_REG_TMODE, 0x8D);
    rc522_write_reg(RC522_REG_TPRESCALER, 0x3E);
    rc522_write_reg(RC522_REG_TRELOAD_H, 0x00);
    rc522_write_reg(RC522_REG_TRELOAD_L, 0x0F);
    rc522_write_reg(RC522_REG_TX_ASK, 0x40);
    rc522_write_reg(RC522_REG_RF_CFG, 0x7F);
    rc522_antenna_on();
    rf_state = RC522_STATE_IDLE;
    rf_poll_tick = 0;
    rf_uid_ready = 0;
}

void RFID_Task(uint32_t now_ms) {
    switch (rf_state) {
        case RC522_STATE_IDLE:
            if ((now_ms - rf_poll_tick) < 500 && rf_poll_tick != 0) break;
            rf_poll_tick = now_ms;
            {
                uint8_t cmd = RC522_PICC_REQA;
                rc522_clear_fifo();
                rc522_fifo_write(&cmd, 1);
                rc522_start_transceive(0x07);
            }
            rf_tick = now_ms;
            rf_state = RC522_STATE_REQA_WAIT;
            break;

        case RC522_STATE_REQA_WAIT:
            if ((now_ms - rf_tick) >= 50) {
                rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_IDLE);
                rf_state = RC522_STATE_IDLE;
                break;
            }
            if (!rc522_transceive_done()) break;
            if (rc522_has_error()) {
                rf_state = RC522_STATE_IDLE;
                break;
            }
            if (rc522_fifo_available() < 2) {
                rf_state = RC522_STATE_IDLE;
                break;
            }
            rf_state = RC522_STATE_ANTICOLL;
            break;

        case RC522_STATE_ANTICOLL:
            {
                uint8_t cmd[2] = {RC522_PICC_ANTICOLL, 0x20};
                rc522_clear_fifo();
                rc522_fifo_write(cmd, 2);
                rc522_start_transceive(0x00);
            }
            rf_tick = now_ms;
            rf_state = RC522_STATE_ANTI_WAIT;
            break;

        case RC522_STATE_ANTI_WAIT:
            if ((now_ms - rf_tick) >= 50) {
                rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_IDLE);
                rf_state = RC522_STATE_IDLE;
                break;
            }
            if (!rc522_transceive_done()) break;
            if (rc522_has_error()) {
                rf_state = RC522_STATE_IDLE;
                break;
            }
            {
                uint8_t fl = rc522_fifo_available();
                if (fl < 5) {
                    rf_state = RC522_STATE_IDLE;
                    break;
                }
                uint8_t raw[5];
                rc522_fifo_read(raw, 5);
                rf_uid_len = (RFID_UID_LEN < 5) ? RFID_UID_LEN : 5;
                for (uint8_t i = 0; i < rf_uid_len; i++) {
                    rf_uid[i] = raw[i];
                }
                rf_state = RC522_STATE_SELECT;
            }
            break;

        case RC522_STATE_SELECT:
            {
                uint8_t sbuf[7];
                sbuf[0] = RC522_PICC_ANTICOLL;
                sbuf[1] = 0x70;
                uint8_t bcc = 0;
                for (uint8_t i = 0; i < rf_uid_len; i++) {
                    sbuf[2 + i] = rf_uid[i];
                    bcc ^= rf_uid[i];
                }
                sbuf[2 + rf_uid_len] = bcc;
                rc522_clear_fifo();
                rc522_fifo_write(sbuf, rf_uid_len + 3);
                rc522_start_transceive(0x00);
            }
            rf_tick = now_ms;
            rf_state = RC522_STATE_SEL_WAIT;
            break;

        case RC522_STATE_SEL_WAIT:
            if ((now_ms - rf_tick) >= 50) {
                rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_IDLE);
                rf_state = RC522_STATE_IDLE;
                break;
            }
            if (!rc522_transceive_done()) break;
            if (rc522_has_error()) {
                rf_state = RC522_STATE_IDLE;
                break;
            }
            rf_uid_ready = 1;
            rf_tick = now_ms;
            rf_state = RC522_STATE_COOLDOWN;
            break;

        case RC522_STATE_COOLDOWN:
            if ((now_ms - rf_tick) >= 250) {
                rf_state = RC522_STATE_IDLE;
            }
            break;

        default:
            rf_state = RC522_STATE_IDLE;
            break;
    }
}

#else /* RFID_SIMULATED */

/* --- Simulación: lee UIDs desde UART (Virtual Terminal) --- */

#include "../uart/uart.h"

#define SIM_RX_IDLE      0
#define SIM_RX_NIBBLE_HI 1
#define SIM_RX_NIBBLE_LO 2

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

void RFID_Init(void) {
    sim_state = SIM_RX_IDLE;
    sim_uid_count = 0;
    rf_uid_len = RFID_UID_LEN;
    rf_uid_ready = 0;
}

void RFID_Task(uint32_t now_ms) {
    (void)now_ms;
    if (rf_uid_ready) return;

    while (UART_Available()) {
        char c = UART_ReadChar();
        uint8_t hv = sim_hex_val(c);

        if (hv != 0xFF) {
            if (sim_state == SIM_RX_IDLE || sim_state == SIM_RX_NIBBLE_LO) {
                sim_nibble = hv;
                sim_state = SIM_RX_NIBBLE_HI;
            } else {
                if (sim_uid_count < RFID_UID_LEN) {
                    sim_uid[sim_uid_count] = (sim_nibble << 4) | hv;
                    sim_uid_count++;
                }
                sim_state = SIM_RX_NIBBLE_LO;
            }
        } else if (c == '\n' || c == '\r') {
            if (sim_uid_count >= RFID_UID_LEN) {
                for (uint8_t i = 0; i < RFID_UID_LEN; i++) {
                    rf_uid[i] = sim_uid[i];
                }
                rf_uid_ready = 1;
            }
            sim_uid_count = 0;
            sim_state = SIM_RX_IDLE;
        }
    }
}

#endif /* RFID_SIMULATED */

uint8_t RFID_UIDAvailable(void) {
    return rf_uid_ready;
}

uint8_t RFID_ReadUID(uint8_t *uid_out) {
    if (!rf_uid_ready) return 0;
    for (uint8_t i = 0; i < rf_uid_len; i++) {
        uid_out[i] = rf_uid[i];
    }
    rf_uid_ready = 0;
#ifdef RFID_SIMULATED
    sim_state = SIM_RX_IDLE;
    sim_uid_count = 0;
#else
    rf_state = RC522_STATE_IDLE;
#endif
    return 1;
}
