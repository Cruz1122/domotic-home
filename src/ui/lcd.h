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
