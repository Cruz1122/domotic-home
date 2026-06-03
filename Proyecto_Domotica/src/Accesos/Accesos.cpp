#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include "../Definiciones.h"
#include "Accesos.h"

#define SS_PIN 53
#define RST_PIN 5

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo servoGaraje;

void inicializarAccesos() {
    SPI.begin();
    mfrc522.PCD_Init();

    servoGaraje.attach(11);
    servoGaraje.write(0);

    DDR_SELECTOR &= ~((1 << BIT_SEL0) | (1 << BIT_SEL1));
    PORT_SELECTOR |= (1 << BIT_SEL0) | (1 << BIT_SEL1);
}

byte leerUbicacionCasa() {
    return (~PIN_SELECTOR) & 0x03;
}

void procesarControlAcceso(Usuario &usuarioActivo) {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    byte zonaActual = leerUbicacionCasa();

    switch(zonaActual) {
        case 0:
            Serial.println("RFID: Leyendo en puerta de GARAJE.");
            Serial.println("Acceso Concedido. Abriendo servomotor...");
            servoGaraje.write(90);
            break;

        case 1:
            Serial.println("RFID: Leyendo en HABITACIÓN DE JUEGOS.");
            if (usuarioActivo.rol == 2) {
                if (usuarioActivo.accesosJuegos > 0) {
                    usuarioActivo.accesosJuegos--;
                    Serial.print("¡Bienvenido! Créditos restantes para jugar: ");
                    Serial.println(usuarioActivo.accesosJuegos);
                } else {
                    Serial.println("Acceso DENEGADO: Te quedaste sin créditos de juego.");
                }
            } else if (usuarioActivo.rol == 1) {
                Serial.println("Acceso concedido automáticamente (Rol: Padre).");
            }
            break;

        case 2:
            Serial.println("RFID: Leyendo en la SALA.");
            if (usuarioActivo.rol == 1) {
                Serial.println("Identidad confirmada: Modo recarga de saldos habilitado.");
            } else {
                Serial.println("Operación denegada: Solo los padres pueden administrar el sistema.");
            }
            break;

        default:
            Serial.println("Error: Ubicación de casa desconocida.");
            break;
    }

    mfrc522.PICC_HaltA();
}
