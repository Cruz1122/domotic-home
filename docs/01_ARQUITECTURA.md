# Arquitectura del firmware

## Principio central

El sistema debe funcionar como una máquina de estados cooperativa. Cada módulo expone funciones `init()` y `task()`; el orquestador las invoca de forma periódica. Ninguna tarea debe quedarse esperando indefinidamente una tecla, una tarjeta, un retardo de servo o una cuenta regresiva del horno.

La arquitectura correcta no es una cadena de `while` internos en cada menú. Eso rompería seguridad, RFID y temporización. La arquitectura correcta es un `loop()` breve que revisa entradas, actualiza estados y sale.

## Capas propuestas

```txt
Aplicación
  Proyecto_Domotica.ino

Servicios funcionales
  seguridad/      alarmas PIR y MQ-2
  accesos/        RFID, usuarios, puerta, garaje y juegos
  confort/        iluminación, temperatura y sonido
  remoto/         horno, mercado y comandos por UART1

Interfaz
  ui/             LCD, menús, captura de teclado

Drivers propios
  uart/           serial por registros
  spi/            bus SPI para RC522
  rfid_rc522/     comandos MFRC522 mínimos
  lcd/            LCD paralelo
  keypad/         teclado matricial 4x4
  adc/            potenciómetro de luz y MQ-2
  pwm/            LED dimmer, sonido, servo si aplica
  timer/          ticks del sistema
  eeprom/         persistencia

Common
  Definiciones.h  pines, constantes, tipos, estados
```

La estructura actual del repositorio ya separa `app`, `common`, `seguridad`, `accesos`, `confort` y `remoto`. Si se agregan drivers, deben vivir como carpetas nuevas bajo `src/`.

## Estados globales de aplicación

Estados globales mínimos:

```c
typedef enum {
    APP_BOOT = 0,
    APP_MAIN_MENU,
    APP_SECURITY_MENU,
    APP_RFID_MENU,
    APP_ENV_MENU,
    APP_SERVICES_MENU,
    APP_ALERT_SCREEN,
    APP_ERROR_SCREEN
} app_state_t;
```

## Estados de seguridad

```c
typedef struct {
    uint8_t access_enabled;
    uint8_t fire_enabled;
    uint8_t access_triggered;
    uint8_t fire_triggered;
    uint8_t pir1_active;
    uint8_t pir2_active;
    uint16_t smoke_adc;
    uint8_t smoke_percent;
} security_state_t;
```

## Estados de accesos

```c
typedef enum {
    USER_EMPTY = 0,
    USER_PARENT,
    USER_CHILD
} user_type_t;

typedef struct {
    uint8_t active;
    uint8_t uid[5];
    user_type_t type;
    uint8_t game_credits;
} user_record_t;
```

## Estados de servicios

```c
typedef struct {
    uint8_t enabled;
    uint16_t target_temp_c;
    uint32_t remaining_seconds;
    uint32_t end_tick_ms;
} oven_state_t;

typedef struct {
    uint8_t enabled;
    uint8_t volume_percent;
} sound_state_t;
```

## Ciclo principal recomendado

```c
void setup(void)
{
    drivers_init();
    seguridad_init();
    accesos_init();
    confort_init();
    servicios_init();
    ui_init();
}

void loop(void)
{
    timer_task();
    keypad_task();
    rfid_task();
    seguridad_task();
    accesos_task();
    confort_task();
    servicios_task();
    ui_task();
    serial_task();
}
```

## Regla de no bloqueo

Prohibido en lógica funcional:

```c
delay(5000);
while (tecla == 0) { }
while (!rfid_disponible()) { }
```

Reemplazo correcto:

```c
if (evento_pendiente && tick_actual >= tick_objetivo) {
    ejecutar_siguiente_paso();
}
```

## Eventos seriales mínimos

Formato recomendado:

```txt
[BOOT] Sistema iniciado
[ALARMA ACCESO] Codigo valido
[ALARMA INCENDIO] Codigo invalido
[ALARMA ACCESO] Intrusion detectada por PIR1
[ALARMA INCENDIO] Humo sobre umbral: 73%
[RFID] UID reconocido: tipo=HIJO cupos=2
[RFID] Acceso juegos permitido. Restantes=1
[RFID] Acceso denegado: tarjeta no registrada
[HORNO] Encendido: temp=180C tiempo=300s
[HORNO] Finalizado
[SONIDO] ON volumen=45%
[MERCADO] Item agregado: ARROZ x2
```

## Orden de integración

1. UART + LCD + teclado.
2. ADC + PWM.
3. PIR + MQ-2.
4. EEPROM.
5. SPI + RC522.
6. Accesos RFID.
7. Menús completos.
8. Servo/garaje.
9. Horno no bloqueante.
10. Mercado.

La integración RFID no debe dejarse para el final. El RC522 es el módulo con mayor riesgo porque exige SPI, temporización, lectura de registros y validación de UID sin librería externa.
