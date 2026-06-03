# Modelo de EEPROM

## Objetivo

La EEPROM almacenará usuarios RFID, roles y cupos de acceso a la habitación de juegos. También puede almacenar la lista de mercado si el equipo decide que debe persistir después del reinicio. La prioridad es persistir accesos RFID y cupos de hijos.

## Restricciones

La EEPROM es limitada y tiene ciclos de escritura finitos. No se debe escribir continuamente dentro del `loop()`. Solo escribir cuando cambie un dato: enrolar usuario, borrar usuario, recargar cupos, descontar cupo o modificar mercado.

## Layout propuesto

```txt
EEPROM
0x000  Header
0x010  Tabla de usuarios RFID
0x100  Lista de mercado opcional
```

## Header

```c
#define EEPROM_MAGIC_0  'D'
#define EEPROM_MAGIC_1  'H'
#define EEPROM_VERSION  1

typedef struct {
    uint8_t magic0;
    uint8_t magic1;
    uint8_t version;
    uint8_t user_count;
    uint8_t checksum;
} eeprom_header_t;
```

Uso:

- Si `magic0 != 'D'` o `magic1 != 'H'`, inicializar EEPROM.
- Si `version` no coincide, reinicializar o migrar.
- `checksum` puede ser simple XOR si no quieren complicarse.

## Usuario RFID

```c
#define RFID_UID_LEN        5
#define MAX_USERS           10
#define USER_NAME_LEN       8

#define USER_EMPTY          0
#define USER_PARENT         1
#define USER_CHILD          2

typedef struct {
    uint8_t active;
    uint8_t uid[RFID_UID_LEN];
    uint8_t type;
    uint8_t game_credits;
    char label[USER_NAME_LEN];
} user_record_t;
```

Tamaño aproximado por usuario: 1 + 5 + 1 + 1 + 8 = 16 bytes. Con 10 usuarios se usan aproximadamente 160 bytes, muy aceptable para ATmega2560.

## Operaciones mínimas

```c
void eeprom_init_if_needed(void);
uint8_t eeprom_load_user(uint8_t index, user_record_t *out);
uint8_t eeprom_save_user(uint8_t index, const user_record_t *user);
uint8_t eeprom_find_user_by_uid(const uint8_t *uid, uint8_t *index_out);
uint8_t eeprom_find_free_user_slot(uint8_t *index_out);
uint8_t eeprom_delete_user(uint8_t index);
uint8_t eeprom_update_game_credits(uint8_t index, uint8_t credits);
```

## Política de borrado

No hace falta compactar la tabla. Para borrar un usuario, marcar `active = 0`. El siguiente enrolamiento puede reutilizar ese slot.

## Descuento de cupos

Secuencia correcta:

1. Leer UID.
2. Buscar usuario en EEPROM o caché RAM cargada desde EEPROM.
3. Validar que el usuario esté activo.
4. Validar que sea hijo.
5. Validar que `game_credits > 0`.
6. Restar 1.
7. Guardar nuevo valor en EEPROM.
8. Reportar por LCD y serial.

## Lista de mercado

Para evitar texto libre con teclado 4x4, se recomienda lista de productos predefinidos.

```c
#define MARKET_MAX_ITEMS 8

typedef struct {
    uint8_t product_id;
    uint8_t quantity;
} market_item_t;
```

Catálogo fijo en Flash/código:

```c
#define PROD_ARROZ   1
#define PROD_LECHE   2
#define PROD_HUEVOS  3
#define PROD_PAN     4
#define PROD_AZUCAR  5
#define PROD_CAFE    6
#define PROD_ACEITE  7
#define PROD_SAL     8
```

Si se agrega el mismo producto dos veces, se debe acumular cantidad en lugar de crear duplicado.

## Criterios de aceptación EEPROM

- Al enrolar una tarjeta, aparece después de reiniciar.
- Al borrar una tarjeta, sigue borrada después de reiniciar.
- Al descontar cupos de juegos, el nuevo saldo persiste.
- La EEPROM no se escribe continuamente mientras se muestra el LCD.
- Si la EEPROM está vacía o corrupta, el sistema inicializa estructura base y lo reporta por serial.
