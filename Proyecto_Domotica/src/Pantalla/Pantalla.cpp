#include <LiquidCrystal.h>
#include "../Definiciones.h"
#include "Pantalla.h"

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

unsigned long ultimoRefrescoPantalla = 0;
const long intervaloRefresco = 500;
bool alternarPantalla = false;

void inicializarPantalla() {
    lcd.begin(16, 2);
    lcd.clear();
    lcd.print("DOMOTICA START");
}

void actualizarPantalla(bool intrusion, bool incendio) {
    if (millis() - ultimoRefrescoPantalla < intervaloRefresco) {
        return;
    }
    ultimoRefrescoPantalla = millis();
    alternarPantalla = !alternarPantalla;

    lcd.clear();

    if (incendio) {
        lcd.setCursor(0, 0);
        lcd.print("   !ALERTA!   ");
        lcd.setCursor(0, 1);
        lcd.print(alternarPantalla ? "   INCENDIO   " : "              ");
        return;
    }

    if (intrusion) {
        lcd.setCursor(0, 0);
        lcd.print("   !ALERTA!   ");
        lcd.setCursor(0, 1);
        lcd.print(alternarPantalla ? "INTRUSO DETECTADO" : "              ");
        return;
    }

    if (alternarPantalla) {
        lcd.setCursor(0, 0);
        lcd.print("ESTADO: SEGURO");
        lcd.setCursor(0, 1);
        lcd.print("CASA PROTEGIDA");
    } else {
        lcd.setCursor(0, 0);
        lcd.print("SISTEMA DOMOTICO");
        lcd.setCursor(0, 1);
        lcd.print("ATMEGA2560 READY");
    }
}
