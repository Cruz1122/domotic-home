/*
 * Módulo: Teclado matricial 4x4
 * Lectura no bloqueante por rotación de filas + interrupción PCINT2 en columnas.
 * Pines: filas 34-37 (PORTC), columnas 62-65 (PORTK). Devuelve una tecla por pulsación.
 */
#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Keypad_Init(void);
void Keypad_Scan(uint32_t now_ms);
char Keypad_GetKey(void);

#ifdef __cplusplus
}
#endif

#endif
