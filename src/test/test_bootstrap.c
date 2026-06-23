/*
 * Módulo: Test bootstrap — implementación
 * Crea un usuario HIJO de prueba (UID 01020304, 10 cupos) si el slot 0 está vacío.
 * Útil para demos sin tener que enrolar a mano. No pisa usuarios existentes.
 */
#include "test_bootstrap.h"

#include "../common/Definiciones.h"
#include "../eeprom/eeprom.h"

#if DOMOTIC_HOME_ENABLE_DEMO_SEED
#include "../uart/uart.h"

void TestBootstrap_SeedDemoUser(void) {
    user_record_t user;

    /* Si ya hay un usuario en el slot 0, no hace nada (no pisa datos reales). */
    if (EEPROM_LoadUser(0, &user)) {
        return;
    }

    user.active = 1;
    user.uid_len = 4;
    user.uid[0] = 0x01;
    user.uid[1] = 0x02;
    user.uid[2] = 0x03;
    user.uid[3] = 0x04;
    for (uint8_t i = 4; i < RFID_UID_MAX; i++) user.uid[i] = 0;
    user.type = USER_CHILD;
    user.game_credits = 10;
    user.label[0] = '\0';

    EEPROM_SaveUser(0, &user);
    UART_WriteEvent(SER_EEPROM, "Demo seed cargado");
}
#else

void TestBootstrap_SeedDemoUser(void) {
}

#endif
