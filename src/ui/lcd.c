/* ============================================================
 *  lcd.c — Driver LCD HD44780 en modo 4 bits
 *
 *  Plataforma  : ATmega2560 / Arduino Mega 2560
 *  Display     : 16x2, conexión 4 bits (D4-D7), RW=GND
 *  Pines       : definidos en Definiciones.h (PORTA bits 0-5)
 *  Lenguaje    : C puro, sin LiquidCrystal
 *
 *  Diagrama de pines (Mega 2560):
 *    LCD_RS  (pin 22) → PORTA, bit 0
 *    LCD_E   (pin 23) → PORTA, bit 1
 *    LCD_D4  (pin 24) → PORTA, bit 2
 *    LCD_D5  (pin 25) → PORTA, bit 3
 *    LCD_D6  (pin 26) → PORTA, bit 4
 *    LCD_D7  (pin 27) → PORTA, bit 5
 *
 *  Secuencia de inicialización HD44780 estándar.
 *  No usa delay() de Arduino — solo NOP-loop waits en setup().
 * ============================================================ */

#include "lcd.h"
#include <avr/io.h>


/* ============================================================
 *  MAPEO DE PINES A PUERTO
 *
 *  Pins 22-27 del Mega 2560 están en PORTA (PA0-PA5).
 *  Aprovechamos acceso directo a registros en vez de
 *  digitalWrite() para minimizar sobrecarga.
 * ============================================================ */

#define LCD_PORT   PORTA
#define LCD_DDR    DDRA

#define LCD_RS     (1 << 0)   /* pin 22 = PA0 */
#define LCD_E      (1 << 1)   /* pin 23 = PA1 */
#define LCD_D4     (1 << 2)   /* pin 24 = PA2 */
#define LCD_D5     (1 << 3)   /* pin 25 = PA3 */
#define LCD_D6     (1 << 4)   /* pin 26 = PA4 */
#define LCD_D7     (1 << 5)   /* pin 27 = PA5 */

#define LCD_DATA_MASK  (LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7)
#define LCD_CTRL_MASK  (LCD_RS | LCD_E)
#define LCD_ALL_MASK   (LCD_CTRL_MASK | LCD_DATA_MASK)

/* Comandos HD44780 */
#define LCD_CMD_CLEAR        0x01
#define LCD_CMD_HOME         0x02
#define LCD_CMD_ENTRY_MODE   0x06
#define LCD_CMD_DISPLAY_OFF  0x08
#define LCD_CMD_DISPLAY_ON   0x0C
#define LCD_CMD_FUNC_SET     0x28   /* 4 bits, 2 lineas, 5x8 */

#define LCD_DDRAM_ROW0       0x00
#define LCD_DDRAM_ROW1       0x40
#define LCD_DDRAM_CMD        0x80   /* OR con direccion */


/* ============================================================
 *  HELPERs ESTÁTICOS — no expuestos en lcd.h
 * ============================================================ */

/* ----------------------------------------------------------
 *  lcd_pulse_e  —  Genera pulso en la línea Enable
 *
 *  El datasheet del HD44780 especifica:
 *    tPW (ancho pulso E)  ≥ 450 ns
 *    tCYC (ciclo E)       ≥ 1000 ns
 *  A 16 MHz, 1 ciclo = 62.5 ns. Usamos NOPs para margen.
 * ---------------------------------------------------------- */
static void lcd_pulse_e(void) {
    LCD_PORT |= LCD_E;               /* E HIGH */
    /* ~1 μs de mantenimiento (>>450 ns minimo) */
    __asm__ __volatile__ (
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
    );
    LCD_PORT &= ~LCD_E;              /* E LOW */
    /* Pequeña pausa para respetar tCYC */
    __asm__ __volatile__ (
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
    );
}


/* ----------------------------------------------------------
 *  lcd_write_nibble  —  Envía 4 bits a D4-D7 + pulso E
 *
 *  nibble : valor de 4 bits (solo se usan bits 0-3)
 *  rs     : 0 = comando, 1 = dato
 * ---------------------------------------------------------- */
static void lcd_write_nibble(uint8_t nibble, uint8_t rs) {
    uint8_t port_val = LCD_PORT;

    /* Limpiar bits de control y datos que vamos a modificar */
    port_val &= ~LCD_ALL_MASK;

    /* RS */
    if (rs) port_val |= LCD_RS;

    /* Colocar nibble en bits 2-5 (D4=Lsb, D7=Msb) */
    port_val |= ((nibble & 0x0F) << 2) & LCD_DATA_MASK;

    LCD_PORT = port_val;
    lcd_pulse_e();
}


/* ----------------------------------------------------------
 *  lcd_write_byte  —  Envía 1 byte (2 nibbles) en modo 4 bits
 *
 *  data : byte a enviar
 *  rs   : 0 = comando, 1 = dato
 * ---------------------------------------------------------- */
static void lcd_write_byte(uint8_t data, uint8_t rs) {
    lcd_write_nibble(data >> 4, rs);   /* nibble alto primero */
    lcd_write_nibble(data & 0x0F, rs); /* nibble bajo despues */
}


/* ----------------------------------------------------------
 *  lcd_delay_us  —  Microsegundos no bloqueantes (NOP loop)
 *
 *  ATmega2560 a 16 MHz:
 *    1 ciclo CPU = 62.5 ns
 *    12 NOP + loop overhead ≈ 1 μs
 *  El HD44780 tolera retardos mayores a los mínimos.
 * ---------------------------------------------------------- */
static void lcd_delay_us(uint16_t us) {
    while (us--) {
        __asm__ __volatile__ (
            "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
            "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
            "nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t"
        );
    }
}


/* ----------------------------------------------------------
 *  lcd_delay_ms  —  Milisegundos (usa lcd_delay_us)
 * ---------------------------------------------------------- */
static void lcd_delay_ms(uint16_t ms) {
    while (ms--) {
        lcd_delay_us(1000);
    }
}


/* ============================================================
 *  FUNCIONES PÚBLICAS  —  API de lcd.h
 * ============================================================ */

/* ----------------------------------------------------------
 *  lcd_init  —  Secuencia de inicialización HD44780 en 4 bits
 *
 *  Se ejecuta una vez en setup(). Usa NOP-loop waits,
 *  NO delay() de Arduino.
 *
 *  Secuencia estándar:
 *    1. Esperar >40 ms tras power-on
 *    2. Escribir 0x03 tres veces (handshake 8-bit)
 *    3. Escribir 0x02 (cambio a 4-bit)
 *    4. Configurar: 4-bit, 2 lineas, 5x8
 *    5. Display off → clear → entry mode → display on
 * ---------------------------------------------------------- */
void lcd_init(void) {
    /* Configurar pines LCD como salidas */
    LCD_DDR |= LCD_ALL_MASK;

    /* 1. Espera de estabilización tras power-on (>40 ms) */
    lcd_delay_ms(50);

    /* 2-3. Secuencia de inicialización estándar HD44780 */
    lcd_write_nibble(0x03, 0);  lcd_delay_ms(5);    /* >4.1 ms */
    lcd_write_nibble(0x03, 0);  lcd_delay_us(200);  /* >100 μs */
    lcd_write_nibble(0x03, 0);  lcd_delay_us(200);  /* >100 μs */
    lcd_write_nibble(0x02, 0);  lcd_delay_us(200);  /* >100 μs */

    /* 4. A partir de aquí, el LCD está en modo 4 bits */
    lcd_write_byte(LCD_CMD_FUNC_SET, 0);     lcd_delay_us(100);
    lcd_write_byte(LCD_CMD_DISPLAY_OFF, 0);  lcd_delay_us(100);
    lcd_write_byte(LCD_CMD_CLEAR, 0);        lcd_delay_ms(3);
    lcd_write_byte(LCD_CMD_ENTRY_MODE, 0);   lcd_delay_us(100);
    lcd_write_byte(LCD_CMD_DISPLAY_ON, 0);   lcd_delay_us(100);
}


/* ----------------------------------------------------------
 *  lcd_clear  —  Limpia la pantalla y coloca cursor en (0,0)
 * ---------------------------------------------------------- */
void lcd_clear(void) {
    lcd_write_byte(LCD_CMD_CLEAR, 0);
    lcd_delay_ms(3);
    lcd_write_byte(LCD_CMD_HOME, 0);
    lcd_delay_ms(3);
}


/* ----------------------------------------------------------
 *  lcd_set_cursor  —  Posiciona cursor en (col, row)
 *
 *  row 0: DDRAM address 0x00 – 0x0F
 *  row 1: DDRAM address 0x40 – 0x4F
 *  col debe estar entre 0 y 15
 * ---------------------------------------------------------- */
void lcd_set_cursor(uint8_t col, uint8_t row) {
    uint8_t address;

    if (row == 0) {
        address = LCD_DDRAM_ROW0 + col;
    } else {
        address = LCD_DDRAM_ROW1 + col;
    }

    lcd_write_byte(LCD_DDRAM_CMD | address, 0);
    lcd_delay_us(100);
}


/* ----------------------------------------------------------
 *  lcd_write_char  —  Escribe un carácter en posición actual
 * ---------------------------------------------------------- */
void lcd_write_char(char c) {
    lcd_write_byte((uint8_t)c, 1);
    lcd_delay_us(100);
}


/* ----------------------------------------------------------
 *  lcd_write_string  —  Escribe cadena terminada en '\0'
 * ---------------------------------------------------------- */
void lcd_write_string(const char *str) {
    while (*str) {
        lcd_write_char(*str);
        str++;
    }
}


/* ----------------------------------------------------------
 *  lcd_write_two_lines  —  Limpia y escribe dos líneas
 *
 *  Conveniencia para mostrar pantallas de 2 líneas.
 * ---------------------------------------------------------- */
void lcd_write_two_lines(const char *line1, const char *line2) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_write_string(line1);
    lcd_set_cursor(0, 1);
    lcd_write_string(line2);
}
