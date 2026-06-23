# Plan de implementación

## Objetivo

Ordenar el trabajo para que el proyecto llegue a una demo defendible. El error más costoso sería intentar construir todos los menús antes de validar los drivers base.

## Fase 1: base de plataforma

Entregables:

- `Definiciones.h` con pines, constantes y tipos.
- UART funcional para logs.
- Timer/tick del sistema.
- LCD funcional.
- Teclado matricial funcional.

Criterios de aceptación:

- El sistema imprime `[BOOT] Sistema iniciado` por serial.
- El LCD muestra el menú principal al arrancar.
- El teclado detecta dígitos, `A-D`, `*` y `#`.
- No hay retardos bloqueantes largos.

## Fase 2: ADC, PWM y actuadores simples

Entregables:

- ADC para MQ-2 y potenciómetro de iluminación.
- PWM para iluminación.
- PWM para volumen.
- Salidas digitales para LEDs/relé.

Criterios de aceptación:

- El potenciómetro de iluminación cambia porcentaje en LCD.
- El LED de iluminación cambia intensidad.
- El volumen remoto cambia porcentaje y PWM.
- Las salidas digitales responden a comandos de prueba.

## Fase 3: seguridad

Entregables:

- Lectura de PIR1 y PIR2.
- Lectura ADC de MQ-2.
- Estados de alarma acceso/incendio.
- Activación/desactivación con código.
- Reporte serial de intrusión e incendio.

Criterios de aceptación:

- PIR dispara alarma solo si la alarma de acceso está activa.
- MQ-2 dispara alarma si supera umbral.
- LCD y serial reportan el evento.
- La alarma se puede desactivar con código.

## Fase 4: EEPROM

Entregables:

- Driver propio de EEPROM.
- Header de validez.
- Tabla de usuarios.
- Escritura/lectura de registros.

Criterios de aceptación:

- Se guarda usuario de prueba.
- El usuario se recupera después de reiniciar.
- Se actualizan cupos sin corromper otros registros.

## Fase 5: RFID-RC522

Entregables:

- Driver SPI propio.
- Inicialización del RC522.
- Detección de tarjeta.
- Lectura de UID.
- Validación contra tabla de usuarios.

Criterios de aceptación:

- El serial muestra UID leído.
- Tarjeta desconocida es rechazada.
- Tarjeta enrolada es reconocida.

## Fase 6: accesos

Entregables:

- Enrolamiento desde menú.
- Borrado lógico desde menú.
- Roles padre/hijo.
- Puerta principal por LED.
- Garaje por servo.
- Sala de juegos con cupos.

Criterios de aceptación:

- Padre/admin puede gestionar tarjetas y cupos.
- Hijo consume cupos al entrar a juegos.
- Sin cupos, el acceso se deniega.
- Los cupos persisten tras reinicio.

## Fase 7: servicios

Entregables:

- Horno con temperatura y tiempo.
- Temporización no bloqueante.
- Sonido ON/OFF y volumen por `UART1`.
- Mercado con productos predefinidos.

Criterios de aceptación:

- El horno se apaga solo al terminar.
- Mientras el horno está activo, PIR/RFID/teclado siguen funcionando.
- `RADIO VOL <0-100>` actualiza la salida PWM proporcional.
- Mercado permite agregar y consultar productos.

## Fase 8: integración y demo

Entregables:

- Menú principal estable.
- Guion de demo probado.
- README y documentación actualizados.
- Tabla final de pines validada.

Criterios de aceptación:

- La demo completa se puede ejecutar en menos de 8 minutos.
- Cada evento crítico aparece en serial.
- Cada operación de usuario tiene respuesta LCD.
- No hay reinicios espontáneos ni cuelgues por bucles bloqueantes.

## Backlog técnico recomendado

| Prioridad | Tarea |
|---:|---|
| 1 | Definir pines finales en `Definiciones.h` |
| 1 | Implementar UART |
| 1 | Implementar LCD y teclado |
| 1 | Implementar tick no bloqueante |
| 2 | Implementar ADC/PWM |
| 2 | Implementar seguridad PIR/MQ-2 |
| 2 | Implementar EEPROM |
| 3 | Implementar SPI |
| 3 | Implementar RC522 mínimo |
| 3 | Implementar tabla de usuarios |
| 4 | Implementar menús de administración |
| 4 | Implementar servo garaje |
| 4 | Implementar horno |
| 5 | Implementar mercado |
| 5 | Pulir LCD, logs y demo |

## Definición de terminado

Una función está terminada solo si:

- Tiene entrada real o simulada definida.
- Tiene salida observable por LCD, serial o actuador.
- No bloquea el ciclo principal.
- Maneja error básico.
- Está integrada al menú si corresponde.
- Está descrita en la documentación.

## Migraciones cerradas

- Login global eliminado: el código solo se usa para activar o desactivar alarmas.
- `UART2` y `UART3` fuera del flujo funcional.
- Volumen del sonido migrado de potenciómetro a `UART1 / RADIO VOL <0-100>`.
