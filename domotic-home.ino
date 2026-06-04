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
#include "src/ui/UI.h"

static uint32_t hb_tick = 0;

void setup(void) {
    UART_Init(9600);
    UART_WriteEvent(SER_BOOT, "Sistema iniciado");
    GPIO_Init();
    Timer_Init();

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

    if (Timer_Expired(hb_tick, 5000)) {
        hb_tick = now_ms;
        UART_WriteEvent(SER_SISTEMA, "Heartbeat OK");
    }

    keypad_scan(now_ms);
    seguridad_task();
    accesos_task();
    confort_task();
    remoto_task();
    ui_task();

    tecla = keypad_get_key();
    if (tecla != '\0') {
        UART_WriteString(SER_TECLADO "Tecla: ");
        UART_WriteChar(tecla);
        UART_Newline();
    }
}
