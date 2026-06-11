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
#include "src/adc/adc.h"
#include "src/pwm/pwm.h"
#include "src/servo/servo_pwm.h"
#include "src/seguridad/Seguridad.h"
#include "src/accesos/Accesos.h"
#include "src/confort/Confort.h"
#include "src/remoto/Remoto.h"
#include "src/eeprom/eeprom.h"
#include "src/rfid_rc522/rfid_rc522.h"
#include "src/ui/UI.h"

static uint32_t hb_tick = 0;

static void seed_test_user(void) {
    user_record_t u;
    if (EEPROM_LoadUser(0, &u)) {
        UART_WriteEvent(SER_EEPROM, "User 0 persistido OK");
    } else {
        u.active = 1;
        u.uid[0] = 0x01; u.uid[1] = 0x02; u.uid[2] = 0x03;
        u.uid[3] = 0x04; u.uid[4] = 0x05;
        u.type = USER_CHILD;
        u.game_credits = 10;
        u.label[0] = '\0';
        EEPROM_SaveUser(0, &u);
        UART_WriteEvent(SER_EEPROM, "User 0 escrito (prueba)");
    }
}

void setup(void) {
    UART_Init(9600);
    UART_WriteEvent(SER_BOOT, "Sistema iniciado");
    GPIO_Init();
    ADC_Init();
    PWM_Init();
    ServoPwm_Init();
    Timer_Init();
    EEPROM_Init();

    seed_test_user();

    RFID_Init();
    Seguridad_Init();
    Accesos_Init();
    Confort_Init();
    Remoto_Init();
    UI_Init();

    UART_WriteEvent(SER_BOOT, "Confort — dimmer volumen temperatura OK");
}

void loop(void) {
    Timer_Task();
    uint32_t now_ms = Timer_GetMs();

    if (Timer_Expired(hb_tick, 5000)) {
        hb_tick = now_ms;
        UART_WriteEvent(SER_SISTEMA, "Heartbeat OK");
    }

    RFID_Task(now_ms);
    Seguridad_Task();
    Accesos_Task(now_ms);
    Confort_Task();
    Remoto_Task(now_ms);
    UI_Task(now_ms);
}
