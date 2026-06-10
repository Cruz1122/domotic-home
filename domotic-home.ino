/* ============================================================
 *  domotic-home — Orquestador principal
 *
 *  setup()   : inicializa todos los módulos
 *  loop()    : ejecuta tareas cooperativas no bloqueantes
 *
 *  Arquitectura: máquina de estados, sin delay() en lógica
 *  Plataforma  : ATmega2560 / Arduino Mega 2560
 *  Lenguaje    : C puro, sin librerías externas
 * ============================================================ */

#include <Arduino.h>
#include "src/common/Definiciones.h"
#include "src/timer/timer.h"
#include "src/gpio/gpio.h"
#include "src/uart/uart.h"
#include "src/keypad/keypad.h"
#include "src/seguridad/Seguridad.h"
#include "src/accesos/Accesos.h"
#include "src/confort/Confort.h"
#include "src/remoto/Remoto.h"
#include "src/eeprom/eeprom.h"
#include "src/rfid_rc522/rfid_rc522.h"
#include "src/ui/UI.h"

static uint32_t hb_tick = 0;
static uint8_t  eeprom_checked = 0;

void setup(void) {
    UART_Init(9600);
    UART_WriteEvent(SER_BOOT, "Sistema iniciado");
    GPIO_Init();
    Timer_Init();
    EEPROM_Init();

    RFID_Init();
    keypad_init();
    seguridad_init();
    accesos_init();
    confort_init();
    remoto_init();
    ui_init();

    UART_WriteEvent(SER_BOOT, "Keypad listo — presione teclas para probar");
}

void loop(void) {
    Timer_Task();
    uint32_t now_ms = Timer_GetMs();
    char tecla;

    if (!eeprom_checked) {
        eeprom_checked = 1;
        user_record_t u;
        if (EEPROM_LoadUser(0, &u)) {
            UART_WriteEvent(SER_EEPROM, "User 0 persistido OK");
        } else {
            u.active = 1;
            u.uid[0] = 0x01; u.uid[1] = 0x02; u.uid[2] = 0x03;
            u.uid[3] = 0x04; u.uid[4] = 0x05;
            u.type = USER_CHILD;
            u.game_credits = 10;
            EEPROM_SaveUser(0, &u);
            UART_WriteEvent(SER_EEPROM, "User 0 escrito (prueba)");
        }
    }

    if (Timer_Expired(hb_tick, 5000)) {
        hb_tick = now_ms;
        UART_WriteEvent(SER_SISTEMA, "Heartbeat OK");
    }

    RFID_Task(now_ms);
    keypad_scan(now_ms);
    seguridad_task();
    accesos_task();
    confort_task();
    remoto_task();
    ui_task(now_ms);

    if (RFID_UIDAvailable()) {
        uint8_t uid[RFID_UID_LEN];
        RFID_ReadUID(uid);
        UART_WriteString(SER_RFID "UID: ");
        for (uint8_t i = 0; i < RFID_UID_LEN; i++) {
            UART_WriteDecimal(uid[i]);
            if (i < RFID_UID_LEN - 1) UART_WriteChar(' ');
        }
        UART_Newline();
    }

    tecla = keypad_get_key();
    if (tecla != '\0') {
        UART_WriteString(SER_TECLADO "Tecla: ");
        UART_WriteChar(tecla);
        UART_Newline();
    }
}
