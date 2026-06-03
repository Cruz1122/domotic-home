# Referencias técnicas

Este proyecto se basa en la plataforma ATmega2560 / Arduino Mega 2560 y en el lector RFID MFRC522/RC522.

## ATmega2560 / Arduino Mega 2560

La placa Arduino Mega 2560 Rev3 usa microcontrolador ATmega2560, opera a 5 V, incluye 54 pines digitales de E/S, 15 salidas PWM, 16 entradas analógicas, 4 KB de EEPROM, 8 KB de SRAM, 256 KB de Flash y reloj de 16 MHz.

Fuentes:

- Arduino Store, Arduino Mega 2560 Rev3: https://store.arduino.cc/products/arduino-mega-2560-rev3
- Arduino Docs, datasheet Mega 2560 Rev3: https://docs.arduino.cc/resources/datasheets/A000067-datasheet.pdf

## RFID-RC522 / MFRC522

El MFRC522 es un lector/escritor integrado para comunicación sin contacto a 13.56 MHz. Soporta ISO/IEC 14443 A/MIFARE/NTAG y ofrece interfaces SPI, UART e I2C. Para este proyecto se usará SPI con driver propio.

Fuentes:

- NXP, MFRC522 Product Data Sheet: https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf
- NXP, MFRC522 Product Page: https://www.nxp.com/products/rfid-nfc/nfc-hf/nfc-readers/standard-performance-mifare-and-ntag-frontend:MFRC52202HN1

## Nota sobre librerías

Las fuentes anteriores documentan capacidades de hardware. El proyecto no usará librerías externas para RFID, LCD, teclado, EEPROM, SPI, PWM, ADC ni servo. Cualquier abstracción debe ser implementada como driver propio dentro del repositorio.
