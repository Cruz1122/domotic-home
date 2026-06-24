/*
 * Módulo: Remoto — implementación
 * Parser de líneas por UART1 (o UART0 vía bridge) que despacha a RADIO/HORNO/MERCADO.
 * Las respuestas al usuario van por UART1 con prefijo [OK]/[ERR]/[STATUS] y se replican en UART0.
 * Los eventos de debug internos van por UART0 con prefijo [DBG].
 */
#include "Remoto.h"
#include "../uart/uart.h"
#include "../confort/Confort.h"
#include "../gpio/gpio.h"
#include "../timer/timer.h"
#include "../eeprom/eeprom.h"
#include <string.h>

#define OVEN_TEMP_MIN_C      10U
#define OVEN_TEMP_MAX_C     300U
#define OVEN_TIME_MIN_MIN     1U
#define OVEN_TIME_MAX_MIN   180U
/* En modo demo, cada minuto lógico del horno dura 5 segundos reales. */
#define OVEN_DEMO_MINUTE_MS 5000UL

/* Estado del horno: encendido, temperatura objetivo y cuenta regresiva. */
static uint8_t  oven_running;
static uint16_t oven_target_temp;
static uint16_t oven_remaining_min;
static uint32_t oven_last_tick;

/* Buffer de la línea de comando entrante por UART1. */
static char   cmd_buf[REMOTE_LINE_MAX];
static uint8_t cmd_len;
static uint8_t cmd_overflow;

static void radio_status(void);
static void oven_status(void);
static void market_list(void);
static void market_products(void);
static void oven_start(uint16_t temp, uint16_t min);
static void oven_stop(void);
static void remote_write_u16(uint16_t value);
static void remote_ok_horno_on(uint16_t temp, uint16_t min);

static void uart0_write_prefix(const char *prefix, const char *module) {
    UART_WriteString(prefix);
    UART_WriteChar('[');
    UART_WriteString(module);
    UART_WriteString("] ");
}

static void remote_reply(const char *prefix, const char *module, const char *msg) {
    UART1_WriteString(prefix);
    UART1_WriteChar('[');
    UART1_WriteString(module);
    UART1_WriteString("] ");
    UART1_WriteString(msg);
    UART1_WriteChar('\r');
    UART1_WriteChar('\n');
}

static void remote_ok(const char *module, const char *msg) {
    remote_reply(REMOTE_OK, module, msg);
}

static void remote_err(const char *module, const char *msg) {
    remote_reply(REMOTE_ERR, module, msg);
}

static void remote_status_prefix(const char *module) {
    UART1_WriteString(REMOTE_STATUS);
    UART1_WriteChar('[');
    UART1_WriteString(module);
    UART1_WriteString("] ");
}

static void remote_status(const char *module, const char *msg) {
    remote_reply(REMOTE_STATUS, module, msg);
}

static void debug_event(const char *module, const char *msg) {
    uart0_write_prefix(REMOTE_DBG, module);
    UART_WriteString(msg);
    UART_Newline();
}

static void remote_write_u16(uint16_t value) {
    UART1_WriteDecimal(value);
}

static void remote_ok_horno_on(uint16_t temp, uint16_t min) {
    UART1_WriteString(REMOTE_OK "[" MOD_HORNO "] ON TEMP=");
    remote_write_u16(temp);
    UART1_WriteString(" MIN=");
    remote_write_u16(min);
    UART1_WriteString("\r\n");
}

/* ---- Helpers internos ---- */

static char ascii_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return (char)(c - ('a' - 'A'));
    }
    return c;
}

static uint8_t streq(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return (uint8_t)((*a == '\0') && (*b == '\0'));
}

static uint8_t split_tokens(char *line, char **tokens, uint8_t max_tokens) {
    uint8_t count = 0;
    char *p = line;

    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') {
            *p = '\0';
            p++;
        }

        if (*p == '\0') {
            break;
        }

        if (count >= max_tokens) {
            return count;
        }

        tokens[count++] = p;

        while (*p != '\0' && *p != ' ' && *p != '\t') {
            *p = ascii_upper(*p);
            p++;
        }
    }

    return count;
}

static uint8_t parse_u16(const char *s, uint16_t *out) {
    uint16_t value = 0;

    if (*s == '\0') {
        return 0;
    }

    while (*s != '\0') {
        if (*s < '0' || *s > '9') {
            return 0;
        }
        value = (uint16_t)(value * 10U + (uint16_t)(*s - '0'));
        s++;
    }

    *out = value;
    return 1;
}

/* HELP: lista los comandos disponibles por UART1. */
static void handle_help(void) {
    remote_status(MOD_SISTEMA, "Comandos:");
    remote_status(MOD_SISTEMA, "RADIO ON|OFF|VOL <0-100>|STATUS");
    remote_status(MOD_SISTEMA, "HORNO ON <temp> <min>|OFF|STATUS");
    remote_status(MOD_SISTEMA, "MERCADO PRODUCTS|ADD <id> <qty>|LIST|CLEAR|STATUS");
}

/* Comandos RADIO (sonido lógico) por UART1:
 *   RADIO ON     -> enciende el sonido (estado en LCD/UART).
 *   RADIO OFF    -> apaga el sonido (conserva el último volumen almacenado).
 *   RADIO STATUS -> reporta estado y volumen remoto actual.
 *   RADIO VOL n  -> ajusta el setpoint remoto (0-100) en LCD/UART.
 */
static void handle_radio(char **tok, uint8_t n) {
    if (n < 2) {
        remote_err(MOD_RADIO, "Falta accion");
        return;
    }

    if (streq(tok[1], "ON")) {
        if (n != 2) {
            remote_err(MOD_RADIO, "Argumentos invalidos");
            return;
        }
        Confort_SetSoundEnabled(1);
        remote_ok(MOD_RADIO, "ON");
        debug_event(MOD_RADIO, "ON");
        return;
    }

    if (streq(tok[1], "OFF")) {
        if (n != 2) {
            remote_err(MOD_RADIO, "Argumentos invalidos");
            return;
        }
        Confort_SetSoundEnabled(0);
        remote_ok(MOD_RADIO, "OFF");
        debug_event(MOD_RADIO, "OFF");
        return;
    }

    if (streq(tok[1], "STATUS")) {
        if (n != 2) {
            remote_err(MOD_RADIO, "Argumentos invalidos");
            return;
        }
        radio_status();
        return;
    }

    if (streq(tok[1], "VOL")) {
        uint16_t volume = 0;

        if (n != 3) {
            remote_err(MOD_RADIO, "Uso RADIO VOL <0-100>");
            return;
        }
        if (!parse_u16(tok[2], &volume)) {
            remote_err(MOD_RADIO, "Volumen invalido");
            return;
        }
        if (volume > 100U) {
            remote_err(MOD_RADIO, "Volumen fuera de rango");
            return;
        }

        Confort_SetVolumePercent((uint8_t)volume);
        UART1_WriteString(REMOTE_OK "[" MOD_RADIO "] VOL=");
        UART1_WriteDecimal(volume);
        UART1_WriteString("\r\n");
        uart0_write_prefix(REMOTE_DBG, MOD_RADIO);
        UART_WriteString("VOL=");
        UART_WriteDecimal(volume);
        UART_Newline();
        return;
    }

    remote_err(MOD_RADIO, "Accion desconocida");
}

/* Comandos HORNO por UART1:
 *   HORNO ON temp min -> enciende el horno (LED) con temp 10-300 y min 1-180.
 *   HORNO OFF         -> apaga el horno a mano.
 *   HORNO STATUS      -> reporta estado, temperatura objetivo y tiempo restante.
 * Validaciones: rangos de temperatura/tiempo y parámetros numéricos.
 */
static void handle_horno(char **tok, uint8_t n) {
    uint16_t temp = 0;
    uint16_t min = 0;

    if (n < 2) {
        remote_err(MOD_HORNO, "Falta accion");
        return;
    }

    if (streq(tok[1], "ON")) {
        if (n != 4) {
            remote_err(MOD_HORNO, "Uso HORNO ON <temp> <min>");
            return;
        }
        if (!parse_u16(tok[2], &temp) || !parse_u16(tok[3], &min)) {
            remote_err(MOD_HORNO, "Parametros invalidos");
            return;
        }
        if (temp < OVEN_TEMP_MIN_C || temp > OVEN_TEMP_MAX_C) {
            remote_err(MOD_HORNO, "Temp fuera de rango");
            return;
        }
        if (min < OVEN_TIME_MIN_MIN || min > OVEN_TIME_MAX_MIN) {
            remote_err(MOD_HORNO, "Tiempo fuera de rango");
            return;
        }
        oven_start(temp, min);
        remote_ok_horno_on(temp, min);
        uart0_write_prefix(REMOTE_DBG, MOD_HORNO);
        UART_WriteString("ON TEMP=");
        UART_WriteDecimal(temp);
        UART_WriteString(" MIN=");
        UART_WriteDecimal(min);
        UART_Newline();
        return;
    }

    if (streq(tok[1], "OFF")) {
        if (n != 2) {
            remote_err(MOD_HORNO, "Argumentos invalidos");
            return;
        }
        if (oven_running) {
            oven_stop();
        }
        remote_ok(MOD_HORNO, "OFF");
        debug_event(MOD_HORNO, "OFF");
        return;
    }

    if (streq(tok[1], "STATUS")) {
        if (n != 2) {
            remote_err(MOD_HORNO, "Argumentos invalidos");
            return;
        }
        oven_status();
        return;
    }

    remote_err(MOD_HORNO, "Accion desconocida");
}

/* Comandos MERCADO por UART1:
 *   MERCADO PRODUCTS      -> lista los 8 productos válidos con su ID.
 *   MERCADO ADD id qty    -> agrega/acumula un producto (id 1-8, qty 1-99).
 *   MERCADO LIST|STATUS   -> muestra la lista actual.
 *   MERCADO CLEAR         -> vacía la lista y guarda en EEPROM.
 */
static void handle_market(char **tok, uint8_t n) {
    uint16_t product_id = 0;
    uint16_t quantity = 0;
    market_item_t item;

    if (n < 2) {
        remote_err(MOD_MERCADO, "Falta accion");
        return;
    }

    if (streq(tok[1], "PRODUCTS")) {
        if (n != 2) {
            remote_err(MOD_MERCADO, "Argumentos invalidos");
            return;
        }
        market_products();
        return;
    }

    if (streq(tok[1], "ADD")) {
        if (n != 4) {
            remote_err(MOD_MERCADO, "Uso MERCADO ADD <id> <qty>");
            return;
        }
        if (!parse_u16(tok[2], &product_id) || product_id < 1U || product_id > MARKET_PRODUCT_COUNT) {
            remote_err(MOD_MERCADO, "Producto invalido");
            return;
        }
        if (!parse_u16(tok[3], &quantity) || quantity < 1U || quantity > 99U) {
            remote_err(MOD_MERCADO, "Cantidad invalida");
            return;
        }
        if (!Remoto_MarketAdd((uint8_t)product_id, (uint8_t)quantity)) {
            remote_err(MOD_MERCADO, "No se pudo agregar");
            return;
        }

        for (uint8_t i = 0; i < Remoto_MarketGetCount(); i++) {
            if (Remoto_MarketGetItem(i, &item) && item.product_id == (uint8_t)product_id) {
                UART1_WriteString(REMOTE_OK "[" MOD_MERCADO "] ");
                UART1_WriteString(Remoto_MarketGetProductName(item.product_id));
                UART1_WriteString(" x");
                UART1_WriteDecimal(item.quantity);
                UART1_WriteString("\r\n");
                return;
            }
        }

        remote_ok(MOD_MERCADO, "Item agregado");
        return;
    }

    if (streq(tok[1], "LIST") || streq(tok[1], "STATUS")) {
        if (n != 2) {
            remote_err(MOD_MERCADO, "Argumentos invalidos");
            return;
        }
        market_list();
        return;
    }

    if (streq(tok[1], "CLEAR")) {
        if (n != 2) {
            remote_err(MOD_MERCADO, "Argumentos invalidos");
            return;
        }
        Remoto_MarketClear();
        remote_ok(MOD_MERCADO, "Lista limpia");
        return;
    }

    remote_err(MOD_MERCADO, "Accion desconocida");
}

/* Estado del sonido por UART1: ON/OFF y volumen remoto actual. */
static void radio_status(void) {
    remote_status_prefix(MOD_RADIO);
    if (Confort_IsSoundEnabled()) {
        UART1_WriteString("ON");
    } else {
        UART1_WriteString("OFF");
    }
    UART1_WriteString(" VOL=");

    UART1_WriteDecimal(Confort_GetVolumePercent());
    UART1_WriteChar('\r');
    UART1_WriteChar('\n');
}

/* Estado del horno por UART1: ON/OFF, temperatura objetivo y minutos restantes. */
static void oven_status(void) {
    uint16_t t, m;

    remote_status_prefix(MOD_HORNO);

    if (oven_running) {
        UART1_WriteString("ON");
    } else {
        UART1_WriteString("OFF");
    }
    UART1_WriteString(" TEMP=");

    t = oven_target_temp;
    if (t >= 100) { UART1_WriteChar((char)('0' + t / 100)); t = (uint16_t)(t % 100); }
    if (oven_target_temp >= 10) { UART1_WriteChar((char)('0' + t / 10)); t = (uint16_t)(t % 10); }
    UART1_WriteChar((char)('0' + t));

    UART1_WriteString(" RESTANTE=");

    m = oven_remaining_min;
    if (m >= 100) { UART1_WriteChar((char)('0' + m / 100)); m = (uint16_t)(m % 100); }
    if (oven_remaining_min >= 10) { UART1_WriteChar((char)('0' + m / 10)); m = (uint16_t)(m % 10); }
    UART1_WriteChar((char)('0' + m));

    UART1_WriteString("min\r\n");
}

/* ---- Market list ---- */

static void market_save(void);
static void market_load(void);

/* Consulta remota de la lista de mercado por UART1. */
static void market_list(void) {
    uint8_t count = Remoto_MarketGetCount();

    remote_status_prefix(MOD_MERCADO);
    UART1_WriteDecimal(count);
    UART1_WriteString(" items\r\n");

    if (count == 0) {
        remote_status(MOD_MERCADO, "Lista vacia");
        return;
    }

    for (uint8_t i = 0; i < count; i++) {
        market_item_t item;
        const char *name;
        if (!Remoto_MarketGetItem(i, &item)) continue;
        name = Remoto_MarketGetProductName(item.product_id);
        if (name == 0) name = "?";

        remote_status_prefix(MOD_MERCADO);
        UART1_WriteDecimal((uint32_t)(i + 1));
        UART1_WriteString(") ");
        UART1_WriteString(name);
        UART1_WriteString(" x");
        UART1_WriteDecimal(item.quantity);
        UART1_WriteChar('\r');
        UART1_WriteChar('\n');
    }
}

static void market_products(void) {
    remote_status(MOD_MERCADO, "Productos:");

    for (uint8_t id = 1; id <= MARKET_PRODUCT_COUNT; id++) {
        const char *name = Remoto_MarketGetProductName(id);
        if (name == 0) {
            name = "?";
        }
        remote_status_prefix(MOD_MERCADO);
        UART1_WriteDecimal(id);
        UART1_WriteChar(' ');
        UART1_WriteString(name);
        UART1_WriteString("\r\n");
    }
}

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

/* Arranca el horno: activa el LED y guarda temp/min y el tick de inicio. */
static void oven_start(uint16_t temp, uint16_t min) {
    oven_running = 1;
    oven_target_temp = temp;
    oven_remaining_min = min;
    oven_last_tick = Timer_GetMs();
    GPIO_WritePin(PIN_OVEN_LED, GPIO_HIGH);
}

/* Apaga el horno y el LED. */
static void oven_stop(void) {
    oven_running = 0;
    GPIO_WritePin(PIN_OVEN_LED, GPIO_LOW);
}

/* ---- Parser de comandos ---- */

static uint8_t trim_line(char *line, uint8_t len) {
    uint8_t start = 0;

    while (start < len && (line[start] == ' ' || line[start] == '\t')) {
        start++;
    }
    while (len > start && (line[len - 1U] == ' ' || line[len - 1U] == '\t')) {
        len--;
    }
    if (start > 0U && len > start) {
        memmove(line, line + start, (size_t)(len - start));
    }
    line[len - start] = '\0';
    return (uint8_t)(len - start);
}

/* Divide la línea en tokens (en mayúsculas) y despacha al módulo correspondiente. */
static void process_command(void) {
    char *tokens[REMOTE_MAX_TOKENS];
    uint8_t count;
    char dbg_buf[REMOTE_LINE_MAX];

    cmd_buf[cmd_len] = '\0';
    cmd_len = trim_line(cmd_buf, cmd_len);
    if (cmd_len == 0U) {
        return;
    }

    /* Copia para diagnostico antes de que split_tokens modifique cmd_buf. */
    for (count = 0; count < cmd_len && count < (uint8_t)(REMOTE_LINE_MAX - 1); count++) {
        dbg_buf[count] = cmd_buf[count];
    }
    dbg_buf[count] = '\0';

    count = split_tokens(cmd_buf, tokens, REMOTE_MAX_TOKENS);

    if (count == 0) {
        return;
    }

    if (streq(tokens[0], "HELP")) {
        handle_help();
        return;
    }

    if (streq(tokens[0], "RADIO")) {
        handle_radio(tokens, count);
        return;
    }

    if (streq(tokens[0], "HORNO")) {
        handle_horno(tokens, count);
        return;
    }

    if (streq(tokens[0], "MERCADO")) {
        handle_market(tokens, count);
        return;
    }

    /* Error + diagnostico: muestra los primeros hasta REMOTE_LINE_MAX caracteres
       tal cual llegaron (sin mayusculas ni separacion). */
    remote_err(MOD_SISTEMA, "Comando desconocido.");
    UART1_WriteString(SER_SISTEMA "Recibido: \"");
    UART1_WriteString(dbg_buf);
    UART1_WriteString("\" ");
    UART1_WriteDecimal(cmd_len);
    UART1_WriteString(" bytes\r\n");
}

/* ---- Persistencia de la lista de mercado en EEPROM (desde 0x100) ---- */

/* Guarda la lista: 1 byte de count + pares (product_id, quantity). */
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

/* Carga la lista desde EEPROM; si el count es inválido, la inicializa vacía. */
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
    UART1_Init(UART_BAUD_DEFAULT);
    oven_running = 0;
    cmd_len = 0;
    cmd_overflow = 0;
    market_count = 0;
    market_load();
    UART_WriteEvent(SER_EEPROM, "Lista mercado cargada");
    GPIO_SetPinMode(PIN_OVEN_LED, GPIO_OUT);
    GPIO_WritePin(PIN_OVEN_LED, GPIO_LOW);
    remote_ok(MOD_SISTEMA, "Canal remoto listo");
    debug_event(MOD_SISTEMA, "Remoto iniciado por UART1");
}

void Remoto_Task(uint32_t now_ms) {
    /* Lee comandos de UART1 carácter a carácter; procesa la línea al recibir CR/LF. */
    while (UART1_Available()) {
        char c = UART1_ReadChar();
        if (c == '\n' || c == '\r') {
            if (cmd_overflow) {
                cmd_len = 0;
                cmd_overflow = 0;
                remote_err(MOD_SISTEMA, "Linea demasiado larga");
            } else if (cmd_len > 0) {
                process_command();
                cmd_len = 0;
            }
        } else if (c == '\b' || c == 0x7F) {
            if (cmd_len > 0U) {
                cmd_len--;
            }
        } else if (c == '\t' || (c >= 0x20 && c < 0x7F)) {
            if (cmd_len < sizeof(cmd_buf) - 1U) {
                cmd_buf[cmd_len++] = c;
            } else {
                cmd_overflow = 1;
            }
        }
    }

    /* Cuenta regresiva del horno (no bloqueante). Al llegar a 0 se apaga solo. */
    if (oven_running) {
        if (Timer_Expired(oven_last_tick, OVEN_DEMO_MINUTE_MS)) {
            oven_last_tick = now_ms;
            if (oven_remaining_min > 0) {
                oven_remaining_min--;
            }
            if (oven_remaining_min == 0) {
                oven_stop();
                remote_ok(MOD_HORNO, "Finalizado");
            } else {
                oven_status();
            }
        }
    }
}
