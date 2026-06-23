#include "UI.h"
#include "lcd.h"
#include "../keypad/keypad.h"
#include "../seguridad/Seguridad.h"
#include "../confort/Confort.h"
#include "../accesos/Accesos.h"
#include "../remoto/Remoto.h"
#include "../uart/uart.h"
#include "../timer/timer.h"
#include <string.h>

#define LCD_COLS 16

static char lcd_line1[LCD_COLS + 1];
static char lcd_line2[LCD_COLS + 1];
static uint8_t lcd_dirty;
static uint32_t refresh_tick;

static char last_key;
static uint8_t last_key_pending;

/* ---- Screen state ---- */

typedef enum {
    UI_HOME = 0,
    UI_SECURITY,
    UI_SEC_CODE_INPUT,
    UI_RFID,
    UI_ENV,
    UI_SERVICES,
    UI_TEMP_INPUT,
    UI_SOUND,
    UI_RFID_RECHARGE_INPUT,
    UI_MARKET,
    UI_MARKET_ADD_PROD,
    UI_MARKET_ADD_QTY,
    UI_MARKET_VIEW
} ui_screen_t;

static ui_screen_t screen;
static ui_screen_t previous_screen;

/* ---- Numeric input buffer ---- */

static char input_buf[4];
static uint8_t input_len;

static void input_clear(void) {
    input_len = 0;
    input_buf[0] = '\0';
}

static void input_push(char key) {
    if (input_len < 3) {
        input_buf[input_len++] = key;
        input_buf[input_len] = '\0';
    }
}

static uint16_t input_to_uint(void) {
    uint16_t val = 0;
    for (uint8_t i = 0; i < input_len; i++) {
        val = val * 10 + (uint16_t)(input_buf[i] - '0');
    }
    return val;
}

/* ---- RFID sub-state ---- */

typedef enum {
    RFID_MENU = 0,
    RFID_ENROLL_CHOOSE
} rfid_menu_state_t;

static rfid_menu_state_t rfid_menu_state;
static uint8_t  rfid_waiting;
static uint8_t  rfid_showing_result;
static uint32_t rfid_result_tick;

/* ---- Security code state ---- */
static uint8_t  sec_code_target;
static uint8_t  sec_code_error;

/* ---- Market state ---- */
static uint8_t market_add_product;
static uint8_t market_view_index;

/* ---- LCD buffer helpers ---- */

static void ui_set_lcd(const char *line1, const char *line2) {
    strncpy(lcd_line1, line1, LCD_COLS);
    lcd_line1[LCD_COLS] = '\0';
    strncpy(lcd_line2, line2, LCD_COLS);
    lcd_line2[LCD_COLS] = '\0';
    lcd_dirty = 1;
}

static void ui_refresh(void) {
    if (!lcd_dirty) return;
    LCD_WriteTwoLines(lcd_line1, lcd_line2);
    lcd_dirty = 0;
}

static void u8_to_str(char *buf, uint8_t val) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return; }
    char tmp[4];
    uint8_t i = 0;
    while (val > 0) {
        tmp[i++] = (char)('0' + (val % 10));
        val /= 10;
    }
    uint8_t j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

/* ---- Render by screen state ---- */

static void ui_render(void) {
    if (screen == UI_RFID && rfid_showing_result) return;

    switch (screen) {
        case UI_HOME:
            ui_set_lcd("A Seg B RFID", "C Amb D Serv");
            break;

        case UI_SECURITY: {
            char l1[LCD_COLS + 1];
            strcpy(l1, "1Acc ");
            strcat(l1, Seguridad_GetAccessState() ? "ON" : "OFF");
            strcat(l1, " 2Fue");
            char l2[LCD_COLS + 1];
            strcpy(l2, Seguridad_GetFireState() ? "ON" : "OFF");
            strcat(l2, " *Volver");
            ui_set_lcd(l1, l2);
            break;
        }

        case UI_SEC_CODE_INPUT:
            if (sec_code_error) {
                ui_set_lcd("Codigo invalido", "*Volver");
            } else {
                lcd_line1[0] = '\0';
                strcpy(lcd_line1, "Ingrese codigo:");
                strcpy(lcd_line2, input_buf);
                strcat(lcd_line2, " #OK *Vol");
                lcd_dirty = 1;
            }
            break;

        case UI_RFID:
            if (rfid_waiting) {
                switch (Accesos_GetMode()) {
                    case ACCESS_MODE_RECHARGE_CHILD:
                        ui_set_lcd("Acerque PADRE", "* Cancelar");
                        break;
                    case ACCESS_MODE_ENROLL_PARENT:
                        ui_set_lcd("Registrar PADRE", "Acerque tarjeta");
                        break;
                    case ACCESS_MODE_ENROLL_CHILD:
                        ui_set_lcd("Registrar HIJO", "Acerque tarjeta");
                        break;
                    case ACCESS_MODE_DELETE_USER:
                        ui_set_lcd("Borrar tarjeta", "Acerque tarjeta");
                        break;
                    default:
                        ui_set_lcd("Acerque tarjeta", "* Cancelar");
                        break;
                }
            } else {
                switch (rfid_menu_state) {
                    case RFID_MENU:
                        ui_set_lcd("1Pta 2Gar 3Jue",
                                   "4Enr 5Bor 6Rec *V");
                        break;
                    case RFID_ENROLL_CHOOSE:
                        ui_set_lcd("Enrolar:",
                                   "1Padre 2Hijo *V");
                        break;
                }
            }
            break;

        case UI_ENV: {
            char l2[LCD_COLS + 1] = "T:";
            char tmp[4];
            u8_to_str(tmp, (uint8_t)Confort_GetCurrentTemp());
            strcat(l2, tmp);
            strcat(l2, "C Luz ");
            char pct[4];
            u8_to_str(pct, Confort_GetLightPercent());
            strcat(l2, pct);
            strcat(l2, "%");
            ui_set_lcd("1Temp 2Son", l2);
            break;
        }
        case UI_SERVICES:
            ui_set_lcd("1Radio 2Horno",
                        "3Merc *Vol");
            break;

        case UI_TEMP_INPUT:
            lcd_line1[0] = '\0';
            strcpy(lcd_line1, "Temp obj 10-40");
            strcpy(lcd_line2, input_buf);
            strcat(lcd_line2, "C #OK *Vol");
            lcd_dirty = 1;
            break;

        case UI_SOUND: {
            char l1[LCD_COLS + 1];
            strcpy(l1, Confort_IsSoundEnabled() ? "Sonido ON" : "Sonido OFF");
            char l2[LCD_COLS + 1] = "Vol ";
            char pct[4];
            u8_to_str(pct, Confort_GetVolumePercent());
            strcat(l2, pct);
            strcat(l2, "% 1ON 2OFF");
            ui_set_lcd(l1, l2);
            break;
        }

        case UI_RFID_RECHARGE_INPUT:
            strcpy(lcd_line1, "Cupos: ");
            strncat(lcd_line1, input_buf, LCD_COLS - strlen(lcd_line1));
            lcd_line1[LCD_COLS] = '\0';
            strcat(lcd_line1, " #OK");
            strcpy(lcd_line2, "*Volver");
            lcd_dirty = 1;
            break;

        case UI_MARKET:
            ui_set_lcd("1Agreg 2Ver",
                       "3Borrar *Vol");
            break;

        case UI_MARKET_ADD_PROD: {
            char l1[LCD_COLS + 1];
            strcpy(l1, "Prod: ");
            if (market_add_product > 0) {
                char id[2] = {0};
                id[0] = (char)('0' + market_add_product);
                strcat(l1, id);
                strcat(l1, " ");
                strncat(l1, Remoto_MarketGetProductName(market_add_product),
                        LCD_COLS - strlen(l1));
            } else {
                strcat(l1, "1-8");
            }
            ui_set_lcd(l1, "1-8 *Vol");
            break;
        }

        case UI_MARKET_ADD_QTY: {
            char l1[LCD_COLS + 1];
            l1[0] = '\0';
            strcat(l1, "Prod: ");
            if (market_add_product > 0) {
                char id[2] = {0};
                id[0] = (char)('0' + market_add_product);
                strcat(l1, id);
                strcat(l1, " ");
                strncat(l1, Remoto_MarketGetProductName(market_add_product),
                        LCD_COLS - strlen(l1));
            } else {
                strcat(l1, "?");
            }
            l1[LCD_COLS] = '\0';
            lcd_line1[0] = '\0';
            strcpy(lcd_line1, l1);
            lcd_line2[0] = '\0';
            strcpy(lcd_line2, "Cant: ");
            strncat(lcd_line2, input_buf, LCD_COLS - strlen(lcd_line2));
            lcd_line2[LCD_COLS] = '\0';
            strcat(lcd_line2, " #OK");
            lcd_dirty = 1;
            break;
        }

        case UI_MARKET_VIEW: {
            market_item_t item;
            uint8_t count = Remoto_MarketGetCount();
            char l1[LCD_COLS + 1];
            char l2[LCD_COLS + 1];
            if (count == 0) {
                ui_set_lcd("Lista vacia", "*Volver");
            } else if (market_view_index < count &&
                       Remoto_MarketGetItem(market_view_index, &item)) {
                l1[0] = '\0';
                u8_to_str(l1, market_view_index + 1);
                strcat(l1, "/");
                u8_to_str(l1 + strlen(l1), count);
                strcat(l1, " ");
                strncat(l1, Remoto_MarketGetProductName(item.product_id),
                        LCD_COLS - strlen(l1));
                l1[LCD_COLS] = '\0';
                l2[0] = '\0';
                strcat(l2, "x");
                u8_to_str(l2 + 1, item.quantity);
                strcat(l2, " <A D> *V");
                ui_set_lcd(l1, l2);
            } else {
                ui_set_lcd("Error", "*Volver");
            }
            break;
        }

        default:
            ui_set_lcd("", "");
            break;
    }
}

/* ---- Key handling ---- */

static void ui_handle_key(char key) {
    UART_WriteString(SER_TECLADO "Tecla: ");
    UART_WriteChar(key);
    UART_Newline();

    switch (screen) {
        case UI_HOME:
            if (key == 'A') {
                previous_screen = screen;
                screen = UI_SECURITY;
            } else if (key == 'B') {
                previous_screen = screen;
                screen = UI_RFID;
                rfid_menu_state = RFID_MENU;
                rfid_waiting = 0;
                rfid_showing_result = 0;
            } else if (key == 'C') {
                previous_screen = screen;
                screen = UI_ENV;
            } else if (key == 'D') {
                previous_screen = screen;
                screen = UI_SERVICES;
            }
            break;

        case UI_SECURITY:
            if (key == '*') {
                previous_screen = screen;
                screen = UI_HOME;
            } else if (key == '1') {
                sec_code_target = 1;
                sec_code_error = 0;
                input_clear();
                screen = UI_SEC_CODE_INPUT;
            } else if (key == '2') {
                sec_code_target = 2;
                sec_code_error = 0;
                input_clear();
                screen = UI_SEC_CODE_INPUT;
            }
            break;

        case UI_SEC_CODE_INPUT:
            if (sec_code_error) {
                if (key == '*') {
                    sec_code_error = 0;
                    input_clear();
                    screen = UI_SECURITY;
                }
            } else {
                if (key >= '0' && key <= '9') {
                    input_push(key);
                } else if (key == '*') {
                    input_clear();
                    screen = UI_SECURITY;
                } else if (key == '#') {
                    if (strcmp(input_buf, CODIGO_ADMIN) == 0) {
                        if (sec_code_target == 1) {
                            Seguridad_SetAccessAlarm(!Seguridad_GetAccessState());
                            UART_WriteEvent(SER_ALARMA, "Codigo valido");
                        } else {
                            Seguridad_SetFireAlarm(!Seguridad_GetFireState());
                            UART_WriteEvent(SER_FUEGO, "Codigo valido");
                        }
                        input_clear();
                        screen = UI_SECURITY;
                    } else {
                        sec_code_error = 1;
                        if (sec_code_target == 1) {
                            UART_WriteEvent(SER_ALARMA, "Codigo invalido");
                        } else {
                            UART_WriteEvent(SER_FUEGO, "Codigo invalido");
                        }
                    }
                }
            }
            break;

        case UI_RFID:
            if (rfid_waiting || rfid_showing_result) {
                if (key == '*') {
                    Accesos_SetMode(ACCESS_MODE_NORMAL);
                    rfid_waiting = 0;
                    rfid_showing_result = 0;
                    rfid_menu_state = RFID_MENU;
                }
            } else {
                switch (rfid_menu_state) {
                    case RFID_MENU:
                        if (key == '*') {
                            Accesos_SetMode(ACCESS_MODE_NORMAL);
                            previous_screen = screen;
                            screen = UI_HOME;
                        } else if (key == '1') {
                            Accesos_SetMode(ACCESS_MODE_MAIN_DOOR);
                            rfid_waiting = 1;
                        } else if (key == '2') {
                            Accesos_SetMode(ACCESS_MODE_GARAGE);
                            rfid_waiting = 1;
                        } else if (key == '3') {
                            Accesos_SetMode(ACCESS_MODE_GAME_ROOM);
                            rfid_waiting = 1;
                        } else if (key == '4') {
                            rfid_menu_state = RFID_ENROLL_CHOOSE;
                        } else if (key == '5') {
                            Accesos_SetMode(ACCESS_MODE_DELETE_USER);
                            rfid_waiting = 1;
                        } else if (key == '6') {
                            previous_screen = screen;
                            screen = UI_RFID_RECHARGE_INPUT;
                            input_clear();
                        }
                        break;

                    case RFID_ENROLL_CHOOSE:
                        if (key == '*') {
                            rfid_menu_state = RFID_MENU;
                        } else if (key == '1') {
                            Accesos_SetMode(ACCESS_MODE_ENROLL_PARENT);
                            rfid_waiting = 1;
                        } else if (key == '2') {
                            Accesos_SetMode(ACCESS_MODE_ENROLL_CHILD);
                            rfid_waiting = 1;
                        }
                        break;
                }
            }
            break;

        case UI_ENV:
            if (key == '*') {
                previous_screen = screen;
                screen = UI_HOME;
            } else if (key == '1') {
                previous_screen = screen;
                screen = UI_TEMP_INPUT;
                input_clear();
            } else if (key == '2') {
                previous_screen = screen;
                screen = UI_SOUND;
            }
            break;

        case UI_TEMP_INPUT:
            if (key >= '0' && key <= '9') {
                input_push(key);
            } else if (key == '*') {
                input_clear();
                previous_screen = screen;
                screen = UI_ENV;
            } else if (key == '#') {
                uint16_t temp = input_to_uint();
                if (temp >= 10 && temp <= 40) {
                    Confort_SetTargetTemp((uint8_t)temp);
                    previous_screen = screen;
                    screen = UI_ENV;
                } else {
                    input_clear();
                    ui_set_lcd("Temp invalida", "Rango 10-40");
                }
            }
            break;

        case UI_SOUND:
            if (key == '*') {
                screen = previous_screen;
            } else if (key == '1') {
                Confort_SetSoundEnabled(1);
            } else if (key == '2') {
                Confort_SetSoundEnabled(0);
            }
            break;

        case UI_SERVICES:
            if (key == '*') {
                previous_screen = screen;
                screen = UI_HOME;
            } else if (key == '1') {
                previous_screen = screen;
                screen = UI_SOUND;
                lcd_dirty = 1;
            } else if (key == '2') {
                UART_WriteEvent(SER_HORNO, "Menu horno");
                ui_set_lcd("Horno", "Pendiente");
            } else if (key == '3') {
                UART_WriteEvent(SER_MERCADO, "Menu mercado");
                previous_screen = screen;
                screen = UI_MARKET;
            }
            break;

        case UI_MARKET:
            if (key == '*') {
                previous_screen = screen;
                screen = UI_SERVICES;
            } else if (key == '1') {
                market_add_product = 0;
                input_clear();
                previous_screen = screen;
                screen = UI_MARKET_ADD_PROD;
            } else if (key == '2') {
                market_view_index = 0;
                previous_screen = screen;
                screen = UI_MARKET_VIEW;
            } else if (key == '3') {
                Remoto_MarketClear();
                UART_WriteEvent(SER_MERCADO, "Lista borrada");
                ui_set_lcd("Lista borrada", "*Volver");
            }
            break;

        case UI_MARKET_ADD_PROD:
            if (key == '*') {
                market_add_product = 0;
                input_clear();
                previous_screen = screen;
                screen = UI_MARKET;
            } else if (key >= '1' && key <= '8') {
                market_add_product = (uint8_t)(key - '0');
                previous_screen = screen;
                screen = UI_MARKET_ADD_QTY;
                input_clear();
            }
            break;

        case UI_MARKET_ADD_QTY:
            if (key >= '0' && key <= '9') {
                input_push(key);
            } else if (key == '*') {
                input_clear();
                previous_screen = screen;
                screen = UI_MARKET_ADD_PROD;
            } else if (key == '#') {
                uint16_t qty = input_to_uint();
                if (qty >= 1 && qty <= 99 && market_add_product > 0) {
                    if (Remoto_MarketAdd(market_add_product, (uint8_t)qty)) {
                        UART_WriteEvent(SER_MERCADO, "Item agregado");
                        ui_set_lcd("Agregado", "*Volver");
                    } else {
                        ui_set_lcd("Lista llena", "*Volver");
                    }
                    market_add_product = 0;
                    input_clear();
                    previous_screen = screen;
                    screen = UI_MARKET;
                } else {
                    input_clear();
                    ui_set_lcd("Cantidad", "Rango 1-99");
                }
            }
            break;

        case UI_MARKET_VIEW:
            if (key == '*') {
                market_view_index = 0;
                previous_screen = screen;
                screen = UI_MARKET;
            } else if (key == 'A') {
                if (market_view_index > 0) {
                    market_view_index--;
                }
            } else if (key == 'D') {
                if (market_view_index + 1 < Remoto_MarketGetCount()) {
                    market_view_index++;
                }
            }
            break;

        case UI_RFID_RECHARGE_INPUT:
            if (key >= '0' && key <= '9') {
                input_push(key);
            } else if (key == '*') {
                input_clear();
                previous_screen = screen;
                screen = UI_RFID;
                rfid_menu_state = RFID_MENU;
            } else if (key == '#') {
                uint16_t credits = input_to_uint();
                if (credits >= 1 && credits <= 250) {
                    Accesos_SetPendingCredits((uint8_t)credits);
                    Accesos_SetMode(ACCESS_MODE_RECHARGE_CHILD);
                    previous_screen = screen;
                    screen = UI_RFID;
                    rfid_waiting = 1;
                } else {
                    input_clear();
                    ui_set_lcd("Invalido", "Rango 1-250");
                }
            }
            break;

        default:
            previous_screen = screen;
            screen = UI_HOME;
            break;
    }
}

/* ---- Public API ---- */

void UI_Init(void) {
    Keypad_Init();
    LCD_Init();
    screen = UI_HOME;
    previous_screen = UI_HOME;
    rfid_menu_state = RFID_MENU;
    rfid_waiting = 0;
    rfid_showing_result = 0;
    sec_code_target = 0;
    sec_code_error = 0;
    input_clear();
    lcd_dirty = 1;
    refresh_tick = 0;
    last_key_pending = 0;
    ui_render();
    ui_refresh();
    UART_WriteEvent(SER_SISTEMA, "UI iniciada");
}

void UI_Task(uint32_t now_ms) {
    char key;

    Keypad_Scan(now_ms);
    key = Keypad_GetKey();
    if (key != '\0') {
        last_key = key;
        last_key_pending = 1;
        ui_handle_key(key);
        ui_render();
    }

    /* RFID result polling (always active) */
    if (screen == UI_RFID && rfid_waiting && !rfid_showing_result) {
        char msg[17];
        if (Accesos_GetResultMsg(msg, sizeof(msg))) {
            ui_set_lcd(msg, "");
            rfid_waiting = 0;
            rfid_showing_result = 1;
            rfid_result_tick = now_ms;
        }
    }
    if (rfid_showing_result && Timer_Expired(rfid_result_tick, 2000)) {
        rfid_showing_result = 0;
        rfid_waiting = 0;
        ui_render();
    }

    /* Periodic refresh for dynamic screens */
    if (Timer_Expired(refresh_tick, 500)) {
        refresh_tick = now_ms;
        if (screen == UI_SECURITY || screen == UI_ENV || screen == UI_SOUND || screen == UI_MARKET_VIEW) {
            ui_render();
        }
    }

    ui_refresh();
}

uint8_t UI_LastKey(char *key) {
    if (!last_key_pending || key == 0) return 0;
    *key = last_key;
    last_key_pending = 0;
    return 1;
}
