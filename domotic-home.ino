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
#include "src/seguridad/Seguridad.h"
#include "src/accesos/Accesos.h"
#include "src/confort/Confort.h"
#include "src/remoto/Remoto.h"
#include "src/ui/UI.h"

void setup(void) {
    Serial.begin(9600);
    Serial.println("[BOOT] Sistema iniciado");

    seguridad_init();
    accesos_init();
    confort_init();
    remoto_init();
    ui_init();

    Serial.println("[BOOT] LCD OK si ves cursor");
    Serial.println("[BOOT] Teclado: PORTC filas, PORTK cols");
}

void loop(void) {
    uint32_t now_ms = millis();
    static char last_key;

    seguridad_task();
    accesos_task();
    confort_task();
    remoto_task();
    ui_task(now_ms);

    /* Debug serial: si imprime aqui, teclado OK y falla LCD */
    if (ui_last_key(&last_key)) {
        Serial.print("[TECLADO] ");
        Serial.println(last_key);
    }
}
