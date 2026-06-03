#include "UI.h"
#include "lcd.h"

void ui_init(void) {
    lcd_init();
    lcd_write_two_lines("A Seg B RFID",
                        "C Amb D Serv");
}

void ui_task(void) {
    /* Keypad + menu navigation in future issue */
    /* LCD driver does not require periodic refresh */
}
