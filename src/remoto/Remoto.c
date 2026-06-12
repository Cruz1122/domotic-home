#include "Remoto.h"
#include "../uart/uart.h"
#include "../confort/Confort.h"
#include "../gpio/gpio.h"
#include "../timer/timer.h"
#include "../eeprom/eeprom.h"
#include <string.h>

#define OVEN_MINUTE_MS 5000UL

/* Oven state */
static uint8_t  oven_running;
static uint16_t oven_target_temp;
static uint16_t oven_remaining_min;
static uint32_t oven_last_tick;

/* UART1 command buffer */
static char   cmd_buf[32];
static uint8_t cmd_len;

/* UART2/UART3 slave response buffers */
static char   radio_rx_buf[48];
static uint8_t radio_rx_len;
static char   oven_rx_buf[48];
static uint8_t oven_rx_len;

/* ---- Forward helpers ---- */

static void forward_radio(const char *cmd) {
    UART2_WriteString(cmd);
    UART2_WriteChar('\n');
    UART_WriteString("[->RADIO] ");
    UART_WriteString(cmd);
    UART_Newline();
}

static void forward_horno(const char *cmd) {
    UART3_WriteString(cmd);
    UART3_WriteChar('\n');
    UART_WriteString("[->HORNO] ");
    UART_WriteString(cmd);
    UART_Newline();
}

/* ---- Internal helpers ---- */

static uint8_t parse_uint8(const char *s) {
    uint8_t v = 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (uint8_t)(*s - '0');
        s++;
    }
    return v;
}

static void remote_reply(const char *msg) {
    UART1_WriteString(msg);
    UART1_WriteChar('\r');
    UART1_WriteChar('\n');
    UART_WriteString(msg);
    UART_Newline();
}

static void radio_status(void) {
    char buf[32];
    uint8_t i = 0;

    buf[i++] = '['; buf[i++] = 'S'; buf[i++] = 'T'; buf[i++] = 'A';
    buf[i++] = 'T'; buf[i++] = 'U'; buf[i++] = 'S'; buf[i++] = ']';
    buf[i++] = '['; buf[i++] = 'R'; buf[i++] = 'A'; buf[i++] = 'D';
    buf[i++] = 'I'; buf[i++] = 'O'; buf[i++] = ']'; buf[i++] = ' ';

    if (Confort_IsSoundEnabled()) {
        buf[i++] = 'O'; buf[i++] = 'N';
    } else {
        buf[i++] = 'O'; buf[i++] = 'F'; buf[i++] = 'F';
    }
    buf[i++] = ' ';
    buf[i++] = 'V'; buf[i++] = 'O'; buf[i++] = 'L'; buf[i++] = '=';

    uint8_t pct = Confort_GetVolumePercent();
    if (pct >= 100) {
        buf[i++] = '1'; buf[i++] = '0'; buf[i++] = '0';
    } else if (pct >= 10) {
        buf[i++] = (char)('0' + pct / 10);
        buf[i++] = (char)('0' + pct % 10);
    } else {
        buf[i++] = (char)('0' + pct);
    }
    buf[i] = '\0';

    remote_reply(buf);
}

static void oven_status(void) {
    char buf[48];
    uint8_t i = 0;
    uint16_t t, m;

    buf[i++] = '['; buf[i++] = 'S'; buf[i++] = 'T'; buf[i++] = 'A';
    buf[i++] = 'T'; buf[i++] = 'U'; buf[i++] = 'S'; buf[i++] = ']';
    buf[i++] = '['; buf[i++] = 'H'; buf[i++] = 'O'; buf[i++] = 'R';
    buf[i++] = 'N'; buf[i++] = 'O'; buf[i++] = ']'; buf[i++] = ' ';

    if (oven_running) {
        buf[i++] = 'O'; buf[i++] = 'N';
    } else {
        buf[i++] = 'O'; buf[i++] = 'F'; buf[i++] = 'F';
    }
    buf[i++] = ' ';
    buf[i++] = 'T'; buf[i++] = 'E'; buf[i++] = 'M'; buf[i++] = 'P'; buf[i++] = '=';

    t = oven_target_temp;
    if (t >= 100) { buf[i++] = (char)('0' + t / 100); t = (uint16_t)(t % 100); }
    if (oven_target_temp >= 10) { buf[i++] = (char)('0' + t / 10); t = (uint16_t)(t % 10); }
    buf[i++] = (char)('0' + t);

    buf[i++] = ' ';
    buf[i++] = 'R'; buf[i++] = 'E'; buf[i++] = 'S'; buf[i++] = 'T';
    buf[i++] = 'A'; buf[i++] = 'N'; buf[i++] = '=';

    m = oven_remaining_min;
    if (m >= 100) { buf[i++] = (char)('0' + m / 100); m = (uint16_t)(m % 100); }
    if (oven_remaining_min >= 10) { buf[i++] = (char)('0' + m / 10); m = (uint16_t)(m % 10); }
    buf[i++] = (char)('0' + m);

    buf[i++] = 'm'; buf[i++] = 'i'; buf[i++] = 'n';
    buf[i] = '\0';

    remote_reply(buf);
}

/* ---- Market list ---- */

static void market_save(void);
static void market_load(void);

static const char * const product_names[MARKET_PRODUCT_COUNT] = {
    "Pan", "Leche", "Huevos", "Arroz",
    "Cafe", "Azucar", "Aceite", "Fruta"
};

static market_item_t market[MARKET_MAX_ITEMS];
static uint8_t market_count;

uint8_t Remoto_MarketAdd(uint8_t product_id, uint8_t quantity) {
    if (product_id < 1 || product_id > MARKET_PRODUCT_COUNT) return 0;
    if (quantity < 1 || quantity > 99) return 0;

    for (uint8_t i = 0; i < market_count; i++) {
        if (market[i].product_id == product_id) {
            uint16_t new_qty = (uint16_t)market[i].quantity + quantity;
            market[i].quantity = (new_qty > 99) ? 99 : (uint8_t)new_qty;
            market_save();
            return 1;
        }
    }
    if (market_count >= MARKET_MAX_ITEMS) return 0;
    market[market_count].product_id = product_id;
    market[market_count].quantity = quantity;
    market_count++;
    market_save();
    return 1;
}

void Remoto_MarketClear(void) {
    market_count = 0;
    market_save();
}

uint8_t Remoto_MarketGetCount(void) {
    return market_count;
}

uint8_t Remoto_MarketGetItem(uint8_t index, market_item_t *out) {
    if (index >= market_count || out == 0) return 0;
    *out = market[index];
    return 1;
}

const char* Remoto_MarketGetProductName(uint8_t product_id) {
    if (product_id < 1 || product_id > MARKET_PRODUCT_COUNT) return 0;
    return product_names[product_id - 1];
}

uint8_t Remoto_MarketGetProductCount(void) {
    return MARKET_PRODUCT_COUNT;
}

/* ---- Oven control ---- */

static void oven_start(uint16_t temp, uint16_t min) {
    oven_running = 1;
    oven_target_temp = temp;
    oven_remaining_min = min;
    oven_last_tick = Timer_GetMs();
    GPIO_WritePin(PIN_OVEN_LED, GPIO_HIGH);
}

static void oven_stop(void) {
    oven_running = 0;
    GPIO_WritePin(PIN_OVEN_LED, GPIO_LOW);
}

/* ---- Command parser ---- */

static void process_command(void) {
    cmd_buf[cmd_len] = '\0';

    if (strcmp(cmd_buf, "HELP") == 0) {
        remote_reply("Comandos: RADIO ON/OFF/VOL n/STATUS | HORNO ON temp min/OFF/STATUS");

    } else if (strcmp(cmd_buf, "RADIO ON") == 0) {
        Confort_SetSoundEnabled(1);
        remote_reply("[OK][RADIO] ON");
        forward_radio(cmd_buf);

    } else if (strcmp(cmd_buf, "RADIO OFF") == 0) {
        Confort_SetSoundEnabled(0);
        remote_reply("[OK][RADIO] OFF");
        forward_radio(cmd_buf);

    } else if (strncmp(cmd_buf, "RADIO VOL ", 10) == 0) {
        uint8_t pct = parse_uint8(cmd_buf + 10);
        if (pct <= 100) {
            Confort_SetSoundEnabled(1);
            Confort_SetVolumePercent(pct);
            UART1_WriteString("[OK][RADIO] VOL=");
            UART1_WriteDecimal(pct);
            UART1_WriteChar('\r');
            UART1_WriteChar('\n');
            UART_WriteString("[OK][RADIO] VOL=");
            UART_WriteDecimal(pct);
            UART_Newline();
            forward_radio(cmd_buf);
        } else {
            remote_reply("[ERR][RADIO] Volumen fuera de rango 0-100");
        }

    } else if (strcmp(cmd_buf, "RADIO STATUS") == 0) {
        radio_status();
        forward_radio(cmd_buf);

    } else if (strncmp(cmd_buf, "HORNO ON ", 9) == 0) {
        const char *rest = cmd_buf + 9;
        uint16_t temp = parse_uint8(rest);

        while (*rest >= '0' && *rest <= '9') rest++;
        while (*rest == ' ') rest++;

        uint16_t min = parse_uint8(rest);

        if (temp >= 10 && temp <= 300 && min >= 1 && min <= 180) {
            oven_start(temp, min);
            remote_reply("[OK][HORNO] ON");
            oven_status();
            forward_horno(cmd_buf);
        } else {
            remote_reply("[ERR][HORNO] Use: HORNO ON temp min, temp 10-300, min 1-180");
        }

    } else if (strcmp(cmd_buf, "HORNO OFF") == 0) {
        if (oven_running) {
            oven_stop();
        }
        remote_reply("[OK][HORNO] OFF");
        forward_horno(cmd_buf);

    } else if (strcmp(cmd_buf, "HORNO STATUS") == 0) {
        oven_status();
        forward_horno(cmd_buf);

    } else {
        remote_reply("[ERR] Comando desconocido. Escriba HELP");
    }
}

/* ---- Market EEPROM persistence ---- */

static void market_save(void) {
    if (market_count > MARKET_MAX_ITEMS) return;
    uint8_t buf[1 + MARKET_MAX_ITEMS * 2];
    buf[0] = market_count;
    for (uint8_t i = 0; i < market_count; i++) {
        buf[1 + i * 2] = market[i].product_id;
        buf[1 + i * 2 + 1] = market[i].quantity;
    }
    EEPROM_WriteBlock(EEPROM_MARKET_ADDR, buf, 1 + market_count * 2);
}

static void market_load(void) {
    uint8_t count = EEPROM_ReadByte(EEPROM_MARKET_ADDR);
    if (count > MARKET_MAX_ITEMS) {
        market_count = 0;
        market_save();
        return;
    }
    market_count = count;
    for (uint8_t i = 0; i < count; i++) {
        market[i].product_id = EEPROM_ReadByte(EEPROM_MARKET_ADDR + 1 + i * 2);
        market[i].quantity = EEPROM_ReadByte(EEPROM_MARKET_ADDR + 1 + i * 2 + 1);
    }
}

/* ---- Public API ---- */

void Remoto_Init(void) {
    UART1_Init(9600);
    UART2_Init(9600);
    UART3_Init(9600);
    oven_running = 0;
    cmd_len = 0;
    radio_rx_len = 0;
    oven_rx_len = 0;
    market_count = 0;
    market_load();
    UART_WriteEvent(SER_EEPROM, "Lista mercado cargada");
    GPIO_SetPinMode(PIN_OVEN_LED, GPIO_OUT);
    GPIO_WritePin(PIN_OVEN_LED, GPIO_LOW);
    UART1_WriteEvent(SER_SISTEMA, "Canal remoto listo. Escriba HELP");
    UART_WriteEvent(SER_SISTEMA, "Remoto iniciado por UART1. Slaves UART2(Radio) UART3(Horno) listos");
}

void Remoto_Task(uint32_t now_ms) {
    /* Read UART1 commands */
    while (UART1_Available()) {
        char c = UART1_ReadChar();
        if (c == '\n' || c == '\r') {
            if (cmd_len > 0) {
                process_command();
                cmd_len = 0;
            }
        } else {
            if (cmd_len < sizeof(cmd_buf) - 1) {
                cmd_buf[cmd_len++] = c;
            }
        }
    }

    /* Read UART2 (radio slave) responses */
    while (UART2_Available()) {
        char c = UART2_ReadChar();
        if (c == '\n' || c == '\r') {
            if (radio_rx_len > 0) {
                radio_rx_buf[radio_rx_len] = '\0';
                UART_WriteString("[RADIO->] ");
                UART_WriteString(radio_rx_buf);
                UART_Newline();
                UART1_WriteString("[RADIO->] ");
                UART1_WriteString(radio_rx_buf);
                UART1_WriteChar('\r');
                UART1_WriteChar('\n');
                radio_rx_len = 0;
            }
        } else {
            if (radio_rx_len < sizeof(radio_rx_buf) - 1) {
                radio_rx_buf[radio_rx_len++] = c;
            }
        }
    }

    /* Read UART3 (horno slave) responses */
    while (UART3_Available()) {
        char c = UART3_ReadChar();
        if (c == '\n' || c == '\r') {
            if (oven_rx_len > 0) {
                oven_rx_buf[oven_rx_len] = '\0';
                UART_WriteString("[HORNO->] ");
                UART_WriteString(oven_rx_buf);
                UART_Newline();
                UART1_WriteString("[HORNO->] ");
                UART1_WriteString(oven_rx_buf);
                UART1_WriteChar('\r');
                UART1_WriteChar('\n');
                oven_rx_len = 0;
            }
        } else {
            if (oven_rx_len < sizeof(oven_rx_buf) - 1) {
                oven_rx_buf[oven_rx_len++] = c;
            }
        }
    }

    /* Oven countdown */
    if (oven_running) {
        if (Timer_Expired(oven_last_tick, OVEN_MINUTE_MS)) {
            oven_last_tick = now_ms;
            if (oven_remaining_min > 0) {
                oven_remaining_min--;
            }
            if (oven_remaining_min == 0) {
                oven_stop();
                remote_reply("[OK][HORNO] Finalizado");
            } else {
                oven_status();
            }
        }
    }
}
