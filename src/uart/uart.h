#ifndef UART_H
#define UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UART0 - debug del sistema */
void UART_Init(uint32_t baud);
void UART_Task(void);
void UART_WriteString(const char *str);
void UART_WriteDecimal(uint32_t num);
void UART_WriteChar(char c);
void UART_Newline(void);
void UART_WriteEvent(const char *tag, const char *msg);

uint8_t UART_Available(void);
char    UART_ReadChar(void);

/* UART1 - control remoto / Virtual Terminal */
void UART1_Init(uint32_t baud);
void UART1_WriteChar(char c);
void UART1_WriteString(const char *str);
void UART1_WriteDecimal(uint32_t num);
void UART1_WriteEvent(const char *tag, const char *msg);
uint8_t UART1_Available(void);
char UART1_ReadChar(void);

/* UART2 - puerto hardware libre / uso futuro */
void UART2_Init(uint32_t baud);
void UART2_WriteChar(char c);
void UART2_WriteString(const char *str);
void UART2_WriteDecimal(uint32_t num);
void UART2_WriteEvent(const char *tag, const char *msg);
uint8_t UART2_Available(void);
char    UART2_ReadChar(void);

/* UART3 - puerto hardware libre / uso futuro */
void UART3_Init(uint32_t baud);
void UART3_WriteChar(char c);
void UART3_WriteString(const char *str);
void UART3_WriteDecimal(uint32_t num);
void UART3_WriteEvent(const char *tag, const char *msg);
uint8_t UART3_Available(void);
char    UART3_ReadChar(void);

#ifdef __cplusplus
}
#endif

#endif
