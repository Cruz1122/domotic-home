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

Si se dispara la alarma de acceso:

```txt
INTRUSION
PIR1 ACTIVO
```

Si se dispara la alarma de incendio:

```txt
ALERTA HUMO
Nivel: 73%
```

## Menú RFID

```txt
1Pta 2Gar 3Jue
4Enr 5Bor 6Rec *V
```

### Enrolamiento

```txt
Enrolar:
1Padre 2Hijo *V
```

Luego:

```txt
Registrar PADRE
Acerque tarjeta
```

### Borrado

```txt
Borrar tarjeta
Acerque tarjeta
```

Confirmación:

```txt
Borrado OK
```

### Sala de juegos

```txt
Acerque tarjeta
* Cancelar
```

Permitido:

```txt
Juegos OK
```

Denegado:

```txt
Sin cupos
```

### Recarga de cupos

```txt
Cupos nuevos:
__ #OK
```

Luego el firmware pide primero una tarjeta de padre y después una tarjeta de hijo para aplicar la recarga.

## Menú ambiente

```txt
1Temp 2Son
T:22C Luz 68%
```

El porcentaje se actualiza leyendo el potenciómetro asignado.

### Temperatura

```txt
Temp obj 10-40
24 #OK *Vol
```

Estado de actuadores:

```txt
Calefactor ON
Ventilador OFF
```

Como no se definió sensor físico de temperatura, este módulo es simulación de consigna y salidas. En el firmware actual la temperatura objetivo se ingresa como valor numérico entre `10` y `40`.

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
1Agreg 2Ver
3Borrar *Vol
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
1/2 Pan
x2 <A D> *V
```

## Errores de UI

| Caso | Mensaje LCD | Evento serial |
|---|---|---|
| Código incorrecto | `Codigo invalido` | `[ALARMA ACCESO] Codigo invalido` o `[ALARMA INCENDIO] Codigo invalido` |
| Tarjeta no registrada | `No registrado` | `[RFID] ... no registrada` |
| EEPROM llena | `EEPROM llena` | `[EEPROM] Sin slots libres` |
| Sin cupos de juego | `Sin cupos` | `[JUEGOS] Acceso denegado` |
| Horno con tiempo cero | `Pendiente` | `[ERR][HORNO] Tiempo fuera de rango` |
| Producto inválido | `Prod: 1-8` | `[ERR][MERCADO] Producto invalido` |
