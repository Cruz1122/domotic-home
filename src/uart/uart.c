#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define TX_BUF_SIZE 128

static volatile uint8_t tx_buffer[TX_BUF_SIZE];
static volatile uint8_t tx_head;
static volatile uint8_t tx_tail;

void UART_Init(uint32_t baud) {
    uint16_t ubrr = (uint16_t)(F_CPU / 16 / baud - 1);
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr);
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    sei();
    tx_head = 0;
    tx_tail = 0;
}

void UART_Task(void) {
}

void UART_WriteChar(char c) {
    uint8_t next = (uint8_t)(tx_head + 1) % TX_BUF_SIZE;
    if (next == tx_tail) return;
    tx_buffer[tx_head] = (uint8_t)c;
    tx_head = next;
    UCSR0B |= (1 << UDRIE0);
}

void UART_WriteString(const char *str) {
    while (*str) {
        UART_WriteChar(*str++);
    }
}

void UART_Newline(void) {
    UART_WriteChar('\r');
    UART_WriteChar('\n');
}

void UART_WriteDecimal(uint32_t num) {
    char buf[10];
    uint8_t i = 0;
    if (num == 0) {
        UART_WriteChar('0');
        return;
    }
    while (num > 0) {
        buf[i++] = (char)('0' + (num % 10));
        num /= 10;
    }
    while (i > 0) {
        UART_WriteChar(buf[--i]);
    }
}

void UART_WriteEvent(const char *tag, const char *msg) {
    UART_WriteString(tag);
    UART_WriteString(msg);
    UART_Newline();
}

ISR(USART0_UDRE_vect) {
    if (tx_head != tx_tail) {
        UDR0 = tx_buffer[tx_tail];
        tx_tail = (uint8_t)(tx_tail + 1) % TX_BUF_SIZE;
    } else {
        UCSR0B &= ~(1 << UDRIE0);
    }
}
