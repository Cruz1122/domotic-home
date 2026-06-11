#include "Accesos.h"
#include "../rfid_rc522/rfid_rc522.h"
#include "../eeprom/eeprom.h"
#include "../gpio/gpio.h"
#include "../servo/servo_pwm.h"
#include "../uart/uart.h"
#include "../timer/timer.h"

static access_mode_t current_mode;
static uint8_t       pending_credits;

/* Recharge internal two-card flow */
static uint8_t recharge_step;                /* 0=wait parent, 1=wait child */
static uint8_t recharge_parent_uid[RFID_UID_LEN];

/* Non-blocking output timers */
static uint8_t  door_led_active;
static uint32_t door_led_tick;
static uint8_t  garage_active;
static uint32_t garage_tick;

/* Pending message for UI */
static char    result_msg[17];
static uint8_t result_pending;

/* ----------------------------------------------------------
 *  Internal helpers
 * ---------------------------------------------------------- */

static void set_result(const char *msg) {
    uint8_t i = 0;
    while (*msg && i < 16) {
        result_msg[i++] = *msg++;
    }
    result_msg[i] = '\0';
    result_pending = 1;
}

static void print_uid(const uint8_t *uid) {
    for (uint8_t i = 0; i < RFID_UID_LEN; i++) {
        UART_WriteDecimal(uid[i]);
        if (i < RFID_UID_LEN - 1) UART_WriteChar(' ');
    }
}

/* ----------------------------------------------------------
 *  Mode handlers  (private)
 * ---------------------------------------------------------- */

static void handle_main_door(const uint8_t *uid) {
    uint8_t idx;
    if (!EEPROM_FindUserByUid(uid, &idx)) {
        UART_WriteEvent(SER_RFID, "Acceso puerta denegado: no registrado");
        set_result("No registrado");
        return;
    }
    GPIO_WritePin(PIN_DOOR_LED, GPIO_HIGH);
    door_led_active = 1;
    door_led_tick = Timer_GetMs();
    UART_WriteString(SER_RFID "Acceso puerta autorizado: UID ");
    print_uid(uid);
    UART_Newline();
    set_result("Puerta OK");
}

static void handle_garage(const uint8_t *uid) {
    uint8_t idx;
    if (!EEPROM_FindUserByUid(uid, &idx)) {
        UART_WriteEvent(SER_RFID, "Acceso garaje denegado: no registrado");
        set_result("No registrado");
        return;
    }
    ServoPwm_Open();
    garage_active = 1;
    garage_tick = Timer_GetMs();
    UART_WriteString(SER_RFID "Garaje abierto: UID ");
    print_uid(uid);
    UART_Newline();
    set_result("Garaje OK");
}

static void handle_game_room(const uint8_t *uid) {
    uint8_t idx;
    user_record_t u;

    if (!EEPROM_FindUserByUid(uid, &idx)) {
        UART_WriteEvent(SER_JUEGOS, "Acceso denegado: tarjeta no registrada");
        set_result("No registrado");
        return;
    }
    if (!EEPROM_LoadUser(idx, &u)) {
        UART_WriteEvent(SER_JUEGOS, "Acceso denegado: error lectura");
        set_result("Error EEPROM");
        return;
    }
    if (u.type != USER_CHILD) {
        UART_WriteEvent(SER_JUEGOS, "Usuario no es hijo");
        set_result("No es hijo");
        return;
    }
    if (u.game_credits == 0) {
        UART_WriteEvent(SER_JUEGOS, "Sin cupos disponibles");
        set_result("Sin cupos");
        return;
    }
    u.game_credits--;
    EEPROM_UpdateGameCredits(idx, u.game_credits);
    UART_WriteString(SER_JUEGOS "Ingreso autorizado. Restantes: ");
    UART_WriteDecimal(u.game_credits);
    UART_Newline();
    set_result("Juegos OK");
}

static void handle_enroll(const uint8_t *uid, user_type_t type) {
    uint8_t idx;
    if (EEPROM_FindUserByUid(uid, &idx)) {
        UART_WriteEvent(SER_RFID, "UID ya existe, enrolamiento rechazado");
        set_result("UID ya existe");
        return;
    }
    if (!EEPROM_FindFreeSlot(&idx)) {
        UART_WriteEvent(SER_EEPROM, "Sin slots libres");
        set_result("EEPROM llena");
        return;
    }
    user_record_t u;
    u.active = 1;
    for (uint8_t i = 0; i < RFID_UID_LEN; i++) u.uid[i] = uid[i];
    u.type = type;
    u.game_credits = 0;
    u.label[0] = '\0';
    EEPROM_SaveUser(idx, &u);
    UART_WriteString(SER_RFID "Enrolamiento OK: UID ");
    print_uid(uid);
    UART_WriteString(type == USER_PARENT ? " PADRE" : " HIJO");
    UART_Newline();
    set_result("Enrolado OK");
}

static void handle_delete(const uint8_t *uid) {
    uint8_t idx;
    if (!EEPROM_FindUserByUid(uid, &idx)) {
        UART_WriteEvent(SER_RFID, "UID no encontrado, no se borra");
        set_result("No existe");
        return;
    }
    EEPROM_DeleteUser(idx);
    UART_WriteString(SER_RFID "Usuario eliminado: UID ");
    print_uid(uid);
    UART_Newline();
    set_result("Borrado OK");
}

static void handle_recharge(const uint8_t *uid) {
    if (recharge_step == 0) {
        /* Step 1: wait for a PARENT card to authorise */
        uint8_t idx;
        user_record_t u;
        if (!EEPROM_FindUserByUid(uid, &idx) || !EEPROM_LoadUser(idx, &u)) {
            UART_WriteEvent(SER_RFID, "Recarga: tarjeta no registrada");
            set_result("No registrado");
            return;
        }
        if (u.type != USER_PARENT) {
            UART_WriteEvent(SER_RFID, "Recarga: se requiere PADRE");
            set_result("Se requiere PADRE");
            return;
        }
        for (uint8_t i = 0; i < RFID_UID_LEN; i++) {
            recharge_parent_uid[i] = uid[i];
        }
        recharge_step = 1;
        UART_WriteEvent(SER_RFID, "Padre autenticado. Acerque HIJO");
        set_result("Acerque HIJO");
        return;
    }

    if (recharge_step == 1) {
        uint8_t idx;
        user_record_t u;
        if (!EEPROM_FindUserByUid(uid, &idx) || !EEPROM_LoadUser(idx, &u)) {
            UART_WriteEvent(SER_RFID, "Recarga: tarjeta no registrada");
            set_result("No registrado");
            return;
        }
        if (u.type != USER_CHILD) {
            UART_WriteEvent(SER_RFID, "Recarga: se requiere HIJO");
            set_result("Se requiere HIJO");
            return;
        }
        uint16_t total = (uint16_t)u.game_credits + pending_credits;
        if (total > 255) total = 255;
        EEPROM_UpdateGameCredits(idx, (uint8_t)total);
        UART_WriteString(SER_RFID "Recarga OK: +");
        UART_WriteDecimal(pending_credits);
        UART_WriteString(" cupos, total: ");
        UART_WriteDecimal(total);
        UART_Newline();
        set_result("Recarga OK");
        recharge_step = 0;
        current_mode = ACCESS_MODE_NORMAL;
    }
}

/* ----------------------------------------------------------
 *  Public API
 * ---------------------------------------------------------- */

void Accesos_Init(void) {
    current_mode  = ACCESS_MODE_NORMAL;
    pending_credits = 0;
    recharge_step = 0;
    door_led_active  = 0;
    garage_active    = 0;
    result_pending   = 0;
    result_msg[0]    = '\0';

    GPIO_SetPinMode(PIN_DOOR_LED, GPIO_OUT);
    GPIO_WritePin(PIN_DOOR_LED, GPIO_LOW);

    UART_WriteEvent(SER_RFID, "Modulo accesos iniciado");
}

void Accesos_Task(uint32_t now_ms) {
    /* --- Non-blocking output timing --- */
    if (door_led_active && Timer_Expired(door_led_tick, 2000)) {
        GPIO_WritePin(PIN_DOOR_LED, GPIO_LOW);
        door_led_active = 0;
    }
    if (garage_active && Timer_Expired(garage_tick, 3000)) {
        ServoPwm_Close();
        garage_active = 0;
    }

    /* --- Consume UID if available --- */
    if (!RFID_UIDAvailable()) return;

    uint8_t uid[RFID_UID_LEN];
    if (!RFID_ReadUID(uid)) return;

    UART_WriteString(SER_RFID "UID recibido: ");
    print_uid(uid);
    UART_Newline();

    switch (current_mode) {
        case ACCESS_MODE_NORMAL:
        case ACCESS_MODE_MAIN_DOOR:
            handle_main_door(uid);
            if (current_mode != ACCESS_MODE_NORMAL) current_mode = ACCESS_MODE_NORMAL;
            break;

        case ACCESS_MODE_GARAGE:
            handle_garage(uid);
            current_mode = ACCESS_MODE_NORMAL;
            break;

        case ACCESS_MODE_GAME_ROOM:
            handle_game_room(uid);
            current_mode = ACCESS_MODE_NORMAL;
            break;

        case ACCESS_MODE_ENROLL_PARENT:
            handle_enroll(uid, USER_PARENT);
            current_mode = ACCESS_MODE_NORMAL;
            break;

        case ACCESS_MODE_ENROLL_CHILD:
            handle_enroll(uid, USER_CHILD);
            current_mode = ACCESS_MODE_NORMAL;
            break;

        case ACCESS_MODE_DELETE_USER:
            handle_delete(uid);
            current_mode = ACCESS_MODE_NORMAL;
            break;

        case ACCESS_MODE_RECHARGE_CHILD:
            handle_recharge(uid);
            break;

        default:
            current_mode = ACCESS_MODE_NORMAL;
            break;
    }
}

void Accesos_SetMode(access_mode_t mode) {
    current_mode  = mode;
    recharge_step = 0;
    result_pending = 0;
}

access_mode_t Accesos_GetMode(void) {
    return current_mode;
}

void Accesos_SetPendingCredits(uint8_t credits) {
    pending_credits = credits;
}

uint8_t Accesos_GetResultMsg(char *buf, uint8_t max_len) {
    if (!result_pending) return 0;
    uint8_t i = 0;
    while (result_msg[i] && i < max_len - 1) {
        buf[i] = result_msg[i];
        i++;
    }
    buf[i] = '\0';
    result_pending = 0;
    return 1;
}
