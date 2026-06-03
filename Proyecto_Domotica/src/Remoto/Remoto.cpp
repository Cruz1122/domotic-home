#include "../Definiciones.h"
#include "Remoto.h"

ItemMercado listaMercado[MAX_PRODUCTOS];
int totalProductos = 0;

bool hornoEncendido = false;
unsigned long tiempoFinalHorno = 0;
int temperaturaHorno = 0;

void inicializarRemoto() {
    pinMode(PIN_VOL_SONIDO, OUTPUT);
    analogWrite(PIN_VOL_SONIDO, 0);

    for(int i = 0; i < MAX_PRODUCTOS; i++) {
        listaMercado[i].nombre = "";
        listaMercado[i].cantidad = 0;
    }
}

void escucharComandosRemotos() {
    if (Serial.available() > 0) {
        String entrada = Serial.readStringUntil('\n');
        entrada.trim();

        if (entrada.startsWith("HORNO:")) {
            String datos = entrada.substring(6);
            int posicionComa = datos.indexOf(',');

            if (posicionComa != -1) {
                int minutos = datos.substring(0, posicionComa).toInt();
                temperaturaHorno = datos.substring(posicionComa + 1).toInt();

                hornoEncendido = true;
                tiempoFinalHorno = millis() + (minutos * 2000);

                Serial.print("REMOTO: Horno ENCENDIDO por ");
                Serial.print(minutos);
                Serial.print(" minutos a ");
                Serial.print(temperaturaHorno);
                Serial.println(" grados C.");
            }
        }
        else if (entrada.startsWith("SONIDO:")) {
            int volumenSolicitado = entrada.substring(7).toInt();

            if (volumenSolicitado >= 0 && volumenSolicitado <= 100) {
                uint8_t pwmVolumen = (volumenSolicitado * 255) / 100;
                analogWrite(PIN_VOL_SONIDO, pwmVolumen);

                Serial.print("REMOTO: Volumen del equipo de sonido ajustado al ");
                Serial.print(volumenSolicitado);
                Serial.println("%. Signal proporcional PWM modificada.");
            }
        }
        else if (entrada.startsWith("MERCADO_ADD:")) {
            String datos = entrada.substring(12);
            int posicionComa = datos.indexOf(',');

            if (posicionComa != -1) {
                String producto = datos.substring(0, posicionComa);
                int cant = datos.substring(posicionComa + 1).toInt();

                if (totalProductos < MAX_PRODUCTOS) {
                    listaMercado[totalProductos].nombre = producto;
                    listaMercado[totalProductos].cantidad = cant;
                    totalProductos++;
                    Serial.println("REMOTO: Producto agregado exitosamente a la lista.");
                } else {
                    Serial.println("REMOTO Error: Lista de mercado llena.");
                }
            }
        }
        else if (entrada == "MERCADO_VER") {
            Serial.println("\n--- LISTA DE MERCADO REMOTA ---");
            if (totalProductos == 0) {
                Serial.println("(La lista se encuentra vacia)");
            } else {
                for (int i = 0; i < totalProductos; i++) {
                    Serial.print("- ");
                    Serial.print(listaMercado[i].nombre);
                    Serial.print(": ");
                    Serial.println(listaMercado[i].cantidad);
                }
            }
            Serial.println("--------------------------------");
        }
    }
}

void ejecutarTemporizadorHorno() {
    if (hornoEncendido && millis() >= tiempoFinalHorno) {
        hornoEncendido = false;
        temperaturaHorno = 0;
        Serial.println("NOTIFICACIÓN REMOTA: ¡El tiempo del Horno ha concluido! Apagando dispositivo.");
    }
}
