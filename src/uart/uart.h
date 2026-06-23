/*
 * Módulo: UART
 * Dos canales serie con buffers circulares:
 *   UART0 -> debug y eventos del sistema ([TAG] mensaje), atendido por UART_Task.
 *   UART1 -> comandos remotos desde el Virtual Terminal (RADIO/HORNO/MERCADO).
 * Ningún envío bloquea: UART_Task drena el buffer de transmisión de UART0.
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

/* UART1 - control remoto / Virtual Terminal */
void UART1_Init(uint32_t baud);
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

/* Reenvía lo recibido por UART0 al buffer RX de UART1 y lo encolado
   para TX en UART1 al buffer TX de UART0. Llamar antes de Remoto_Task. */
void UART_Bridge_Task(void);

#ifdef __cplusplus
}
#endif

#endif
