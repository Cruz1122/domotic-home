# Alcance cerrado y guion de demo

## Propósito

Este documento fija el alcance real del proyecto `domotic-home`. Su función es impedir que el sistema se convierta en una lista de requisitos imposibles de demostrar con el hardware disponible. La entrega debe verse como un sistema integrado de vivienda, no como pruebas aisladas de periféricos.

## Alcance cerrado

El sistema se implementará sobre ATmega2560 con drivers propios en C puro. La interfaz principal será LCD + teclado matricial 4x4. La telemetría y los eventos críticos se reportarán por UART/Serial. El RFID usa la librería externa MFRC522 (adaptador C++ en el repositorio).

El término "remoto" del enunciado se resolverá como operación por `UART1 / Virtual Terminal`. No se implementará WiFi, Bluetooth, Ethernet ni GSM salvo que se agregue hardware adicional y se actualice este documento.

## Implementación física vs simulada

| Función del enunciado | Implementación acordada |
|---|---|
| Alarma de acceso | PIR HC-SR501 x2 como sensores de presencia/intrusión |
| Alarma de incendio | MQ-2 leído por ADC con umbral configurable |
| Código de activación/desactivación | Constante compilada, digitada por teclado matricial |
| Acceso por RFID | RC522 físico |
| Enrolamiento/borrado de usuarios | Menú LCD/teclado + EEPROM |
| Puerta principal con imán | LED como simulación del electroimán/cerradura |
| Garaje | Servomotor físico |
| Habitación de juegos | Validación RFID + cupos en EEPROM + salida visual |
| Carga de accesos de hijos | Menú de administrador/padre |
| Iluminación dimerizada | LED PWM con nivel definido por potenciómetro |
| Temperatura ambiente | Control simulado desde teclado; calefactor/ventilador con LEDs o relé |
| Horno remoto | Configuración por `UART1`; LED/relé como actuador; temporización no bloqueante |
| Sonido remoto | ON/OFF y volumen por `UART1`; porcentaje en LCD; sin salida PWM física |
| Lista de mercado | Productos predefinidos + cantidad; consulta por `UART1` |

## Demo mínima obligatoria

La secuencia recomendada para la presentación es la siguiente:

1. **Arranque.** El LCD muestra directamente el menú principal.
2. **Código de seguridad.** Se entra al módulo de seguridad, se intenta activar una alarma con código incorrecto y se valida que no cambie el estado.
3. **Alarma de acceso.** Se activa desde menú de seguridad con código correcto. Luego se dispara un PIR o el pulsador de prueba (D45). El LCD muestra intrusión y el serial reporta el evento.
4. **Alarma de incendio.** Se activa desde menú con código correcto. Luego se dispara el MQ-2 sobre umbral o el pulsador de prueba (D44). El LCD muestra nivel de humo y el serial reporta alarma de incendio.
5. **RFID puerta principal.** Se acerca una tarjeta autorizada. El sistema reconoce el UID y enciende el LED de puerta principal durante un intervalo corto.
6. **RFID garaje.** Se acerca una tarjeta autorizada o se selecciona garaje desde menú. El servo abre y el LED D8 se enciende; tras 10 s cierra automáticamente.
7. **Sala de juegos.** Se acerca tarjeta de hijo. Si tiene cupos, se permite ingreso y se descuenta uno. Si no tiene cupos, se deniega.
8. **Persistencia.** Se reinicia el sistema y se demuestra que los cupos descontados siguen en EEPROM.
9. **Dimmer.** Se entra al módulo de iluminación y se varía el potenciómetro. El LED cambia intensidad y el LCD muestra porcentaje.
10. **Sonido.** En `UART1` (o en el Monitor Serie vía bridge), se ejecuta `RADIO ON` y `RADIO VOL 70`. El LCD muestra porcentaje y el serial reporta el evento; no hay salida PWM de volumen.
11. **Horno.** En `UART1`, se ingresa `HORNO ON 180 2`. El sistema muestra cuenta regresiva y apaga al terminar.
12. **Mercado.** En `UART1`, se agrega un producto predefinido con cantidad y luego se consulta la lista.

## Criterios de aceptación

El sistema se considera defendible si cumple estas condiciones:

- Ninguna función principal bloquea la lectura de alarmas.
- Las alarmas reportan eventos por serial.
- La UI por LCD/teclado permite navegar de forma consistente.
- RFID diferencia al menos padre/administrador e hijo.
- Los cupos de juegos sobreviven a un reinicio.
- El horno se apaga automáticamente sin congelar el sistema.
- El dimmer usa ADC + PWM (D7) y el sonido usa setpoint remoto + estado lógico (sin PWM).
- Las simulaciones se declaran explícitamente; no se presentan como actuadores reales.

## Riesgos

| Riesgo | Mitigación |
|---|---|
| Hacer muchos menús y poca lógica real | Implementar primero RFID, alarmas y EEPROM |
| Usar retardos bloqueantes | Implementar temporización por ticks |
| Consumir EEPROM sin control | Usar registros fijos y escritura solo cuando cambian datos |
| Confundir simulación con hardware real | Documentar cada simulación en la presentación |
| Integrar tarde en ATmega2560 | Probar drivers por módulo desde el inicio |
