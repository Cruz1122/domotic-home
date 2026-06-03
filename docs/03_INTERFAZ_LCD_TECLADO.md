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
DOMOTIC HOME
Ingrese codigo:
```

Código correcto:

```txt
Acceso correcto
Cargando menu...
```

Código incorrecto:

```txt
Codigo invalido
Intentelo de nuevo
```

Evento serial:

```txt
[LOGIN] Codigo invalido
[LOGIN] Codigo correcto
```

## Menú principal

```txt
A Seg B RFID
C Amb D Serv
```

## Menú de seguridad

```txt
1 Acceso 2 Fuego
3 Estado * Volver
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
1 Luz 2 Temp
3 Sonido * Volver
```

### Iluminación

```txt
Luz PWM
Nivel: 68%
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
Sonido: ON
Volumen: 45%
```

El volumen se actualiza en tiempo real desde potenciómetro y debe reflejarse también en PWM.

## Menú servicios

```txt
1 Horno
2 Mercado
```

### Horno

Captura de temperatura:

```txt
Temp horno C:
___ #OK
```

Captura de tiempo:

```txt
Tiempo min:
__ #OK
```

Operación:

```txt
Horno 180C
Restan: 179s
```

Finalización:

```txt
Horno OFF
Tiempo final
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
| 1 | Arroz |
| 2 | Leche |
| 3 | Huevos |
| 4 | Pan |
| 5 | Azucar |
| 6 | Cafe |
| 7 | Aceite |
| 8 | Sal |

Agregar producto:

```txt
Prod: 1 Arroz
Cant: __ #OK
```

Consultar:

```txt
Arroz x2
Leche x1
```

## Errores de UI

| Caso | Mensaje LCD | Evento serial |
|---|---|---|
| Código incorrecto | `Codigo invalido` | `[LOGIN] Codigo invalido` |
| Tarjeta no registrada | `UID no valido` | `[RFID] Tarjeta no registrada` |
| EEPROM llena | `Memoria llena` | `[EEPROM] Sin slots` |
| Sin cupos de juego | `Sin cupos` | `[JUEGOS] Acceso denegado` |
| Horno con tiempo cero | `Tiempo invalido` | `[HORNO] Tiempo invalido` |
| Producto inválido | `Prod invalido` | `[MERCADO] Producto invalido` |
