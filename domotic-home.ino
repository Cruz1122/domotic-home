/* ============================================================
 *  domotic-home — Orquestador principal
 *
 *  setup()   : inicializa todos los módulos
 *  loop()    : ejecuta tareas cooperativas no bloqueantes
 *
 *  Arquitectura: máquina de estados, sin delay() en lógica
 *  Plataforma  : ATmega2560 / Arduino Mega 2560
 *  Lenguaje    : C puro (RFID vía librería MFRC522)
 * ============================================================ */

#include <Arduino.h>
#include "src/common/Definiciones.h"
#include "src/timer/timer.h"
#include "src/gpio/gpio.h"
#include "src/uart/uart.h"
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
#include "src/test/test_bootstrap.h"

void setup(void) {
    /* Drivers y periféricos primero, luego módulos de aplicación, y la UI al final. */
    UART_Init(UART_BAUD_DEFAULT);
    UART_WriteEvent(SER_BOOT, "Sistema iniciado");
    GPIO_Init();
    ADC_Init();
    PWM_Init();
    ServoPwm_Init();
    Timer_Init();
    EEPROM_Init();

#if DOMOTIC_HOME_ENABLE_DEMO_SEED
    TestBootstrap_SeedDemoUser();
#endif

    RFID_Init();
    Seguridad_Init();
    Accesos_Init();
    Confort_Init();
    Remoto_Init();
    UI_Init();
}

void loop(void) {
    /* Tareas cooperativas: cada una corre rápido y devuelve el control.
     * Ninguna bloquea; todas usan now_ms como reloj (Timer_Millis). */
    uint32_t now_ms = Timer_Millis();

    /* UART0 por sondeo: RX del Monitor Serie y drenado de TX encolado. */
    UART_Task();

    /* Bridge UART0<->UART1: reenvía lo tecleado en el Monitor Serie (USB)
     * al parser de comandos y clona las respuestas al mismo canal. */
    UART_Bridge_Task();

    RFID_Task(now_ms);
    Seguridad_Task(now_ms);
    Accesos_Task(now_ms);
    Confort_Task(now_ms);
    Remoto_Task(now_ms);
    UI_Task(now_ms);

    UART_Task();
}
