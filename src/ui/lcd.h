#ifndef LCD_H
#define LCD_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_write_char(char c);
void lcd_write_string(const char *str);
void lcd_write_two_lines(const char *line1, const char *line2);

#ifdef __cplusplus
}
#endif

#endif /* LCD_H */
