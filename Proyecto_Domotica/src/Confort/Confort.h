#ifndef CONFORT_H
#define CONFORT_H

#include <Arduino.h>

void inicializarConfort();
uint16_t leerADC(uint8_t canal);
void actualizarClimatizacion();
void actualizarIluminacion();

#endif
