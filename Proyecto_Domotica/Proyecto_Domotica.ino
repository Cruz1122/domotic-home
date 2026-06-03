#include "src/Definiciones.h"
#include "src/Seguridad/Seguridad.h"
#include "src/Accesos/Accesos.h"
#include "src/Confort/Confort.h"
#include "src/Remoto/Remoto.h"
#include "src/Pantalla/Pantalla.h"

Usuario hijoPrueba = {{0xAA, 0xBB, 0xCC, 0xDD}, 2, 3};

void setup() {
    Serial.begin(9600);

    inicializarSeguridad();
    inicializarAccesos();
    inicializarConfort();
    inicializarRemoto();
    inicializarPantalla();

    Serial.println("--- SISTEMA INTEGRAL DOMOTICO ATMEGA2560 COMPLETADO Y CORRIENDO ---");
}

void loop() {
    bool hayIntrusion = verificarIntrusion();
    bool hayIncendio = verificarIncendio();

    if (hayIntrusion) Serial.println("ALERTA TELEMETRIA: !Falla de seguridad perimetral!");
    if (hayIncendio)  Serial.println("ALERTA TELEMETRIA: !Presencia de humo detectada!");

    procesarControlAcceso(hijoPrueba);

    actualizarClimatizacion();
    actualizarIluminacion();

    escucharComandosRemotos();
    ejecutarTemporizadorHorno();

    actualizarPantalla(hayIntrusion, hayIncendio);
}
