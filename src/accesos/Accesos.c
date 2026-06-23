/*
 * Módulo: Accesos — implementación
 * Cada modo (access_mode_t) decide qué hacer con el UID leído.
 * Las aperturas son temporizadas y no bloqueantes (LED de puerta y servo del garaje).
 * Los cupos de juegos y la recarga se persisten en EEPROM.
 */
#include "Accesos.h"
#include "../rfid_rc522/rfid_rc522.h"
#include "../eeprom/eeprom.h"
#include "../gpio/gpio.h"
#include "../servo/servo_pwm.h"
#include "../uart/uart.h"

static access_mode_t current_mode;
static uint8_t       pending_credits;
static uint8_t       cur_uid_len;        /* longitud del UID en proceso (4/7/10) */

/* Flujo de recarga en dos pasos: 0=espera tarjeta PADRE, 1=espera tarjeta HIJO. */
static uint8_t recharge_step;

/* Temporizadores no bloqueantes de las salidas (LED puerta y servo garaje). */
static uint8_t  door_led_active;
static uint32_t door_led_tick;
static uint8_t  garage_active;
static uint32_t garage_tick;

/* Mensaje pendiente para mostrar en la UI tras una operación RFID. */
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
    for (uint8_t i = 0; i < cur_uid_len; i++) {
        UART_WriteDecimal(uid[i]);
        if (i + 1 < cur_uid_len) UART_WriteChar(' ');
    }
}

/* ----------------------------------------------------------
 *  Handlers por modo (privados)
 * ---------------------------------------------------------- */

/* Puerta principal: valida el UID y enciende el LED que simula la cerradura. */
static void handle_main_door(const uint8_t *uid, uint32_t now_ms) {
    uint8_t idx;
    if (!EEPROM_FindUserByUid(uid, cur_uid_len, &idx)) {
        UART_WriteEvent(SER_RFID, "Acceso puerta denegado: no registrado");
        set_result("No registrado");
        return;
    }
    /* Activa el LED que simula la cerradura de la puerta principal. */
    GPIO_WritePin(PIN_DOOR_LED, GPIO_HIGH);
    door_led_active = 1;
    door_led_tick = now_ms;
    UART_WriteString(SER_RFID "Acceso puerta autorizado: UID ");
    print_uid(uid);
    UART_Newline();
    set_result("Puerta OK");
}

/* Garaje: valida el UID y abre el servo; el cierre se temporiza en Accesos_Task. */
static void handle_garage(const uint8_t *uid, uint32_t now_ms) {
    uint8_t idx;
    if (!EEPROM_FindUserByUid(uid, cur_uid_len, &idx)) {
        UART_WriteEvent(SER_RFID, "Acceso garaje denegado: no registrado");
        set_result("No registrado");
        return;
    }
    ServoPwm_Open();
    garage_active = 1;
    garage_tick = now_ms;
    UART_WriteString(SER_RFID "Garaje abierto: UID ");
    print_uid(uid);
    UART_Newline();
    set_result("Garaje OK");
}

/* Sala de juegos: solo hijos, descuenta un cupo y lo guarda en EEPROM. */
static void handle_game_room(const uint8_t *uid) {
    uint8_t idx;
    user_record_t u;

    if (!EEPROM_FindUserByUid(uid, cur_uid_len, &idx)) {
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

/* Enrola un UID nuevo como PADRE o HIJO en el primer slot libre. */
static void handle_enroll(const uint8_t *uid, user_type_t type) {
    uint8_t idx;
    if (EEPROM_FindUserByUid(uid, cur_uid_len, &idx)) {
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
    u.uid_len = cur_uid_len;
    for (uint8_t i = 0; i < cur_uid_len; i++) u.uid[i] = uid[i];
    for (uint8_t i = cur_uid_len; i < RFID_UID_MAX; i++) u.uid[i] = 0;
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

/* Borra un usuario existente por su UID. */
static void handle_delete(const uint8_t *uid) {
    uint8_t idx;
    if (!EEPROM_FindUserByUid(uid, cur_uid_len, &idx)) {
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

/* Recarga de cupos en dos pasos: primero autentica PADRE, luego suma a un HIJO.
 * Valida rol en cada paso para evitar que un hijo se autoricarga. */
static void handle_recharge(const uint8_t *uid) {
    if (recharge_step == 0) {
        /* Paso 1: espera tarjeta PADRE que autoriza la recarga. */
        uint8_t idx;
        user_record_t u;
        if (!EEPROM_FindUserByUid(uid, cur_uid_len, &idx) || !EEPROM_LoadUser(idx, &u)) {
            UART_WriteEvent(SER_RFID, "Recarga: tarjeta no registrada");
            set_result("No registrado");
            return;
        }
        if (u.type != USER_PARENT) {
            UART_WriteEvent(SER_RFID, "Recarga: se requiere PADRE");
            set_result("Se requiere PADRE");
            return;
        }
        recharge_step = 1;
        UART_WriteEvent(SER_RFID, "Padre autenticado. Acerque HIJO");
        set_result("Acerque HIJO");
        return;
    }

    if (recharge_step == 1) {
        /* Paso 2: espera tarjeta HIJO y suma pending_credits (tope 255). */
        uint8_t idx;
        user_record_t u;
        if (!EEPROM_FindUserByUid(uid, cur_uid_len, &idx) || !EEPROM_LoadUser(idx, &u)) {
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
 *  API pública
 * ---------------------------------------------------------- */

void Accesos_Init(void) {
    current_mode  = ACCESS_MODE_NORMAL;
    pending_credits = 0;
    cur_uid_len   = 0;
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
    /* --- Temporizadores no bloqueantes de las salidas --- */
    /* LED de puerta: se apaga solo a los 2 s de la apertura. */
    if (door_led_active && (uint32_t)(now_ms - door_led_tick) >= 2000U) {
        GPIO_WritePin(PIN_DOOR_LED, GPIO_LOW);
        door_led_active = 0;
    }
    /* Garaje: cierra el servo a los 3 s de la apertura. */
    if (garage_active && (uint32_t)(now_ms - garage_tick) >= 3000U) {
        ServoPwm_Close();
        garage_active = 0;
    }

    /* --- Consume el UID si el driver dejó uno listo --- */
    if (!RFID_UIDAvailable()) return;

    uint8_t uid[RFID_UID_MAX];
    uint8_t uid_len = 0;
    rfid_status_t status = RFID_ReadUIDEx(uid, &uid_len);
    if (status != RFID_OK) {
        if (status != RFID_NO_CARD) {
            UART_WriteEvent(SER_RFID, "UID rechazada: error de lectura");
        }
        return;
    }
    /* Validación de longitud de UID (4/7/10 bytes ISO14443A). */
    if (uid_len < RFID_UID_LEN || uid_len > RFID_UID_MAX) {
        UART_WriteEvent(SER_RFID, "UID rechazada: longitud no soportada");
        set_result("UID invalida");
        return;
    }
    cur_uid_len = uid_len;

    UART_WriteString(SER_RFID "UID recibido: ");
    print_uid(uid);
    UART_Newline();

    switch (current_mode) {
        case ACCESS_MODE_NORMAL:
        case ACCESS_MODE_MAIN_DOOR:
            handle_main_door(uid, now_ms);
            if (current_mode != ACCESS_MODE_NORMAL) current_mode = ACCESS_MODE_NORMAL;
            break;

        case ACCESS_MODE_GARAGE:
            handle_garage(uid, now_ms);
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
    door_led_tick = 0;
    garage_tick = 0;
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
