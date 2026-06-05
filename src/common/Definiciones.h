#ifndef DEFINICIONES_H
#define DEFINICIONES_H

#include <stdint.h>

/* ============================================================
 *  MAPA DE PINES — ATmega2560 / Arduino Mega 2560
 *  Fuente: docs/02_HARDWARE_Y_PINOUT.md
 * ============================================================ */

/* --- RFID-RC522 (SPI) --- */
#define PIN_RFID_SS               53
#define PIN_RFID_RST              49
/* SCK=52, MOSI=51, MISO=50 son SPI nativo */

/* --- LCD paralelo 16x2 --- */
#define PIN_LCD_RS                22
#define PIN_LCD_E                 23
#define PIN_LCD_D4                24
#define PIN_LCD_D5                25
#define PIN_LCD_D6                26
#define PIN_LCD_D7                27

/* --- Teclado matricial 4x4 --- */
/* Filas D34-D37 (PORTL PL0-PL3), Columnas A8-A11 (PORTK PK0-PK3) */
#define PIN_KEYPAD_R1             34
#define PIN_KEYPAD_R2             35
#define PIN_KEYPAD_R3             36
#define PIN_KEYPAD_R4             37
#define PIN_KEYPAD_C1             62   /* A8 / PK0 */
#define PIN_KEYPAD_C2             63   /* A9 / PK1 */
#define PIN_KEYPAD_C3             64   /* A10 / PK2 */
#define PIN_KEYPAD_C4             65   /* A11 / PK3 */

/* --- Sensores --- */
#define PIN_PIR_1                 38
#define PIN_PIR_2                 39

/* --- Actuadores digitales --- */
#define PIN_HEATER_LED            40
#define PIN_FAN_LED               41
#define PIN_OVEN_LED              42

/* --- PWM --- */
#define PIN_DOOR_LED               6
#define PIN_LIGHT_PWM              7
#define PIN_SOUND_PWM              8
#define PIN_GARAGE_SERVO           9
#define PIN_ALARM_BUZZER          10

/* --- Canales ADC --- */
#define ADC_MQ2                    0
#define ADC_LIGHT_POT              1
#define ADC_VOLUME_POT             2


/* ============================================================
 *  CONSTANTES DEL SISTEMA
 * ============================================================ */

#define RFID_SIMULATED   1       /* Simula RFID por UART (Virtual Terminal) */

#define CODIGO_ADMIN            "1234"
#define CODE_LEN                   4

#define MAX_USERS                 10
#define RFID_UID_LEN               5
#define USER_NAME_LEN              8

#define MARKET_MAX_ITEMS           8

#define EEPROM_MAGIC_0           'D'
#define EEPROM_MAGIC_1           'H'
#define EEPROM_VERSION             1


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

typedef enum {
    USER_EMPTY = 0,
    USER_PARENT,
    USER_CHILD
} user_type_t;

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
    uint8_t uid[RFID_UID_LEN];
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

#endif /* DEFINICIONES_H */
