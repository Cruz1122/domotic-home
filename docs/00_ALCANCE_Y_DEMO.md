# Alcance cerrado y guion de demo

## Propósito

Este documento fija el alcance real del proyecto `domotic-home`. Su función es impedir que el sistema se convierta en una lista de requisitos imposibles de demostrar con el hardware disponible. La entrega debe verse como un sistema integrado de vivienda, no como pruebas aisladas de periféricos.

## Alcance cerrado

El sistema se implementará sobre ATmega2560 con módulos propios en C puro. La interfaz principal será LCD + teclado matricial 4x4. La telemetría y los eventos críticos se reportarán por UART/Serial.

El término "remoto" del enunciado se resolverá como operación desde una interfaz local de usuario: el usuario no manipula directamente los actuadores, sino que solicita acciones desde el menú LCD/teclado. No se implementará WiFi, Bluetooth, Ethernet ni GSM salvo que se agregue hardware adicional y se actualice este documento.

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
| Horno remoto | Configuración desde menú; LED/relé como actuador; temporización no bloqueante |
| Sonido remoto | ON/OFF desde menú; volumen por potenciómetro; porcentaje en LCD; PWM proporcional |
| Lista de mercado | Productos predefinidos + cantidad; consulta por LCD |

## Demo mínima obligatoria

La secuencia recomendada para la presentación es la siguiente:

1. **Arranque e ingreso seguro.** El LCD muestra solicitud de código. Se digita el código por teclado. Si el código es correcto, entra al menú principal. Si no, se reporta intento inválido por serial.
2. **Alarma de acceso.** Se activa desde menú de seguridad. Luego se dispara un PIR. El LCD muestra intrusión y el serial reporta el evento.
3. **Alarma de incendio.** Se activa desde menú. Luego se dispara o simula lectura alta del MQ-2. El LCD muestra nivel de humo y el serial reporta alarma de incendio.
4. **RFID puerta principal.** Se acerca una tarjeta autorizada. El sistema reconoce el UID y enciende el LED de puerta principal durante un intervalo corto.
5. **RFID garaje.** Se acerca una tarjeta autorizada o se selecciona garaje desde menú. El servo ejecuta apertura/cierre.
6. **Sala de juegos.** Se acerca tarjeta de hijo. Si tiene cupos, se permite ingreso y se descuenta uno. Si no tiene cupos, se deniega.
7. **Persistencia.** Se reinicia el sistema y se demuestra que los cupos descontados siguen en EEPROM.
8. **Dimmer.** Se entra al módulo de iluminación y se varía el potenciómetro. El LED cambia intensidad y el LCD muestra porcentaje.
9. **Horno.** Se ingresa temperatura y tiempo desde teclado. El sistema muestra cuenta regresiva y apaga al terminar.
10. **Sonido.** Se activa sonido desde menú y se cambia el volumen con potenciómetro. El LCD muestra porcentaje en tiempo real y la salida PWM cambia.
11. **Mercado.** Se agrega un producto predefinido con cantidad y luego se consulta la lista.

## Criterios de aceptación

El sistema se considera defendible si cumple estas condiciones:

- Ninguna función principal bloquea la lectura de alarmas.
- Las alarmas reportan eventos por serial.
- La UI por LCD/teclado permite navegar de forma consistente.
- RFID diferencia al menos padre/administrador e hijo.
- Los cupos de juegos sobreviven a un reinicio.
- El horno se apaga automáticamente sin congelar el sistema.
- El volumen y el dimmer usan ADC + PWM, no solo texto en LCD.
- Las simulaciones se declaran explícitamente; no se presentan como actuadores reales.

## Riesgos

| Riesgo | Mitigación |
|---|---|
| Hacer muchos menús y poca lógica real | Implementar primero RFID, alarmas y EEPROM |
| Usar retardos bloqueantes | Implementar temporización por ticks |
| Consumir EEPROM sin control | Usar registros fijos y escritura solo cuando cambian datos |
| Confundir simulación con hardware real | Documentar cada simulación en la presentación |
| Integrar tarde en ATmega2560 | Probar drivers por módulo desde el inicio |
