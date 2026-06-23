/*
 * Módulo: LCD (HD44780, 16x2, 4 bits)
 * Driver del display de usuario. Pines RS/E/D4-D7 en PORTA (pines 22-27).
 * RW a GND. Lo usa la UI para pintar menús y mensajes de estado.
 */
#ifndef LCD_H
#define LCD_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t col, uint8_t row);
void LCD_WriteChar(char c);
void LCD_WriteString(const char *str);
void LCD_WriteTwoLines(const char *line1, const char *line2);

#ifdef __cplusplus
}
#endif

#endif /* LCD_H */
