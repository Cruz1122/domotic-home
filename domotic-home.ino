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

#include "src/common/Definiciones.h"
#include "src/seguridad/Seguridad.h"
#include "src/accesos/Accesos.h"
#include "src/confort/Confort.h"
#include "src/remoto/Remoto.h"
#include "src/ui/UI.h"

void setup(void) {
    seguridad_init();
    accesos_init();
    confort_init();
    remoto_init();
    ui_init();
}

void loop(void) {
    seguridad_task();
    accesos_task();
    confort_task();
    remoto_task();
    ui_task();
}
