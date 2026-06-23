/*
 * Módulo: UART — implementación
 * Buffers circulares de TX/RX por interrupción para UART0 y UART1 (8N1).
 * TX: se encola y la ISR UDRE transmite cuando el registro queda libre.
 * RX: la ISR RX guarda el byte; el resto del sistema lee sin bloquear.
 */
#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define TX_BUF_SIZE 256
#define RX_BUF_SIZE 32

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
    UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    tx0_head = 0; tx0_tail = 0;
    rx0_head = 0; rx0_tail = 0;
}

void UART_Task(void) {}

/* Encola un char para TX. Si el buffer está lleno, se descarta (no bloquea). */
void UART_WriteChar(char c) {
    uint8_t next = (uint8_t)(tx0_head + 1) % TX_BUF_SIZE;
    if (next == tx0_tail) return;
    tx0_buffer[tx0_head] = (uint8_t)c;
    tx0_head = next;
    UCSR0B |= (1 << UDRIE0);
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

/* ISR RX de UART0: guarda el byte recibido si hay espacio en el buffer. */
ISR(USART0_RX_vect) {
    uint8_t data = UDR0;
    uint8_t next = (uint8_t)(rx0_head + 1) % RX_BUF_SIZE;
    if (next != rx0_tail) { rx0_buffer[rx0_head] = data; rx0_head = next; }
}

/* ISR de registro de TX vacío: envía el siguiente byte encolado o apaga la IRQ. */
ISR(USART0_UDRE_vect) {
    if (tx0_head != tx0_tail) {
        UDR0 = tx0_buffer[tx0_tail];
        tx0_tail = (uint8_t)(tx0_tail + 1) % TX_BUF_SIZE;
    } else { UCSR0B &= ~(1 << UDRIE0); }
}

/* ============================================================
 *  UART1 — comandos remotos (Virtual Terminal / celular)
 * ============================================================ */

void UART1_Init(uint32_t baud) {
    uint16_t ubrr = (uint16_t)(F_CPU / 16 / baud - 1);
    UBRR1H = (uint8_t)(ubrr >> 8);
    UBRR1L = (uint8_t)(ubrr);
    UCSR1B = (1 << TXEN1) | (1 << RXEN1) | (1 << RXCIE1);
    UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
    tx1_head = 0; tx1_tail = 0;
    rx1_head = 0; rx1_tail = 0;
}

void UART1_WriteChar(char c) {
    uint8_t next = (uint8_t)(tx1_head + 1) % TX_BUF_SIZE;
    if (next == tx1_tail) return;
    tx1_buffer[tx1_head] = (uint8_t)c;
    tx1_head = next;
    UCSR1B |= (1 << UDRIE1);
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

/* ISR RX de UART1: recibe los comandos remotos que escribe el usuario. */
ISR(USART1_RX_vect) {
    uint8_t data = UDR1;
    uint8_t next = (uint8_t)(rx1_head + 1) % RX_BUF_SIZE;
    if (next != rx1_tail) { rx1_buffer[rx1_head] = data; rx1_head = next; }
}

ISR(USART1_UDRE_vect) {
    if (tx1_head != tx1_tail) {
        UDR1 = tx1_buffer[tx1_tail];
        tx1_tail = (uint8_t)(tx1_tail + 1) % TX_BUF_SIZE;
    } else { UCSR1B &= ~(1 << UDRIE1); }
}
