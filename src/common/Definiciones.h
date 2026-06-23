#ifndef DEFINICIONES_H
#define DEFINICIONES_H

#include <stdint.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* ============================================================
 *  MAPA DE PINES — ATmega2560 / Arduino Mega 2560
 *  Fuente: docs/02_HARDWARE_Y_PINOUT.md
 * ============================================================ */

/* --- SPI / RFID-RC522 --- */
#ifndef PIN_SPI_SS
#define PIN_SPI_SS                53
#endif
#ifndef PIN_SPI_MOSI
#define PIN_SPI_MOSI              51
#endif
#ifndef PIN_SPI_MISO
#define PIN_SPI_MISO              50
#endif
#ifndef PIN_SPI_SCK
#define PIN_SPI_SCK               52
#endif

#define PIN_RFID_SS               PIN_SPI_SS
#define PIN_RFID_RST              49

/* --- LCD paralelo 16x2 --- */
#define PIN_LCD_RS                22
#define PIN_LCD_E                 23
#define PIN_LCD_D4                24
#define PIN_LCD_D5                25
#define PIN_LCD_D6                26
#define PIN_LCD_D7                27

/* --- Teclado matricial 4x4 --- */
#define PIN_KEYPAD_ROW1           34
#define PIN_KEYPAD_ROW2           35
#define PIN_KEYPAD_ROW3           36
#define PIN_KEYPAD_ROW4           37
#define PIN_KEYPAD_COL1           62
#define PIN_KEYPAD_COL2           63
#define PIN_KEYPAD_COL3           64
#define PIN_KEYPAD_COL4           65

#define PIN_KEYPAD_R1             PIN_KEYPAD_ROW1
#define PIN_KEYPAD_R2             PIN_KEYPAD_ROW2
#define PIN_KEYPAD_R3             PIN_KEYPAD_ROW3
#define PIN_KEYPAD_R4             PIN_KEYPAD_ROW4
#define PIN_KEYPAD_C1             PIN_KEYPAD_COL1
#define PIN_KEYPAD_C2             PIN_KEYPAD_COL2
#define PIN_KEYPAD_C3             PIN_KEYPAD_COL3
#define PIN_KEYPAD_C4             PIN_KEYPAD_COL4

/* --- Sensores --- */
#define PIN_PIR1                  38
#define PIN_PIR2                  39
#define PIN_PIR_1                 PIN_PIR1
#define PIN_PIR_2                 PIN_PIR2

/* --- ADC --- */
#define PIN_MQ2                    0
#define PIN_LIGHT_POT              1
#define PIN_VOLUME_POT             2

#define ADC_MQ2                   PIN_MQ2
#define ADC_LIGHT_POT             PIN_LIGHT_POT
#define ADC_VOLUME_POT            PIN_VOLUME_POT

/* --- Actuadores digitales --- */
#define PIN_HEATER                40
#define PIN_FAN                   41
#define PIN_OVEN                  42

#define PIN_HEATER_LED            PIN_HEATER
#define PIN_FAN_LED               PIN_FAN
#define PIN_OVEN_LED              PIN_OVEN

/* --- PWM --- */
#define PIN_PWM_DOOR_LED           6
#define PIN_PWM_LIGHT              7
#define PIN_PWM_SOUND              8
#define PIN_PWM_SERVO_GARAGE       9
#define PIN_PWM_BUZZER_ALARM      10

#define PIN_DOOR_PWM              PIN_PWM_DOOR_LED
#define PIN_LIGHT_PWM             PIN_PWM_LIGHT
#define PIN_SOUND_PWM             PIN_PWM_SOUND
#define PIN_GARAGE_SERVO          PIN_PWM_SERVO_GARAGE
#define PIN_ALARM_BUZZER          PIN_PWM_BUZZER_ALARM

#define PIN_FIRE_LED              11

#define PIN_DOOR_LED              PIN_PWM_DOOR_LED
#define PIN_BUZZER_ALARM          PIN_PWM_BUZZER_ALARM


/* ============================================================
 *  CONSTANTES DEL SISTEMA
 * ============================================================ */

#define CODIGO_ADMIN            "123"
#define CODE_LEN                   3

#define MAX_USERS                 10
#define RFID_UID_LEN               4   /* longitud por defecto (UID corto / inyeccion UART) */
#define RFID_UID_MAX              10   /* UID maximo ISO14443A (triple cascada: 4/7/10 bytes) */
#define USER_NAME_LEN              8

#define MARKET_MAX_ITEMS           8
#define MARKET_PRODUCT_COUNT        8

#define EEPROM_MAGIC_0           'D'
#define EEPROM_MAGIC_1           'H'
#define EEPROM_VERSION             3
#define EEPROM_MARKET_ADDR      0x100
#define EEPROM_MARKET_MAGIC      'M'

#define UART_BAUD_DEFAULT      9600UL

#define REMOTE_LINE_MAX            64
#define REMOTE_MAX_TOKENS           5

#define UART_DEBUG_ID               0
#define UART_REMOTE_ID              1
#define UART_RADIO_ID               2
#define UART_OVEN_ID                3

#ifndef DOMOTIC_HOME_ENABLE_DEMO_SEED
#define DOMOTIC_HOME_ENABLE_DEMO_SEED 0
#endif


/* ============================================================
 *  TIPOS GLOBALES — Estados de aplicación
 *  Fuente: docs/01_ARQUITECTURA.md
 * ============================================================ */

typedef enum {
    APP_BOOT = 0,
    APP_LOGIN,
    APP_MAIN_MENU,
    APP_SECURITY_MENU,
    APP_RFID_MENU,
    APP_ENV_MENU,
    APP_SERVICES_MENU,
    APP_ALERT_SCREEN,
    APP_ERROR_SCREEN
} app_state_t;

typedef uint8_t user_type_t;

#define USER_EMPTY   0
#define USER_PARENT  1
#define USER_CHILD   2

typedef struct {
    uint8_t access_enabled;
    uint8_t fire_enabled;
    uint8_t access_triggered;
    uint8_t fire_triggered;
    uint8_t pir1_active;
    uint8_t pir2_active;
    uint16_t smoke_adc;
    uint8_t smoke_percent;
} security_state_t;

typedef struct {
    uint8_t enabled;
    uint16_t target_temp_c;
    uint32_t remaining_seconds;
    uint32_t end_tick_ms;
} oven_state_t;

typedef struct {
    uint8_t enabled;
    uint8_t volume_percent;
} sound_state_t;

typedef struct {
    uint8_t active;
    uint8_t uid[RFID_UID_MAX];
    uint8_t uid_len;
    user_type_t type;
    uint8_t game_credits;
    char label[USER_NAME_LEN];
} user_record_t;

typedef struct {
    uint8_t product_id;
    uint8_t quantity;
} market_item_t;

typedef struct {
    uint8_t magic0;
    uint8_t magic1;
    uint8_t version;
    uint8_t user_count;
    uint8_t checksum;
} eeprom_header_t;


/* ============================================================
 *  MACROS DE EVENTOS SERIALES
 *  Formato: [TAG] mensaje
 *  Fuente: docs/01_ARQUITECTURA.md — Eventos seriales mínimos
 * ============================================================ */

#define SER_BOOT    "[BOOT] "
#define SER_LOGIN   "[LOGIN] "
#define SER_ALARMA  "[ALARMA ACCESO] "
#define SER_FUEGO   "[ALARMA INCENDIO] "
#define SER_RFID    "[RFID] "
#define SER_JUEGOS  "[JUEGOS] "
#define SER_HORNO   "[HORNO] "
#define SER_SONIDO  "[SONIDO] "
#define SER_MERCADO "[MERCADO] "
#define SER_EEPROM  "[EEPROM] "
#define SER_SISTEMA "[SISTEMA] "
#define SER_TECLADO "[TECLADO] "

#define REMOTE_OK     "[OK]"
#define REMOTE_ERR    "[ERR]"
#define REMOTE_STATUS "[STATUS]"
#define REMOTE_DBG    "[DBG]"

#define MOD_SISTEMA "SISTEMA"
#define MOD_RADIO   "RADIO"
#define MOD_HORNO   "HORNO"
#define MOD_MERCADO "MERCADO"

#endif /* DEFINICIONES_H */
