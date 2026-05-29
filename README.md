<p align="center" style="font-size:2rem;"><strong>Domotic Home</strong></p>
<p align="center" style="font-size:1.25rem; margin-top:-1em;"><em>Scaffold segmentado para sistema domótico en enfoque C puro</em></p>

<p align="center">
  <strong>Plataforma objetivo</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Arduino-Mega_2560-555555?labelColor=00979D&logo=arduino&logoColor=white" alt="Arduino Mega 2560" />
  <img src="https://img.shields.io/badge/Lenguaje-C_puro-555555?labelColor=283593&logo=c&logoColor=white" alt="C puro" />
  <img src="https://img.shields.io/badge/Arquitectura-Modular-555555?labelColor=37474F" alt="Arquitectura modular" />
  <img src="https://img.shields.io/badge/Estado-Scaffold-555555?labelColor=6A1B9A" alt="Estado scaffold" />
</p>

<p align="center">
  <strong>Interfaces de vivienda</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/RFID-Control_de_acceso-555555?labelColor=1565C0" alt="RFID" />
  <img src="https://img.shields.io/badge/LCD-Salida_local-555555?labelColor=2E7D32" alt="LCD" />
  <img src="https://img.shields.io/badge/Serial-Telemetría_y_comandos-555555?labelColor=EF6C00" alt="Serial" />
  <img src="https://img.shields.io/badge/PWM-Luz_y_audio-555555?labelColor=5D4037" alt="PWM" />
</p>

<p align="center">
  <strong>Subsistemas esperados</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Seguridad-Alarmas_y_sensores-555555?labelColor=C62828" alt="Seguridad" />
  <img src="https://img.shields.io/badge/Accesos-RFID_y_puertas-555555?labelColor=283593" alt="Accesos" />
  <img src="https://img.shields.io/badge/Confort-Luz_y_temperatura-555555?labelColor=00838F" alt="Confort" />
  <img src="https://img.shields.io/badge/Remoto-Horno_sonido_mercado-555555?labelColor=6D4C41" alt="Remoto" />
</p>

## Decisión de arquitectura

Este repositorio quedó reiniciado como **scaffold segmentado**. El código previo fue vaciado por solicitud y ahora cada conjunto vive en una carpeta separada para reconstruir el sistema con **criterio procedural de C puro**: interfaz en encabezados, implementación en archivos `.c`, estado compartido centralizado y flujo principal aislado.

El objetivo funcional sigue siendo sistema domótico para vivienda con dos alarmas, RFID, control de accesos, climatización, iluminación dimerizada, horno remoto, sonido remoto, lista de mercado y salida local por LCD. Sin embargo, **estado actual del repositorio = solo estructura vacía**.

## Estructura segmentada

```txt
src/
  app/
    Proyecto_Domotica.ino    Orquestador principal vacío

  common/
    Definiciones.h           Contratos compartidos, pines, macros y estructuras

  seguridad/
    Seguridad.h             Interfaz de alarmas y sensores
    Seguridad.c             Implementación futura de seguridad

  accesos/
    Accesos.h               Interfaz de RFID, garaje y cuarto de juegos
    Accesos.c               Implementación futura de accesos

  confort/
    Confort.h               Interfaz de luz y climatización
    Confort.c               Implementación futura de confort

  remoto/
    Remoto.h                Interfaz de comandos remotos
    Remoto.c                Implementación futura de comandos remotos
```

## Requisitos de implementación futura

- Microcontrolador objetivo: **ATmega2560 / Arduino Mega 2560**
- Estilo obligatorio: **C puro**, modular y procedural
- Salida de diagnóstico: **puerto serial**
- Salida local al usuario: **LCD**
- Sensores esperados: humo, puertas, ventanas, selector de ubicación
- Actuadores esperados: alarma, imán de puerta principal, servomotor de garaje, calefactor, ventilador, iluminación PWM, salida analógica de sonido, horno remoto

## Estado actual

```txt
Scaffold vacío.
Sin lógica compilable.
Sin implementación funcional.
Sin documentación duplicada.
```

## Segmentos y funcionalidad esperada

### `src/app/`

**Rol:** punto de entrada del firmware.

**Archivo:**

- `Proyecto_Domotica.ino`

**Responsabilidad esperada en enfoque C puro:**

- Ejecutar `setup()` y `loop()` como coordinador mínimo.
- Invocar rutinas de inicialización de todos los módulos.
- Ejecutar ciclo principal sin bloquear.
- Coordinar refresco de LCD, lectura de sensores, comandos seriales y lectura RFID.

**Contrato esperado:**

```c
void setup(void);
void loop(void);
```

**Estado actual:** vacío.

---

### `src/common/`

**Rol:** concentrar contratos globales de bajo nivel.

**Archivo:**

- `Definiciones.h`

**Responsabilidad esperada en enfoque C puro:**

- Definir macros de pines.
- Definir constantes de umbral.
- Declarar estructuras de datos compartidas.
- Declarar variables globales controladas por módulos.

**Contrato esperado:**

```c
#define PIN_VENTANA1       ...
#define PIN_VENTANA2       ...
#define PIN_PUERTA         ...
#define PIN_HUMO           ...
#define PIN_SERVO          ...
#define PIN_TEMP_POT       ...
#define PIN_LUZ_POT        ...

typedef struct {
    char uid[UID_MAX];
    char nombre[NOMBRE_MAX];
    unsigned char tipo;
    int accesos_juegos;
} Usuario;
```

**Estado actual:** vacío.

---

### `src/seguridad/`

**Rol:** manejar alarmas de acceso e incendio.

**Archivos:**

- `Seguridad.h`
- `Seguridad.c`

**Funcionalidad esperada:**

- Leer sensores de puertas y ventanas.
- Leer sensores de humo.
- Distinguir alarma de acceso vs alarma de incendio.
- Activar y desactivar alarmas solo con código válido.
- Reportar eventos críticos por puerto serial.
- Mantener estado seguro/inseguro del sistema.

**Interfaz esperada:**

```c
void seguridad_inicializar(void);
void seguridad_verificar_alarmas(void);
void seguridad_activar(unsigned int codigo);
void seguridad_desactivar(unsigned int codigo);
void seguridad_reportar_evento(const char *tipo);
```

**Estado actual:** vacío.

---

### `src/accesos/`

**Rol:** controlar autenticación RFID y acceso físico.

**Archivos:**

- `Accesos.h`
- `Accesos.c`

**Funcionalidad esperada:**

- Autorizar entrada por puerta principal y garaje.
- Permitir enrolamiento de nuevas tarjetas RFID.
- Permitir borrado de tarjetas existentes.
- Registrar padres e hijos con permisos distintos.
- Programar y descontar accesos a la habitación de juegos.
- Permitir a padres recargar accesos para hijos.
- Operar imán de puerta principal y servomotor de garaje.

**Interfaz esperada:**

```c
void accesos_inicializar(void);
unsigned char accesos_leer_selector_ubicacion(void);
void accesos_procesar_rfid(unsigned char ubicacion);
void accesos_enrolar_usuario(const Usuario *usuario);
void accesos_borrar_usuario(const char *uid);
void accesos_recargar_juegos(const char *uid, int cantidad);
```

**Estado actual:** vacío.

---

### `src/confort/`

**Rol:** controlar ambiente interior.

**Archivos:**

- `Confort.h`
- `Confort.c`

**Funcionalidad esperada:**

- Leer referencia de temperatura.
- Activar calefactor si temperatura cae por debajo del umbral.
- Activar ventilador si temperatura supera el umbral.
- Realizar control dimerizado de iluminación.
- Exponer estado actual al LCD.

**Interfaz esperada:**

```c
void confort_inicializar(void);
void confort_actualizar_clima(void);
void confort_actualizar_iluminacion(void);
int confort_obtener_temperatura(void);
unsigned char confort_obtener_nivel_luz(void);
```

**Estado actual:** vacío.

---

### `src/remoto/`

**Rol:** procesar comandos remotos y servicios secundarios.

**Archivos:**

- `Remoto.h`
- `Remoto.c`

**Funcionalidad esperada:**

- Encender horno con tiempo y temperatura configurables.
- Encender equipo de sonido con volumen proporcional.
- Mantener lista de mercado remota.
- Permitir consulta remota de lista de mercado.
- Publicar respuestas y errores por serial.

**Interfaz esperada:**

```c
void remoto_inicializar(void);
void remoto_escuchar_serial(void);
void remoto_procesar_horno(const char *comando);
void remoto_procesar_sonido(const char *comando);
void remoto_gestionar_mercado(const char *comando);
```

**Estado actual:** vacío.

---

## Flujo funcional esperado

```txt
setup()
  -> seguridad_inicializar()
  -> accesos_inicializar()
  -> confort_inicializar()
  -> remoto_inicializar()

loop()
  -> seguridad_verificar_alarmas()
  -> remoto_escuchar_serial()
  -> accesos_procesar_rfid(ubicacion)
  -> confort_actualizar_clima()
  -> confort_actualizar_iluminacion()
  -> lcd_actualizar_estado()
```

## Mapa de responsabilidades

| Segmento | Responsabilidad principal | Entradas | Salidas |
|----------|---------------------------|----------|---------|
| `app` | Orquestación | todos los módulos | ciclo principal |
| `common` | Contratos comunes | requisitos del hardware | macros, structs, estado |
| `seguridad` | Alarmas y sensores | humo, puertas, ventanas, código | serial, alarma sonora |
| `accesos` | RFID y control físico | tarjetas, selector, permisos | imán, servo, saldo |
| `confort` | Ambiente | temperatura, nivel de luz | calefactor, ventilador, PWM |
| `remoto` | Servicios remotos | comandos seriales | horno, sonido, mercado |

## Siguiente implementación recomendada

1. Restaurar contratos mínimos en `Definiciones.h`.
2. Definir prototipos C reales en cada `.h`.
3. Implementar primero `seguridad` y `confort`.
4. Implementar luego `accesos` con persistencia en memoria estática.
5. Implementar finalmente `remoto` y salida a LCD.

## Documentación

Este `README.md` reemplaza documentación anterior. No quedan documentos funcionales paralelos en raíz.
