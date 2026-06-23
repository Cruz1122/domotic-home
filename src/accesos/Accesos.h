/*
 * Módulo: Accesos
 * Gestiona RFID: validación de usuarios, enrolamiento, borrado, recarga de cupos,
 * puerta principal (LED imán), garaje (servo) y sala de juegos (cupos en EEPROM).
 * No bloquea esperando tarjeta: consume el UID que deja el driver RFID.
 */
#ifndef ACCESOS_H
#define ACCESOS_H

#include "../common/Definiciones.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Modo de acceso en curso: determina qué se hace con la próxima tarjeta leída. */
typedef enum {
    ACCESS_MODE_NORMAL = 0,         /* acceso a puerta principal por defecto */
    ACCESS_MODE_MAIN_DOOR,
    ACCESS_MODE_GARAGE,
    ACCESS_MODE_GAME_ROOM,
    ACCESS_MODE_ENROLL_PARENT,
    ACCESS_MODE_ENROLL_CHILD,
    ACCESS_MODE_DELETE_USER,
    ACCESS_MODE_RECHARGE_CHILD
} access_mode_t;

void     Accesos_Init(void);
void     Accesos_Task(uint32_t now_ms);
void     Accesos_SetMode(access_mode_t mode);
access_mode_t Accesos_GetMode(void);
void     Accesos_SetPendingCredits(uint8_t credits);
uint8_t  Accesos_GetResultMsg(char *buf, uint8_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* ACCESOS_H */
