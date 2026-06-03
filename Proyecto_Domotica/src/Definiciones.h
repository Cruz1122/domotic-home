#ifndef DEFINICIONES_H
#define DEFINICIONES_H

#include <avr/io.h>
#include <Arduino.h>

// --- MÓDULO SEGURIDAD (PUERTO C) ---
#define DDR_SEGURIDAD    DDRC
#define PORT_SEGURIDAD   PORTC
#define PIN_SEGURIDAD    PINC
#define BIT_VENTANA1     PC0
#define BIT_VENTANA2     PC1
#define BIT_PUERTA       PC2
#define BIT_HUMO         PC3

// --- MÓDULO ACCESOS (PUERTO A) ---
#define DDR_SELECTOR     DDRA
#define PORT_SELECTOR    PORTA
#define PIN_SELECTOR     PINA
#define BIT_SEL0         PA0
#define BIT_SEL1         PA1

// --- MÓDULO CONFORT (PUERTO L) ---
#define DDR_CONFORT      DDRL
#define PORT_CONFORT     PORTL
#define BIT_CALEFACTOR   PL0
#define BIT_VENTILADOR   PL1
#define PIN_LED_DIMMER   4

// --- MÓDULO REMOTO ---
#define PIN_VOL_SONIDO   9
#define MAX_PRODUCTOS    5

struct ItemMercado {
    String nombre;
    int cantidad;
};

// --- MÓDULO PANTALLA LCD ---
#define LCD_RS           12
#define LCD_EN           11
#define LCD_D4           5
#define LCD_D5           6
#define LCD_D6           7
#define LCD_D7           8

// --- ESTRUCTURA PARA LAS TARJETAS RFID ---
struct Usuario {
    byte uid[4];
    byte rol;
    int accesosJuegos;
};

#endif
