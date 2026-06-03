#include "../Definiciones.h"
#include "Seguridad.h"

void inicializarSeguridad() {
    DDR_SEGURIDAD &= ~((1 << BIT_VENTANA1) | (1 << BIT_VENTANA2) | (1 << BIT_PUERTA) | (1 << BIT_HUMO));

    PORT_SEGURIDAD |= (1 << BIT_VENTANA1) | (1 << BIT_VENTANA2) | (1 << BIT_PUERTA) | (1 << BIT_HUMO);
}

bool verificarIntrusion() {
    if (!(PIN_SEGURIDAD & (1 << BIT_VENTANA1)) ||
        !(PIN_SEGURIDAD & (1 << BIT_VENTANA2)) ||
        !(PIN_SEGURIDAD & (1 << BIT_PUERTA))) {
        return true;
    }
    return false;
}

bool verificarIncendio() {
    if (!(PIN_SEGURIDAD & (1 << BIT_HUMO))) {
        return true;
    }
    return false;
}
