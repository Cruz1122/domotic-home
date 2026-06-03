# Asignación de módulos y reglas de trabajo

Este documento define la separación de responsabilidades, las restricciones técnicas y las reglas de integración para el firmware `domotic-home`.

El objetivo es que Eduardo y Camilo puedan avanzar en paralelo sin pisarse archivos, sin romper contratos compartidos y sin introducir dependencias que después obliguen a reescribir medio proyecto.

## Plataforma y lenguaje obligatorio

| Restricción | Definición |
|-------------|------------|
| Microcontrolador | ATmega2560 / Arduino Mega 2560 |
| Lenguaje | C puro |
| Archivos de módulos | `.c` y `.h` |
| Orquestador | `.ino` mínimo, solo para `setup()` y `loop()` |
| Arquitectura | Máquina de estados cooperativa |
| Control de tiempo | Por ticks/timers, nunca con `delay()` |
| Librerías externas | Prohibidas |
| Persistencia | EEPROM interna, mediante driver propio |

## Asignación de módulos

| Carpeta | Archivos | Responsable | Propósito |
|---------|----------|-------------|-----------|
| `src/common/` | `Definiciones.h` | Eduardo + Camilo, solo con acuerdo | Pines, tipos, macros, constantes compartidas |
| `src/seguridad/` | `Seguridad.h`, `Seguridad.c` | Eduardo | PIR, MQ-2, alarmas, eventos críticos |
| `src/accesos/` | `Accesos.h`, `Accesos.c` | Camilo | RFID, usuarios, puerta, garaje, sala de juegos |
| `src/confort/` | `Confort.h`, `Confort.c` | Eduardo | Iluminación, temperatura, ventilación, calefacción, sonido |
| `src/remoto/` | `Remoto.h`, `Remoto.c` | Camilo | Horno, lista de mercado, servicios simulados |
| `src/ui/` | `UI.h`, `UI.c` | Camilo | LCD, menús, teclado matricial, navegación |
| raíz / `src/app/` | `domotic-home.ino` o `Proyecto_Domotica.ino` | Integración conjunta | Orquestador mínimo de `setup()` y `loop()` |

## Regla principal de separación

Cada responsable trabaja únicamente dentro de su carpeta asignada.

Un módulo no debe modificar directamente archivos de otro módulo. Si necesita una función, constante, evento o dato de otro subsistema, debe solicitar que se exponga en el `.h` correspondiente o que se agregue una constante compartida en `Definiciones.h`.

La integración se hace por contratos, no editando el código del otro.

## Reglas para `Definiciones.h`

1. No modificar `Definiciones.h` sin avisar al equipo.
2. Todo cambio en pines, tipos, macros, constantes globales o códigos de eventos afecta a varios módulos.
3. No duplicar constantes locales si ya existe una constante compartida.
4. No poner lógica en `Definiciones.h`.
5. No declarar variables globales con almacenamiento real en `Definiciones.h`.
6. Solo se permiten:
   - `#define`
   - `typedef`
   - `enum`
   - `struct`
   - prototipos estrictamente compartidos si son necesarios
   - macros seriales `SER_*`
   - constantes de pines
   - constantes de tamaños, límites y tiempos

Ejemplo permitido:

```c
#define SER_ALARMA "[ALARMA] "
#define TIEMPO_SERVO_GARAJE_MS 3000UL
#define PIN_LED_PUERTA 22
```

Ejemplo prohibido:

```c
uint8_t estado_alarma = 0;  /* Prohibido en .h */
```

## Restricciones de C puro

El proyecto debe escribirse como C procedural. Aunque el `.ino` sea compilado por el entorno Arduino, los módulos del proyecto deben mantenerse en C puro.

Queda prohibido usar:

- Clases de C++.
- Objetos globales tipo `LiquidCrystal lcd(...)`, `Servo servo`, `MFRC522 rfid`, etc.
- `String` de Arduino.
- `new` / `delete`.
- STL.
- Templates.
- Lambdas.
- Sobrecarga de funciones.
- Herencia.
- Excepciones.
- `.cpp` para módulos del proyecto.
- Código funcional dentro de constructores u objetos globales.

Se debe usar:

- `stdint.h` para tipos enteros.
- `stdbool.h` si se requiere `bool` en C.
- `uint8_t`, `uint16_t`, `uint32_t`, `int16_t`, etc.
- Funciones con contratos claros en `.h`.
- Variables `static` internas dentro del `.c` cuando el estado no deba salir del módulo.
- `enum` para estados.
- `struct` para datos agrupados.

Ejemplo recomendado:

```c
void Seguridad_Init(void);
void Seguridad_Task(uint32_t now_ms);
```

Ejemplo no permitido:

```cpp
class Seguridad {
public:
  void update();
};
```

## Enlace C/C++ (`extern "C"`)

El entorno Arduino compila los archivos `.ino` como C++ y los archivos `.c` como C. Esto causa que los nombres de función tengan representación interna distinta (mangling), generando errores de `undefined reference` en el enlazador.

Para evitarlo, todo archivo `.h` de módulo debe envolver sus prototipos con el bloque `extern "C"` condicional:

```c
#ifdef __cplusplus
extern "C" {
#endif

void Modulo_Init(void);
void Modulo_Task(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
```

Esto garantiza que el compilador C++ del `.ino` busque símbolos con linkage C, que son los que generan los archivos `.c`.

Esta regla aplica a todos los `.h` bajo `src/*/`. No aplica a `Definiciones.h` porque solo contiene macros, tipos y constantes, nunca funciones.

## Librerías externas prohibidas

No se permite usar librerías externas ni abstracciones de Arduino para los periféricos principales.

Prohibidas explícitamente:

- `MFRC522`
- `LiquidCrystal`
- `Servo`
- `Keypad`
- `EEPROM`
- `SPI`
- librerías externas de timers
- librerías externas de menús
- librerías externas de sensores

Permitido únicamente si es implementación propia del equipo:

- `uart.c` / `uart.h`
- `spi.c` / `spi.h`
- `lcd.c` / `lcd.h`
- `keypad.c` / `keypad.h`
- `adc.c` / `adc.h`
- `pwm.c` / `pwm.h`
- `timer.c` / `timer.h`
- `eeprom.c` / `eeprom.h`
- `rfid_rc522.c` / `rfid_rc522.h`
- `servo_pwm.c` / `servo_pwm.h`

Si un driver todavía no existe, no se debe incluir como si ya existiera. Primero se crea su issue, contrato `.h` y versión mínima funcional.

## Regla absoluta de tiempo: no `delay()`

No se puede usar `delay()` en ninguna parte del proyecto.

Tampoco se permiten reemplazos bloqueantes disfrazados, por ejemplo:

```c
while (Timer_GetMs() - inicio < 3000) {
  /* Esperar */
}
```

También están prohibidos:

- Bucles esperando una tecla.
- Bucles esperando una tarjeta RFID.
- Bucles esperando que termine el servo.
- Bucles esperando que termine el horno.
- Bucles esperando actualización del LCD.
- Esperas activas por ADC, UART, SPI o EEPROM.

Todo control de tiempo debe implementarse con ticks y máquina de estados.

Ejemplo correcto:

```c
static uint8_t garage_abierto = 0;
static uint32_t garage_t_inicio = 0;

void Accesos_AbrirGaraje(uint32_t now_ms)
{
    garage_abierto = 1;
    garage_t_inicio = now_ms;
    ServoPwm_SetAngulo(90);
}

void Accesos_Task(uint32_t now_ms)
{
    if (garage_abierto && ((now_ms - garage_t_inicio) >= TIEMPO_SERVO_GARAJE_MS)) {
        ServoPwm_SetAngulo(0);
        garage_abierto = 0;
    }
}
```

## Firma obligatoria de módulos

Cada módulo funcional debe exponer, como mínimo, una función de inicialización y una función cooperativa de tarea.

Formato recomendado:

```c
void Modulo_Init(void);
void Modulo_Task(uint32_t now_ms);
```

Aplicado al proyecto:

```c
void Seguridad_Init(void);
void Seguridad_Task(uint32_t now_ms);

void Accesos_Init(void);
void Accesos_Task(uint32_t now_ms);

void Confort_Init(void);
void Confort_Task(uint32_t now_ms);

void Remoto_Init(void);
void Remoto_Task(uint32_t now_ms);

void UI_Init(void);
void UI_Task(uint32_t now_ms);
```

Ninguna función `Task` puede bloquear. Cada una debe ejecutar rápido y devolver el control al `loop()`.

## Orquestador principal

El `.ino` no debe contener lógica de negocio. Solo debe inicializar módulos, obtener el tick actual y llamar tareas.

Ejemplo esperado:

```c
#include "src/common/Definiciones.h"
#include "src/seguridad/Seguridad.h"
#include "src/accesos/Accesos.h"
#include "src/confort/Confort.h"
#include "src/remoto/Remoto.h"
#include "src/ui/UI.h"

void setup(void)
{
    Drivers_Init();
    Seguridad_Init();
    Accesos_Init();
    Confort_Init();
    Remoto_Init();
    UI_Init();
}

void loop(void)
{
    uint32_t now_ms = Timer_GetMs();

    Keypad_Task(now_ms);
    Rfid_Task(now_ms);
    Seguridad_Task(now_ms);
    Accesos_Task(now_ms);
    Confort_Task(now_ms);
    Remoto_Task(now_ms);
    UI_Task(now_ms);
    Serial_Task(now_ms);
}
```

## Restricciones por módulo

### `src/seguridad/`

Responsable: Eduardo.

Incluye:

- PIR 1.
- PIR 2.
- MQ-2 por ADC.
- Activación/desactivación de alarma de acceso.
- Activación/desactivación de alarma de incendio.
- Estados de alarma.
- Eventos seriales críticos.
- Notificación de alertas hacia UI.

Restricciones:

- No leer teclado directamente desde `Seguridad.c`.
- No escribir directamente en LCD desde `Seguridad.c`, salvo que el contrato final lo permita explícitamente.
- No usar `delay()` para antirrebote, ventanas de alarma o silenciamiento.
- No manejar lógica de usuarios RFID.
- No acceder directamente a EEPROM salvo que exista contrato compartido.

### `src/accesos/`

Responsable: Camilo.

Incluye:

- Lectura lógica de UID RFID.
- Validación de usuarios.
- Enrolamiento de tarjetas.
- Borrado lógico de tarjetas.
- Roles padre/administrador e hijo.
- Apertura de puerta principal simulada con LED.
- Apertura/cierre de garaje con servomotor.
- Sala de juegos.
- Control y descuento de cupos.
- Recarga de cupos por administrador.
- Persistencia lógica de usuarios y cupos.

Restricciones:

- No usar librería `MFRC522`.
- No usar librería `Servo`.
- No bloquear esperando una tarjeta.
- No bloquear esperando movimiento del servo.
- No modificar pantallas directamente si la UI ya expone un contrato.
- No modificar constantes de EEPROM sin coordinar con el equipo.

### `src/confort/`

Responsable: Eduardo.

Incluye:

- Dimmer de iluminación mediante LED PWM.
- Lectura de potenciómetro por ADC.
- Control de temperatura simulado desde teclado/UI.
- Calefacción representada por LED o relé.
- Ventilación representada por LED o relé.
- Equipo de sonido.
- Volumen por potenciómetro.
- Porcentaje de volumen calculado en tiempo real.
- Salida PWM proporcional al volumen.

Restricciones:

- No leer teclado directamente para menús; la UI debe entregar eventos o valores.
- No escribir pantallas completas desde `Confort.c`.
- No bloquear lecturas ADC.
- No usar `analogRead()` como dependencia final si existe driver ADC propio.
- No usar `analogWrite()` como dependencia final si existe driver PWM propio.
- No usar `delay()` para transiciones de temperatura, sonido o dimmer.

### `src/remoto/`

Responsable: Camilo.

Incluye:

- Horno configurable por teclado/UI.
- Temperatura del horno.
- Tiempo del horno.
- Cuenta regresiva no bloqueante.
- Apagado automático al terminar el tiempo.
- Lista de mercado.
- Consulta de productos.
- Cantidades configurables.

Restricciones:

- El horno nunca puede detener el sistema mientras cuenta el tiempo.
- No usar `delay()` para la cuenta regresiva.
- No implementar menús propios por fuera de `UI.c`.
- No mezclar lógica de mercado con accesos RFID.
- No usar memoria dinámica para listas.
- La lista de mercado debe tener tamaño máximo definido en `Definiciones.h`.

### `src/ui/`

Responsable: Camilo.

Incluye:

- LCD.
- Teclado matricial.
- Menú principal.
- Menús de seguridad, accesos, confort y servicios.
- Captura de códigos.
- Captura de números.
- Mensajes de estado.
- Pantallas de alerta.
- Priorización de alertas sobre pantallas normales.

Restricciones:

- No usar `LiquidCrystal`.
- No usar `Keypad`.
- No bloquear esperando entrada del usuario.
- No hacer `while` hasta que el usuario presione una tecla.
- No controlar directamente hardware de seguridad, accesos, confort o remoto.
- La UI debe invocar contratos públicos de los módulos, no tocar sus variables internas.
- La UI debe poder refrescar volumen, horno y alertas sin congelar el resto del sistema.

## Comunicación entre módulos

La comunicación entre módulos debe hacerse por funciones públicas, eventos simples o estructuras compartidas bien definidas.

Permitido:

```c
Seguridad_SetAlarmaAccesoActiva(true);
Accesos_SolicitarAperturaGaraje();
Confort_SetTemperaturaObjetivo(24);
Remoto_ConfigurarHorno(180, 120);
```

Prohibido:

```c
extern uint8_t estado_alarma;
estado_alarma = 1;
```

Los estados internos deben permanecer privados dentro del `.c` correspondiente mediante variables `static`.

## Eventos seriales

Todos los eventos seriales deben usar las macros `SER_*` definidas en `Definiciones.h`.

Ejemplo:

```c
printf(SER_ALARMA "Intrusion PIR1\n");
printf(SER_ACCESO "Tarjeta autorizada\n");
printf(SER_HORNO "Horno finalizado\n");
```

Eventos mínimos a reportar:

- Alarma de intrusión activada.
- PIR 1 detectado.
- PIR 2 detectado.
- MQ-2 sobre umbral.
- Alarma de incendio activada.
- Tarjeta RFID autorizada.
- Tarjeta RFID rechazada.
- Usuario enrolado.
- Usuario borrado.
- Puerta principal abierta/cerrada.
- Garaje abierto/cerrado.
- Cupo de juegos descontado.
- Cupos recargados.
- Horno iniciado.
- Horno finalizado.
- Error de EEPROM o datos inválidos.

## EEPROM

La EEPROM debe manejarse con driver propio y modelo de datos definido.

Reglas:

1. No usar librería `EEPROM` de Arduino.
2. No escribir EEPROM en cada iteración del `loop()`.
3. No escribir EEPROM por cambios temporales.
4. Solo escribir cuando haya cambios persistentes reales:
   - enrolar tarjeta
   - borrar tarjeta
   - descontar cupo
   - recargar cupos
   - actualizar lista de mercado si se decide persistirla
5. Usar cabecera de validez.
6. Usar versión de estructura.
7. Usar checksum simple si el tiempo lo permite.
8. Definir límites máximos en `Definiciones.h`.

Ejemplo de cabecera:

```c
#define EEPROM_MAGIC 0xD0
#define EEPROM_VERSION 1
```

## LCD y teclado

Todo lo que se maneje por teclado matricial debe tener una UI visible en LCD.

Ejemplo de menú principal:

```txt
A Seg B RFID
C Amb D Serv
```

Ejemplo de submenú:

```txt
A Act Alarma
B Desactivar
```

Reglas:

- Cada pantalla debe tener una salida clara con `*`.
- Cada confirmación debe usar `#`.
- La captura numérica debe tener longitud máxima.
- No se aceptan entradas infinitas.
- El LCD no debe refrescarse innecesariamente en cada ciclo si no cambió el contenido.
- Las alertas críticas tienen prioridad sobre pantallas normales.
- El volumen del equipo de sonido debe mostrarse en porcentaje en tiempo real cuando la pantalla de sonido esté activa.

## Simulaciones aceptadas

| Requisito | Implementación aceptada |
|----------|--------------------------|
| Imán/cerradura de puerta | LED controlado por salida digital |
| Garaje | Servomotor real |
| Dimmer | LED PWM controlado por potenciómetro |
| Calefacción | LED o relé |
| Ventilación | LED o relé |
| Horno | LED/relé + estado en LCD + temporizador no bloqueante |
| Equipo de sonido | PWM/LED/buzzer según disponibilidad + volumen por potenciómetro |
| Lista de mercado | Datos locales desde UI LCD |
| Servicios remotos | Simulados localmente desde menú LCD + teclado |

## Reglas de commits

1. Un commit debe tocar solo archivos del módulo del responsable.
2. Si un commit toca más de una carpeta, debe justificarlo en el mensaje.
3. Cambios en `Definiciones.h` deben ir en commits pequeños y revisables.
4. No mezclar refactor con funcionalidad.
5. No meter drivers incompletos como dependencia de módulos terminados.
6. No subir código que compile solo en el computador de una persona.
7. No subir archivos generados, temporales o basura del IDE.

## Criterios de aceptación por módulo

Un módulo se considera aceptable cuando:

- Compila sin depender de librerías externas prohibidas.
- Expone `.h` claro y mínimo.
- Mantiene estado interno privado.
- No usa `delay()`.
- No contiene bucles bloqueantes.
- Puede ejecutarse repetidamente desde `loop()`.
- Reporta eventos relevantes por serial si aplica.
- No toca archivos fuera de su carpeta.
- No rompe contratos de otros módulos.

## Regla final

Si algo necesita esperar, debe convertirse en estado + timestamp + condición de salida.

Si algo necesita comunicarse con otro módulo, debe pasar por contrato público.

Si algo necesita compartirse entre todos, debe discutirse antes de tocar `Definiciones.h`.

Si algo parece rápido de resolver con `delay()`, está mal diseñado para este proyecto.
