#include "UI.h"
#include "lcd.h"
#include "../keypad/keypad.h"

#include <string.h>

#define LCD_COLS 16

static char lcd_line1[LCD_COLS + 1];
static char lcd_line2[LCD_COLS + 1];
static uint8_t lcd_dirty;
static char last_key;
static uint8_t last_key_pending;

static void ui_refresh(void) {
    if (!lcd_dirty) {
        return;
    }

    lcd_write_two_lines(lcd_line1, lcd_line2);
    lcd_dirty = 0;
}

static void ui_handle_key(char key) {
    size_t len;

    if (key == '*') {
        lcd_line1[0] = '\0';
        lcd_line2[0] = '\0';
        lcd_dirty = 1;
        return;
    }

    len = strlen(lcd_line2);
    if (len < LCD_COLS) {
        lcd_line2[len] = key;
        lcd_line2[len + 1] = '\0';
        lcd_dirty = 1;
        return;
    }

    len = strlen(lcd_line1);
    if (len < LCD_COLS) {
        lcd_line1[len] = key;
        lcd_line1[len + 1] = '\0';
        lcd_dirty = 1;
    }
}

void ui_init(void) {
    keypad_init();
    lcd_init();

    lcd_line1[0] = '\0';
    lcd_line2[0] = '_';
    lcd_line2[1] = '\0';
    lcd_dirty = 1;
    ui_refresh();
}

void ui_task(uint32_t now_ms) {
    char key;

    keypad_scan(now_ms);

    key = keypad_get_key();
    if (key != '\0') {
        last_key         = key;
        last_key_pending = 1;
        ui_handle_key(key);
    }

    ui_refresh();
}

uint8_t ui_last_key(char *key) {
    if (!last_key_pending || key == 0) {
        return 0;
    }

    *key = last_key;
    last_key_pending = 0;
    return 1;
}
