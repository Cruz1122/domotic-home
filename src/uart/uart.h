#ifndef UART_H
#define UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void UART_Init(uint32_t baud);
void UART_Task(void);
void UART_WriteString(const char *str);
void UART_WriteDecimal(uint32_t num);
void UART_WriteChar(char c);
void UART_Newline(void);
void UART_WriteEvent(const char *tag, const char *msg);

uint8_t UART_Available(void);
char    UART_ReadChar(void);

/* UART1 - celular (RX/TX) */
void UART1_Init(uint32_t baud);
void UART1_WriteChar(char c);
void UART1_WriteString(const char *str);
void UART1_WriteDecimal(uint32_t num);
void UART1_WriteEvent(const char *tag, const char *msg);
uint8_t UART1_Available(void);
char UART1_ReadChar(void);

/* UART2 - radio slave (TX only) */
void UART2_Init(uint32_t baud);
void UART2_WriteChar(char c);
void UART2_WriteString(const char *str);
void UART2_WriteDecimal(uint32_t num);
void UART2_WriteEvent(const char *tag, const char *msg);

/* UART3 - horno slave (TX only) */
void UART3_Init(uint32_t baud);
void UART3_WriteChar(char c);
void UART3_WriteString(const char *str);
void UART3_WriteDecimal(uint32_t num);
void UART3_WriteEvent(const char *tag, const char *msg);

#ifdef __cplusplus
}
#endif

#endif
