/*=============================================================================
  DIAGNÓSTICO #4: CÓDIGO COMPLETO SIN ANIMACIÓN NI RELÉ
  
  Es tu código v8 COMPLETO pero con:
  - Animación DESACTIVADA (cuando llega a 0, solo muestra "00")
  - Relé/Bocina DESACTIVADO
  
  Si el HC-05 MUERE → El problema es el código de dibujo pesado o
    el envío frecuente de datos al BT
  Si el HC-05 VIVE → El asesino es la animación o el relé
=============================================================================*/

#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>
#include <SoftwareSerial.h>
#include "SystemFont5x7.h"

#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN   2
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

SoftwareSerial BTApp(4, 5);
SoftwareSerial BTApollo(2, 3);

#define PIN_RELE A0

void enviarAEsclavo(byte b) {
  BTApollo.write(b);
}

int tiempo                      = 24;
int ultimoTiempo                = 24;
bool cronometroActivo           = false;
unsigned long previousMillis    = 0;
bool yaSonoCero                 = false;

void ScanDMD() { dmd.scanDisplayBySPI(); }

void pix(int x, int y) {
  if (x >= 0 && x < 32 && y >= 0 && y < 32)
    dmd.writePixel(x, y, GRAPHICS_NORMAL, 1);
}

void lineaH(int x, int y, int w) {
  for (int yy = 0; yy < 4; yy++)
    for (int xx = 0; xx < w; xx++) pix(x + xx, y + yy);
}
void lineaV(int x, int y, int h) {
  for (int yy = 0; yy < h; yy++)
    for (int xx = 0; xx < 4; xx++) pix(x + xx, y + yy);
}

void dibujar0(int o=0){int p=3+o,b=3,w=10,h=26;lineaH(p,b,w);lineaH(p,b+h-4,w);lineaV(p,b,h);lineaV(p+w-4,b,h);}
void dibujar1(int o=0){int p=3+o,b=3,h=26;lineaV(p+3,b,h);lineaH(p,b+h-4,10);lineaH(p,b,7);}
void dibujar2(int o=0){int p=3+o,b=3,w=10,h=26;lineaH(p,b,w);lineaV(p+w-4,b,h/2);lineaH(p,(b+h/2)-2,w);lineaV(p,b+h/2,h/2);lineaH(p,b+h-4,w);}
void dibujar3(int o=0){int p=3+o,b=3,w=10,h=26;lineaH(p,b,w);lineaH(p,(b+h/2)-2,w);lineaH(p,b+h-4,w);lineaV(p+w-4,b,h);}
void dibujar4(int o=0){int p=3+o,b=3,w=10,h=26;lineaV(p+w-4,b,h);lineaV(p,b,h/2);lineaH(p,(b+h/2)-2,w);}
void dibujar5(int o=0){int p=3+o,b=3,w=10,h=26;lineaH(p,b,w);lineaV(p,b,h/2);lineaH(p,(b+h/2)-2,w);lineaV(p+w-4,b+h/2,h/2);lineaH(p,b+h-4,w);}
void dibujar6(int o=0){int p=3+o,b=3,w=10,h=26;lineaV(p,b,h);lineaH(p,(b+h/2)-2,w);lineaH(p,b+h-4,w);lineaV(p+w-4,b+h/2,h/2);lineaH(p,b,w);}
void dibujar7(int o=0){int p=3+o,b=3,w=10,h=26;lineaH(p,b,w);lineaV(p+w-4,b,h);}
void dibujar8(int o=0){int p=3+o,b=3,w=10,h=26;lineaH(p,b,w);lineaH(p,(b+h/2)-2,w);lineaH(p,b+h-4,w);lineaV(p,b,h);lineaV(p+w-4,b,h);}
void dibujar9(int o=0){int p=3+o,b=3,w=10,h=26;lineaH(p,b,w);lineaH(p,(b+h/2)-2,w);lineaV(p+w-4,b,h);lineaV(p,b,h/2);lineaH(p,b+h-4,w);}

void mostrarNumero(int n, int o=0){
  switch(n){
    case 0:dibujar0(o);break; case 1:dibujar1(o);break;
    case 2:dibujar2(o);break; case 3:dibujar3(o);break;
    case 4:dibujar4(o);break; case 5:dibujar5(o);break;
    case 6:dibujar6(o);break; case 7:dibujar7(o);break;
    case 8:dibujar8(o);break; case 9:dibujar9(o);break;
  }
}
void mostrarTiempo(int v){
  dmd.clearScreen(true);
  mostrarNumero(v / 10, 0);
  mostrarNumero(v % 10, 16);
}

void procesarCmd(byte cmd) {
  // SIN ANIMACIÓN, SIN RELÉ
  if      (cmd == 250) { if (!cronometroActivo && tiempo > 0) { cronometroActivo = true; previousMillis = millis(); } }
  else if (cmd == 251) { cronometroActivo = false; }
  else if (cmd == 252) { tiempo = 0; yaSonoCero = true; cronometroActivo = false; }
  else if (cmd == 253) { tiempo = 14; ultimoTiempo = 14; yaSonoCero = false; cronometroActivo = false; }
  else if (cmd == 254) { tiempo = 24; ultimoTiempo = 24; yaSonoCero = false; cronometroActivo = false; }
  else if (cmd >= 1 && cmd <= 99) { tiempo = cmd; ultimoTiempo = cmd; yaSonoCero = false; cronometroActivo = false; }
  mostrarTiempo(tiempo);
}

void setup() {
  // Relé en estado seguro pero NO lo usamos
  pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_RELE, HIGH);
  
  Serial.begin(9600);
  BTApollo.begin(9600);
  BTApp.begin(9600);
  BTApp.listen();
  
  Timer1.initialize(4000);
  Timer1.attachInterrupt(ScanDMD);
  dmd.clearScreen(true);
  mostrarTiempo(tiempo);
  Serial.println(F("[DIAG4: Codigo completo SIN animacion SIN rele]"));
}

void loop() {
  byte cmd = 0;

  // NO hay procesarAnimacion() aquí

  if (Serial.available()) {
    char t = Serial.read();
    if      (t == 's' || t == 'S') cmd = 250;
    else if (t == 'p' || t == 'P') cmd = 251;
    else if (t == '0')             cmd = 252;
    else if (t == '4')             cmd = 253;
    else if (t == '2')             cmd = 254;
  }
  else if (BTApp.available()) {
    byte raw = BTApp.read();
    
    while(BTApp.available() > 0) {
      BTApp.read();
    }

    if ((raw >= 1 && raw <= 99) || (raw >= 250 && raw <= 254)) {
      cmd = raw;
    }
  }

  if (cmd != 0) {
    procesarCmd(cmd);
    enviarAEsclavo(cmd);
    Serial.print(F("CMD: "));
    Serial.println(cmd);
  }

  if (cronometroActivo) {
    if (millis() - previousMillis >= 1000) {
      previousMillis = millis();
      if (tiempo > 0) {
        tiempo--;
        mostrarTiempo(tiempo);
        enviarAEsclavo((byte)tiempo);  // Enviar tiempo actual al esclavo
        Serial.print(F("T: "));
        Serial.println(tiempo);
        if (tiempo == 0 && !yaSonoCero) {
          yaSonoCero = true;
          // SIN animación, SIN bocina, SIN enviar 249
          // Solo mostramos 00 y paramos
          Serial.println(F("CERO! (sin animacion)"));
        }
      }
    }
  }
}
