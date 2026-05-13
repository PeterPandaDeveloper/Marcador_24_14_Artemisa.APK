/*----------------------------------------------------------
PROYECTO: Temporizador con pantalla LED DMD y control Bluetooth
VERSION: 3.0 GOLD - Protocolo de 2 Bytes (Sin Lag)
----------------------------------------------------------*/
#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>
#include <SoftwareSerial.h>
#include "SystemFont5x7.h"

// === CONFIGURACIÓN DE PANELES (APILADOS) ===
#define DISPLAYS_ACROSS 1 //
#define DISPLAYS_DOWN 2   //
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN); //

// === BLUETOOTH HC-05 ===
#define BT_RX 3 //
#define BT_TX 2 //
SoftwareSerial BT(BT_RX, BT_TX); //

// === VARIABLES DE CRONÓMETRO ===
int tiempo = 24; //
int ultimoTiempo = 24; 
bool cronometroActivo = false; //
unsigned long previousMillis = 0; //
const long intervalo = 1000; //

// === ESCANEO DE MATRIZ ===
void ScanDMD() { dmd.scanDisplayBySPI(); } //

// === CONFIGURACIÓN DE MATRIZ ===
const int totalWidth = 32;  //
const int totalHeight = 32; //
const int grosor = 4;       //

// === FUNCIONES DE DIBUJO (ORIGINALES SIN ROTACIÓN) ===
void pix(int x, int y, int offset = 0) {
  int rx = x + offset; 
  int ry = y;          
  if (rx >= 0 && rx < totalWidth && ry >= 0 && ry < totalHeight) {
    dmd.writePixel(rx, ry, GRAPHICS_NORMAL, 1); //
  }
}

void lineaH(int x, int y, int w) {
  for (int yy = 0; yy < grosor; yy++) {
    for (int xx = 0; xx < w; xx++) {
      pix(x + xx, y + yy);
    }
  }
}

void lineaV(int x, int y, int h) {
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < grosor; xx++) {
      pix(x + xx, y + yy);
    }
  }
}

// === DIBUJOS DE NÚMEROS (TU VERSIÓN IDEAL) ===
void dibujar1(int offset=0) { int posX=3+offset, baseY=3, h=26; lineaV(posX+3, baseY, h); lineaH(posX, baseY+h-grosor, 10); lineaH(posX, baseY, 7); }
void dibujar2(int offset=0) { int posX=3+offset, baseY=3, w=10, h=26; lineaH(posX, baseY, w); lineaV(posX+w-grosor, baseY, h/2); lineaH(posX, (baseY+h/2)-2, w); lineaV(posX, baseY+h/2, h/2); lineaH(posX, baseY+h-grosor, w); }
void dibujar3(int offset=0) { int posX=3+offset, baseY=3, w=10, h=26; lineaH(posX, baseY, w); lineaH(posX, (baseY+h/2)-2, w); lineaH(posX, baseY+h-grosor, w); lineaV(posX+w-grosor, baseY, h); }
void dibujar4(int offset=0) { int posX=3+offset, baseY=3, w=10, h=26; lineaV(posX+w-grosor, baseY, h); lineaV(posX, baseY, h/2); lineaH(posX, (baseY+h/2)-2, w); }
void dibujar5(int offset=0) { int posX=3+offset, baseY=3, w=10, h=26; lineaH(posX, baseY, w); lineaV(posX, baseY, h/2); lineaH(posX, (baseY+h/2)-2, w); lineaV(posX+w-grosor, baseY+h/2, h/2); lineaH(posX, baseY+h-grosor, w); }
void dibujar6(int offset=0) { int posX=3+offset, baseY=3, w=10, h=26; lineaV(posX, baseY, h); lineaH(posX, (baseY+h/2)-2, w); lineaH(posX, baseY+h-grosor, w); lineaV(posX+w-grosor, baseY+h/2, h/2); lineaH(posX, baseY, w); }
void dibujar7(int offset=0) { int posX=3+offset, baseY=3, w=10, h=26; lineaH(posX, baseY, w); lineaV(posX+w-grosor, baseY, h); }
void dibujar8(int offset=0) { int posX=3+offset, baseY=3, w=10, h=26; lineaH(posX, baseY, w); lineaH(posX, (baseY+h/2)-2, w); lineaH(posX, baseY+h-grosor, w); lineaV(posX, baseY, h); lineaV(posX+w-grosor, baseY, h); }
void dibujar9(int offset=0) { int posX=3+offset, baseY=3, w=10, h=26; lineaH(posX, baseY, w); lineaH(posX, (baseY+h/2)-2, w); lineaV(posX+w-grosor, baseY, h); lineaV(posX, baseY, h/2); lineaH(posX, baseY+h-grosor, w); }
void dibujar0(int offset=0) { int posX=3+offset, baseY=3, w=10, h=26; lineaH(posX, baseY, w); lineaH(posX, baseY+h-grosor, w); lineaV(posX, baseY, h); lineaV(posX+w-grosor, baseY, h); }

void mostrarNumero(int n, int offset=0) {
  switch (n) {
    case 0: dibujar0(offset); break; case 1: dibujar1(offset); break;
    case 2: dibujar2(offset); break; case 3: dibujar3(offset); break;
    case 4: dibujar4(offset); break; case 5: dibujar5(offset); break;
    case 6: dibujar6(offset); break; case 7: dibujar7(offset); break;
    case 8: dibujar8(offset); break; case 9: dibujar9(offset); break;
  }
}

void mostrarTiempo(int valor) {
  dmd.clearScreen(true); //
  mostrarNumero(valor / 10, 0); //
  mostrarNumero(valor % 10, 16); //
}

void setup() {
  Timer1.initialize(3000); //
  Timer1.attachInterrupt(ScanDMD); //
  dmd.clearScreen(true); //
  BT.begin(9600); //
  mostrarTiempo(tiempo); //
}

// === LOOP DEFINITIVO DE 1 BYTE (CERO LAG) ===
void loop() {
  if (BT.available()) {
    byte cmd = BT.read(); // Leemos el Byte crudo (0 a 255)
    
    // Controles Especiales (Números Altos)
    if (cmd == 250) { // START
      if (!cronometroActivo && tiempo > 0) {
        cronometroActivo = true;
        previousMillis = millis();
      }
    } 
    else if (cmd == 251) { cronometroActivo = false; } // PAUSE
    else if (cmd == 252) { tiempo = 0; cronometroActivo = false; } // ZERO
    else if (cmd == 253) { tiempo = 14; ultimoTiempo = 14; cronometroActivo = false; } // T14
    else if (cmd == 254) { tiempo = 24; ultimoTiempo = 24; cronometroActivo = false; } // H24
    
    // Ingreso de Números (Del 0 al 99)
    else if (cmd >= 0 && cmd <= 99) { 
      tiempo = cmd;
      ultimoTiempo = cmd;
      cronometroActivo = false;
    }
    
    // Mostramos inmediatamente
    mostrarTiempo(tiempo);
  }

  // --- Control de tiempo (Intacto) ---
  if (cronometroActivo) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= intervalo) {
      previousMillis = currentMillis;
      if (tiempo > 0) {
        tiempo--;
        mostrarTiempo(tiempo);
      } else {
        cronometroActivo = false;
      }
    }
  }
}