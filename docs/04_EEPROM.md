# Modelo de EEPROM

## Objetivo

La EEPROM almacenará usuarios RFID, roles y cupos de acceso a la habitación de juegos. También puede almacenar la lista de mercado si el equipo decide que debe persistir después del reinicio. La prioridad es persistir accesos RFID y cupos de hijos.

## Restricciones

La EEPROM es limitada y tiene ciclos de escritura finitos. No se debe escribir continuamente dentro del `loop()`. Solo escribir cuando cambie un dato: enrolar usuario, borrar usuario, recargar cupos, descontar cupo o modificar mercado.

## Layout final

```txt
EEPROM (4 KB — ATmega2560)
0x000  Header                    5 B
0x005-0x00F  Padding            11 B
0x010  Usuario 0                16 B
0x020  Usuario 1                16 B
0x030  Usuario 2                16 B
0x040  Usuario 3                16 B
0x050  Usuario 4                16 B
0x060  Usuario 5                16 B
0x070  Usuario 6                16 B
0x080  Usuario 7                16 B
0x090  Usuario 8                16 B
0x0A0  Usuario 9                16 B
0x0B0-0x0FF  Reservado          80 B
0x100  Lista de mercado (futuro)
```

## Header (dirección 0x000)

```c
#define EEPROM_MAGIC_0  'D'
#define EEPROM_MAGIC_1  'H'
#define EEPROM_VERSION  1

typedef struct {
    uint8_t magic0;       // 0x00  'D'
    uint8_t magic1;       // 0x01  'H'
    uint8_t version;      // 0x02
    uint8_t user_count;   // 0x03  contador de usuarios activos
    uint8_t checksum;     // 0x04  XOR de magic0 ^ magic1 ^ version ^ user_count
} eeprom_header_t;        // total 5 bytes
```

**Validación:**
- `magic0 == 'D'` y `magic1 == 'H'`
- `version == EEPROM_VERSION` (1)
- `checksum` calculado como `magic0 ^ magic1 ^ version ^ user_count`
- `user_count <= MAX_USERS` (10)

Si alguna condición falla, `EEPROM_Init()` llama a `EEPROM_Format()` y reporta `[EEPROM] Header invalido — EEPROM formateada`.

## Usuario RFID (desde 0x010, 16 B c/u)

```c
#define RFID_UID_LEN        5
#define MAX_USERS           10
#define USER_NAME_LEN       8

typedef enum {
    USER_EMPTY  = 0,
    USER_PARENT = 1,
    USER_CHILD  = 2
} user_type_t;

typedef struct {
    uint8_t active;                   // 0: libre, 1: ocupado
    uint8_t uid[RFID_UID_LEN];        // 5 bytes UID de la tarjeta
    user_type_t type;                 // rol
    uint8_t game_credits;             // cupos de juegos
    char label[USER_NAME_LEN];        // nombre (8 chars)
} user_record_t;
```

Tamaño real: `sizeof(user_record_t)` determinado por el compilador (≈16 B). Constante `EEPROM_USER_SIZE = 16` en el driver.

## Driver (`src/eeprom/eeprom.c`)

### API pública

| Función | Descripción |
|---------|-------------|
| `EEPROM_Init()` | Verifica header; si inválido, formatea |
| `EEPROM_Task()` | Hook (vacío—no se escribe en loop) |
| `EEPROM_ReadByte(addr)` | Retorna byte en `addr` |
| `EEPROM_WriteByte(addr, data)` | Escribe `data` en `addr` (espera segura + cli) |
| `EEPROM_ReadBlock(addr, buf, len)` | Lee `len` bytes hacia `buf` |
| `EEPROM_WriteBlock(addr, buf, len)` | Escribe `len` bytes desde `buf` |
| `EEPROM_IsValid()` | 1 si header intacto |
| `EEPROM_Format()` | Limpia tabla, escribe header fresco |
| `EEPROM_GetUserCount()` | Retorna número de usuarios activos |
| `EEPROM_LoadUser(index, &out)` | Lee usuario en índice, retorna 1 si activo |
| `EEPROM_SaveUser(index, &user)` | Escribe usuario; incrementa contador si es nuevo |
| `EEPROM_FindUserByUid(uid, &index)` | Busca por UID, retorna 1 si existe |
| `EEPROM_FindFreeSlot(&index)` | Busca slot con active=0 |
| `EEPROM_DeleteUser(index)` | Marca active=0, decrementa contador |
| `EEPROM_UpdateGameCredits(index, credits)` | Escribe un byte en `game_credits` |

### Escritura segura (por registros)

```c
void EEPROM_WriteByte(uint16_t addr, uint8_t data) {
    while (EECR & (1 << EEWE));              // esperar escritura anterior
    EEARH = (uint8_t)(addr >> 8);
    EEARL = (uint8_t)(addr);
    EEDR = data;
    uint8_t sreg = SREG;
    cli();                                    // ventana de 4 ciclos
    EECR |= (1 << EEMWE);
    EECR |= (1 << EEWE);
    SREG = sreg;                              // restaura estado de IRQ
}
```

### Checksum

```c
static uint8_t eeprom_calc_checksum(const eeprom_header_t *h) {
    return h->magic0 ^ h->magic1 ^ h->version ^ h->user_count;
}
```

### Política de borrado

No hace falta compactar la tabla. Para borrar un usuario, marcar `active = 0` (offset 0 del registro). El siguiente enrolamiento puede reutilizar ese slot vía `EEPROM_FindFreeSlot()`.

### Descuento de cupos

Secuencia correcta:

1. Leer UID.
2. Buscar usuario en EEPROM o caché RAM cargada desde EEPROM.
3. Validar que el usuario esté activo.
4. Validar que sea hijo.
5. Validar que `game_credits > 0`.
6. Llamar `EEPROM_UpdateGameCredits(index, credits - 1)`.
7. Reportar por LCD y serial.

### Prueba de persistencia

En `domotic-home.ino`, en el primer ciclo de `loop()`:

```c
user_record_t u;
if (EEPROM_LoadUser(0, &u)) {
    UART_WriteEvent(SER_EEPROM, "User 0 persistido OK");
} else {
    u.active = 1;
    u.uid[0] = 0x01; u.uid[1] = 0x02; u.uid[2] = 0x03;
    u.uid[3] = 0x04; u.uid[4] = 0x05;
    u.type = USER_CHILD;
    u.game_credits = 10;
    EEPROM_SaveUser(0, &u);
    UART_WriteEvent(SER_EEPROM, "User 0 escrito (prueba)");
}
```

Flujo de prueba:
1. Primer arranque → EEPROM vacía → format + escribe user 0.
2. Reinicio (simulado por reset) → lee user 0 → "persistido OK".

## Lista de mercado (futuro)

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
