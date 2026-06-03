#ifndef KEYPAD_H
#define KEYPAD_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 *  keypad.h — Driver de teclado matricial 4x4
 *
 *  Plataforma  : ATmega2560 / Arduino Mega 2560
 *  Interrupción: PCINT2 (columnas en PORTK PK0-PK3)
 *  Lenguaje    : C puro, sin Keypad library
 *
 *  Funciones públicas:
 *    keypad_init()        — Configura pines, pull-ups y PCINT
 *    keypad_scan(now_ms)  — Procesa interrupción, barre matriz,
 *                           aplica antirrebote, registra evento
 *    keypad_get_key()     — Consume y retorna tecla, o '\0'
 *
 *  Mapa de teclas (filas x columnas):
 *               C1  C2  C3  C4
 *         R1    1   2   3   A
 *         R2    4   5   6   B
 *         R3    7   8   9   C
 *         R4    *   0   #   D
 *
 *  Contrato:
 *    - keypad_init() se llama una vez en setup()
 *    - keypad_scan() se llama desde loop() con millis() actual
 *    - keypad_get_key() consume el evento; dos llamadas
 *      segundas devuelven '\0' sin nuevo evento
 * ============================================================ */

/* ----------------------------------------------------------
 *  keypad_init  —  Inicializa pines e interrupción PCINT2
 *
 *  Configura:
 *    - Filas D34-D37 (PORTL PL0-PL3) como salidas, LOW en reposo
 *    - Columnas D62-D65 / A8-A11 (PORTK PK0-PK3) como entradas pull-up
 *    - PCINT2 (PCINT16-PCINT19) para detectar cambio en PK0-PK3
 * ---------------------------------------------------------- */
void keypad_init(void);

/* ----------------------------------------------------------
 *  keypad_scan  —  Tarea cooperativa no bloqueante
 *
 *  Parámetros:
 *    now_ms  —  Marca de tiempo actual en milisegundos
 *
 *  Comportamiento:
 *    - Si hay bandera de interrupción, espera antirrebote
 *    - Barre filas activando una a la vez, lee columnas
 *    - Si detecta tecla nueva, almacena evento interno
 *    - Si tecla sigue presionada, NO repite evento
 *    - Si tecla liberada, permite nueva detección
 * ---------------------------------------------------------- */
void keypad_scan(uint32_t now_ms);

/* ----------------------------------------------------------
 *  keypad_get_key  —  Retorna y consume evento de tecla
 *
 *  Retorna:
 *    - Carácter de la tecla ('0'-'9', 'A'-'D', '*', '#')
 *    - '\0' si no hay evento pendiente
 *
 *  Consume el evento: una segunda llamada sin nueva
 *  pulsación retorna '\0'.
 * ---------------------------------------------------------- */
char keypad_get_key(void);

#ifdef __cplusplus
}
#endif

#endif /* KEYPAD_H */
