/*
 * Módulo: UART
 * Dos canales serie con buffers circulares (sondeo, sin ISRs propias):
 *   UART0 -> debug, eventos del sistema y entrada del Monitor Serie (bridge RX).
 *   UART1 -> comandos remotos (Virtual Terminal) y respuestas [OK]/[ERR]/[STATUS].
 * UART1_Task replica las respuestas remotas en UART0.
 */
#ifndef UART_H
#define UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UART0 - debug y eventos del sistema */
void UART_Init(uint32_t baud);
void UART_Task(void);
void UART_WriteString(const char *str);
void UART_WriteDecimal(uint32_t num);
void UART_WriteChar(char c);
void UART_Newline(void);
void UART_WriteEvent(const char *tag, const char *msg);

uint8_t UART_Available(void);
char    UART_ReadChar(void);

/* UART1 - control remoto / Virtual Terminal (hardware + bridge desde UART0) */
void UART1_Init(uint32_t baud);
void UART1_Task(void);
void UART1_WriteChar(char c);
void UART1_WriteString(const char *str);
void UART1_WriteDecimal(uint32_t num);
void UART1_WriteEvent(const char *tag, const char *msg);
uint8_t UART1_Available(void);
char UART1_ReadChar(void);

/* Helpers de buffer para el bridge UART0<->UART1.
   Permiten simular bytes entrantes en UART1 sin pasar por la línea física
   y leer los bytes que el Remoto encola para TX sin enviarlos al pin. */
void UART1_InjectRxChar(char c);
uint8_t UART1_TxAvailable(void);
char UART1_ReadTxChar(void);

/* Reenvía RX de UART0 al buffer RX de UART1 (comandos desde Monitor Serie).
 * Llamar después de UART_Task y antes de Remoto_Task. TX UART1->UART0 lo hace UART1_Task. */
void UART_Bridge_Task(void);

#ifdef __cplusplus
}
#endif

#endif
