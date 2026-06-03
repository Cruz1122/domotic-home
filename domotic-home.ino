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
#include "src/keypad/keypad.h"
#include "src/seguridad/Seguridad.h"
#include "src/accesos/Accesos.h"
#include "src/confort/Confort.h"
#include "src/remoto/Remoto.h"
#include "src/ui/UI.h"

void setup(void) {
    Serial.begin(9600);
    Serial.println("[BOOT] Sistema iniciado");

    keypad_init();
    seguridad_init();
    accesos_init();
    confort_init();
    remoto_init();
    ui_init();

    Serial.println("[BOOT] Keypad listo — presione teclas para probar");
}

void loop(void) {
    uint32_t now_ms = millis();
    char tecla;

    keypad_scan(now_ms);
    seguridad_task();
    accesos_task();
    confort_task();
    remoto_task();
    ui_task();

    tecla = keypad_get_key();
    if (tecla != '\0') {
        Serial.print("[TECLADO] Tecla: ");
        Serial.println(tecla);
    }
}
