# Interfaz LCD y teclado matricial

## Principios de interfaz

La interfaz debe ser simple, consistente y demostrable. No se deben inventar combinaciones distintas de teclas para cada módulo. El usuario navega por menús, confirma con `#` y cancela con `*`.

## Mapa de teclas

| Tecla | Acción |
|---|---|
| `A` | Entrar a Seguridad |
| `B` | Entrar a Accesos RFID |
| `C` | Entrar a Ambiente |
| `D` | Entrar a Servicios |
| `#` | Confirmar / aceptar |
| `*` | Volver / cancelar |
| `0-9` | Captura numérica |

## Flujo de arranque

```txt
A Seg B RFID
C Amb D Serv
```

No existe login global al arranque. El código administrativo se pide solo al activar o desactivar alarmas.

## Menú principal

```txt
A Seg B RFID
C Amb D Serv
```

## Menú de seguridad

```txt
1Acc ON 2Fue
OFF *Volver
```

## Pantalla de código para alarmas

```txt
Ingrese codigo:
___ #OK *Vol
```

Si el código es incorrecto:

```txt
Codigo invalido
*Volver
```

Eventos seriales esperados:

```txt
[ALARMA ACCESO] Codigo valido
[ALARMA ACCESO] Codigo invalido
[ALARMA INCENDIO] Codigo valido
[ALARMA INCENDIO] Codigo invalido
```

### Alarma de acceso

```txt
Alarma acceso
1 ON 2 OFF
```

Si se dispara:

```txt
INTRUSION
PIR1 ACTIVO
```

### Alarma de incendio

```txt
Alarma fuego
1 ON 2 OFF
```

Si se dispara:

```txt
ALERTA HUMO
Nivel: 73%
```

## Menú RFID

```txt
1 Leer 2 Enrol
3 Borrar 4 Juegos
```

### Lectura de usuario

```txt
UID OK
Tipo: PADRE
```

```txt
UID OK
Hijo Cupos: 3
```

### Enrolamiento

```txt
Acerque tarjeta
Esperando...
```

Luego:

```txt
Tipo usuario
1 Padre 2 Hijo
```

Confirmación:

```txt
Usuario guardado
EEPROM OK
```

### Borrado

```txt
Acerque tarjeta
para borrar
```

Confirmación:

```txt
Usuario borrado
EEPROM OK
```

### Sala de juegos

```txt
Acerque hijo
para ingreso
```

Permitido:

```txt
Juegos OK
Restantes: 2
```

Denegado:

```txt
Sin cupos
Acceso negado
```

### Recarga de cupos

```txt
Cupos nuevos:
__ #OK
```

## Menú ambiente

```txt
1Temp 2Son
T:22C Luz 68%
```

El porcentaje se actualiza leyendo el potenciómetro asignado.

### Temperatura

```txt
Temp obj: 24C
1+ 2- #OK
```

Estado de actuadores:

```txt
Calefactor ON
Ventilador OFF
```

Como no se definió sensor físico de temperatura, este módulo es simulación de consigna y salidas.

### Sonido

```txt
Sonido ON
Vol 45% 1ON 2OFF
```

El volumen mostrado corresponde al último setpoint remoto recibido por `UART1` con `RADIO VOL <0-100>`.

## Menú servicios

```txt
1Radio 2Horno
3Merc *Vol
```

### Horno

La configuración del horno no se hace por LCD. El menú local solo informa que el control está disponible por `UART1 / Virtual Terminal`.

```txt
Horno
Pendiente
```

## Lista de mercado

```txt
1 Agregar
2 Consultar
3 Borrar
```

Productos predefinidos recomendados:

| Código | Producto |
|---:|---|
| 1 | Pan |
| 2 | Leche |
| 3 | Huevos |
| 4 | Arroz |
| 5 | Cafe |
| 6 | Azucar |
| 7 | Aceite |
| 8 | Fruta |

Agregar producto:

```txt
Prod: 1 Pan
Cant: __ #OK
```

Consultar:

```txt
Pan x2
Leche x1
```

## Errores de UI

| Caso | Mensaje LCD | Evento serial |
|---|---|---|
| Código incorrecto | `Codigo invalido` | `[ALARMA ACCESO] Codigo invalido` o `[ALARMA INCENDIO] Codigo invalido` |
| Tarjeta no registrada | `UID no valido` | `[RFID] Tarjeta no registrada` |
| EEPROM llena | `Memoria llena` | `[EEPROM] Sin slots` |
| Sin cupos de juego | `Sin cupos` | `[JUEGOS] Acceso denegado` |
| Horno con tiempo cero | `Tiempo invalido` | `[HORNO] Tiempo invalido` |
| Producto inválido | `Prod invalido` | `[MERCADO] Producto invalido` |
