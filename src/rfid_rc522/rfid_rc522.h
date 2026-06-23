/*
 * Módulo: RFID (RC522)
 * Driver híbrrido de lectura de UID:
 *  - FUENTE 1: RC522 real por SPI con librería MFRC522. Si al arrancar no
 *    detecta el chip, se autodesactiva.
 *  - FUENTE 2: inyección de UID por UART0 (simulación Proteus): se teclea el
 *    UID en hex + Enter en el Virtual Terminal.
 * Así el MISMO firmware sirve en placa real y en Proteus (definir RFID_SIMULATED=1
 * deshabilita el RC522 y deja solo la inyección por UART).
 */
#ifndef RFID_RC522_H
#define RFID_RC522_H

#include <stdint.h>
#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configuración de fuentes de UID (ver cabecera del archivo). */
#ifndef RFID_USE_RC522
#define RFID_USE_RC522    1
#endif
#ifndef RFID_UART_INJECT
#define RFID_UART_INJECT  1
#endif

#ifdef RFID_SIMULATED
#if RFID_SIMULATED
#undef  RFID_USE_RC522
#define RFID_USE_RC522    0
#endif
#endif

/* Resultado de una lectura de UID. NO_CARD = no hay tarjeta cerca. */
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
