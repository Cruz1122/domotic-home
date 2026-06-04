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

#ifdef __cplusplus
}
#endif

#endif
