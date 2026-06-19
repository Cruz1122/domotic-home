#ifndef RFID_RC522_H
#define RFID_RC522_H

#include <stdint.h>
#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------
 *  Configuracion de fuentes de UID
 *
 *  El driver es hibrido: puede leer de un RC522 real por SPI y/o
 *  aceptar UIDs inyectados por la UART consola. Esto permite usar el
 *  MISMO firmware en la placa fisica (RC522 presente) y en Proteus
 *  (sin RC522: se teclea el UID hex + Enter en el Virtual Terminal).
 *
 *  - RFID_USE_RC522   : compila el driver SPI real. Si en el arranque
 *                       no se detecta el chip, se autodesactiva.
 *  - RFID_UART_INJECT : compila la inyeccion de UID por UART consola.
 *
 *  Compatibilidad: definir RFID_SIMULATED=1 deshabilita el RC522 real
 *  y deja solo la inyeccion por UART (modo Proteus puro).
 * ------------------------------------------------------------------ */
#ifndef RFID_USE_RC522
#define RFID_USE_RC522    1
#endif
#ifndef RFID_UART_INJECT
#define RFID_UART_INJECT  1
#endif

#ifdef RFID_SIMULATED 1
#if RFID_SIMULATED
#undef  RFID_USE_RC522
#define RFID_USE_RC522    0
#endif
#endif

typedef enum {
    RFID_OK = 0,
    RFID_NO_CARD,
    RFID_TIMEOUT,
    RFID_COLLISION,
    RFID_CRC_ERROR,
    RFID_UNSUPPORTED_UID,
    RFID_HW_ERROR
} rfid_status_t;

void     RFID_Init(void);
void     RFID_Task(uint32_t now_ms);
uint8_t  RFID_UIDAvailable(void);
rfid_status_t RFID_ReadUIDEx(uint8_t *uid_out, uint8_t *uid_len);
rfid_status_t RFID_GetLastError(void);
uint8_t  RFID_ReadUID(uint8_t *uid_out);

#ifdef __cplusplus
}
#endif

#endif
