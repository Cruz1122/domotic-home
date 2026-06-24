/*
 * Módulo: UART — implementación
 * Buffers circulares de TX/RX para UART0 y UART1 (8N1).
 * UART0 se atiende por sondeo en UART_Task (sin ISRs: compatible con core Arduino).
 * UART1 usa colas en RAM; el bridge inyecta RX y drena TX hacia UART0.
 */
#include "uart.h"
#include <avr/io.h>

#define TX_BUF_SIZE 512
#define RX_BUF_SIZE 128

/* UART0 (debug/eventos): buffers de transmisión y recepción. */
static volatile uint8_t tx0_buffer[TX_BUF_SIZE];
static volatile uint8_t tx0_head, tx0_tail;
static volatile uint8_t rx0_buffer[RX_BUF_SIZE];
static volatile uint8_t rx0_head, rx0_tail;

/* UART1 (comandos remotos): buffers de transmisión y recepción. */
static volatile uint8_t tx1_buffer[TX_BUF_SIZE];
static volatile uint8_t tx1_head, tx1_tail;
static volatile uint8_t rx1_buffer[RX_BUF_SIZE];
static volatile uint8_t rx1_head, rx1_tail;

/* ============================================================
 *  UART0 — debug y eventos del sistema
 * ============================================================ */

void UART_Init(uint32_t baud) {
    uint16_t ubrr = (uint16_t)(F_CPU / 16 / baud - 1);
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr);
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    tx0_head = 0; tx0_tail = 0;
    rx0_head = 0; rx0_tail = 0;
}

/* Sondeo no bloqueante de UART0 (evita ISRs que chocan con HardwareSerial del core). */
void UART_Task(void) {
    while (UCSR0A & (1 << RXC0)) {
        uint8_t data = UDR0;
        uint8_t next = (uint8_t)(rx0_head + 1) % RX_BUF_SIZE;
        if (next != rx0_tail) {
            rx0_buffer[rx0_head] = data;
            rx0_head = next;
        }
    }
    while ((tx0_head != tx0_tail) && (UCSR0A & (1 << UDRE0))) {
        UDR0 = tx0_buffer[tx0_tail];
        tx0_tail = (uint8_t)(tx0_tail + 1) % TX_BUF_SIZE;
    }
}

/* Encola un char para TX. Si el buffer está lleno, se descarta (no bloquea). */
void UART_WriteChar(char c) {
    uint8_t next = (uint8_t)(tx0_head + 1) % TX_BUF_SIZE;
    if (next == tx0_tail) return;
    tx0_buffer[tx0_head] = (uint8_t)c;
    tx0_head = next;
}

void UART_WriteString(const char *str) {
    while (*str) { UART_WriteChar(*str++); }
}

void UART_Newline(void) {
    UART_WriteChar('\r');
    UART_WriteChar('\n');
}

void UART_WriteDecimal(uint32_t num) {
    char buf[10]; uint8_t i = 0;
    if (num == 0) { UART_WriteChar('0'); return; }
    while (num > 0) { buf[i++] = (char)('0' + (num % 10)); num /= 10; }
    while (i > 0) { UART_WriteChar(buf[--i]); }
}

/* Escribe un evento con formato "[TAG]mensaje". */
void UART_WriteEvent(const char *tag, const char *msg) {
    UART_WriteString(tag); UART_WriteString(msg); UART_Newline();
}

uint8_t UART_Available(void) {
    uint8_t h = rx0_head; uint8_t t = rx0_tail;
    if (h >= t) return h - t;
    return (RX_BUF_SIZE - t) + h;
}

char UART_ReadChar(void) {
    if (rx0_head == rx0_tail) return '\0';
    char c = (char)rx0_buffer[rx0_tail];
    rx0_tail = (uint8_t)(rx0_tail + 1) % RX_BUF_SIZE;
    return c;
}

/* ============================================================
 *  UART1 — comandos remotos (Virtual Terminal / celular)
 * ============================================================ */

void UART1_Init(uint32_t baud) {
    uint16_t ubrr = (uint16_t)(F_CPU / 16 / baud - 1);
    UBRR1H = (uint8_t)(ubrr >> 8);
    UBRR1L = (uint8_t)(ubrr);
    UCSR1B = (1 << TXEN1) | (1 << RXEN1);
    UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
    tx1_head = 0; tx1_tail = 0;
    rx1_head = 0; rx1_tail = 0;
}

/* Sondeo no bloqueante de UART1: RX del Virtual Terminal y TX hacia pin 18 + copia a UART0. */
void UART1_Task(void) {
    while (UCSR1A & (1 << RXC1)) {
        uint8_t data = UDR1;
        uint8_t next = (uint8_t)(rx1_head + 1) % RX_BUF_SIZE;
        if (next != rx1_tail) {
            rx1_buffer[rx1_head] = data;
            rx1_head = next;
        }
    }
    while ((tx1_head != tx1_tail) && (UCSR1A & (1 << UDRE1))) {
        char c = (char)tx1_buffer[tx1_tail];
        UDR1 = (uint8_t)c;
        UART_WriteChar(c);
        tx1_tail = (uint8_t)(tx1_tail + 1) % TX_BUF_SIZE;
    }
}

void UART1_WriteChar(char c) {
    uint8_t next = (uint8_t)(tx1_head + 1) % TX_BUF_SIZE;
    if (next == tx1_tail) return;
    tx1_buffer[tx1_head] = (uint8_t)c;
    tx1_head = next;
}

void UART1_WriteString(const char *str) { while (*str) { UART1_WriteChar(*str++); } }

void UART1_WriteDecimal(uint32_t num) {
    char buf[10]; uint8_t i = 0;
    if (num == 0) { UART1_WriteChar('0'); return; }
    while (num > 0) { buf[i++] = (char)('0' + (num % 10)); num /= 10; }
    while (i > 0) { UART1_WriteChar(buf[--i]); }
}

void UART1_WriteEvent(const char *tag, const char *msg) {
    UART1_WriteString(tag); UART1_WriteString(msg); UART1_WriteChar('\r'); UART1_WriteChar('\n');
}

uint8_t UART1_Available(void) {
    uint8_t h = rx1_head; uint8_t t = rx1_tail;
    if (h >= t) return h - t;
    return (RX_BUF_SIZE - t) + h;
}

char UART1_ReadChar(void) {
    if (rx1_head == rx1_tail) return '\0';
    char c = (char)rx1_buffer[rx1_tail];
    rx1_tail = (uint8_t)(rx1_tail + 1) % RX_BUF_SIZE;
    return c;
}

void UART1_InjectRxChar(char c) {
    uint8_t next = (uint8_t)(rx1_head + 1) % RX_BUF_SIZE;
    if (next != rx1_tail) {
        rx1_buffer[rx1_head] = (uint8_t)c;
        rx1_head = next;
    }
}

uint8_t UART1_TxAvailable(void) {
    uint8_t h = tx1_head;
    uint8_t t = tx1_tail;
    if (h >= t) return (uint8_t)(h - t);
    return (uint8_t)((TX_BUF_SIZE - t) + h);
}

char UART1_ReadTxChar(void) {
    if (tx1_head == tx1_tail) return '\0';
    char c = (char)tx1_buffer[tx1_tail];
    tx1_tail = (uint8_t)(tx1_tail + 1) % TX_BUF_SIZE;
    return c;
}

void UART_Bridge_Task(void) {
    /* Sentido 1: lo tecleado en Serial Monitor (UART0 RX) -> UART1 RX.
       Solo se inyectan chars imprimibles y CR/LF. Sin eco local:
       el IDE ya muestra lo que se teclea, y el eco dobla el trafico TX
       compitiendo con los eventos del sistema. */
    while (UART_Available()) {
        char c = UART_ReadChar();
        if (c == '\r' || c == '\n' || c == '\t' || c == '\b' || c == 0x7F
            || (c >= 0x20 && c < 0x7F)) {
            UART1_InjectRxChar(c);
        }
    }
    /* Sentido 2 (TX): UART1_Task drena el buffer TX hacia el pin fisico y UART0. */
}
