/*=============================================================================
  PROYECTO: Temporizador LED DMD (Básquet) - NODO: APOLLO (ESCLAVO)
  VERSIÓN:  v7.0 - FIX CRÍTICO: ISR LIMPIA + RECEPCIÓN PROTEGIDA
  
  CAMBIOS RESPECTO A v6.6:
  1. ELIMINADO el delayMicroseconds() de dentro de la ISR ScanDMD().
     El delay DENTRO de una interrupción es extremadamente peligroso:
     bloquea TODAS las demás interrupciones (incluyendo SoftwareSerial RX).
     Resultado: los bytes del maestro se perdían o corrompían al recibirlos.
  2. El control de brillo ahora se hace con analogWrite() en el pin OE,
     que usa PWM por hardware (Timer2) y no interfiere con nada.
  3. Se añade flush del buffer serial para evitar acumulación de basura.
=============================================================================*/

#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>
#include <SoftwareSerial.h>
#include "SystemFont5x7.h"

#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN   2
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

SoftwareSerial BTEsclavo(2, 3);
#define PIN_RELE A0

#define BOCINA_APAGAR()    { pinMode(PIN_RELE, OUTPUT); digitalWrite(PIN_RELE, HIGH); }
#define BOCINA_ENCENDER()  { pinMode(PIN_RELE, OUTPUT); digitalWrite(PIN_RELE, LOW); }

int tiempo                    = 24;
int ultimoTiempo              = 24;
bool cronometroActivo         = false;
unsigned long previousMillis  = 0;
const long intervalo          = 1000;

enum AnimState { ANIM_IDLE, ANIM_HORN, ANIM_EXPLOSION, ANIM_BLINK_ON, ANIM_BLINK_OFF };
AnimState estadoAnimacion = ANIM_IDLE;
unsigned long tiempoAnimacion = 0;
int pasoAnimacion = 0;

// ============================================================================
// CONTROL DE BRILLO v7: Sin delay dentro de la ISR
// ============================================================================
// En v6.6 el brillo se controlaba con delayMicroseconds(300) DENTRO de la ISR.
// Eso bloqueaba la recepción de SoftwareSerial y corrompía los datos.
//
// Ahora usamos analogWrite() en el pin OE:
// - analogWrite genera una señal PWM por HARDWARE (Timer2, no Timer1)
// - No bloquea ninguna interrupción
// - Valores: 0 = brillo máximo, 255 = apagado
// - El "sweet spot" para tu panel es ~180 (equivalente a tu nivelBrillo=300)
// ============================================================================
#define OE_PIN 9

// AJUSTA ESTE VALOR: 0 = máximo brillo, 255 = apagado total
// Equivalencias aproximadas con tu viejo nivelBrillo:
//   nivelBrillo=100  → brilloPWM ≈ 230 (oscuro)
//   nivelBrillo=300  → brilloPWM ≈ 180 (tu valor anterior)
//   nivelBrillo=1000 → brilloPWM ≈ 100 (brillante)
//   nivelBrillo=3500 → brilloPWM ≈ 20  (muy brillante)
byte brilloPWM = 180;

// ISR LIMPIA: Solo hace el scan SPI, nada más
void ScanDMD() {
  dmd.scanDisplayBySPI();
  // NO hay delay aquí. El brillo lo controla analogWrite() abajo.
}

void pix(int x, int y) {
  if (x >= 0 && x < 32 && y >= 0 && y < 32) dmd.writePixel(x, y, GRAPHICS_NORMAL, 1);
}

void lineaH(int x, int y, int w) {
  for (int yy = 0; yy < 4; yy++) {
    for (int xx = 0; xx < w; xx++) pix(x + xx, y + yy);
  }
}
void lineaV(int x, int y, int h) {
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < 4; xx++) pix(x + xx, y + yy);
  }
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

void dibujarCuadradoCentrado(int r) {
  int left = 15 - r;
  int right = 16 + r;
  int top = 15 - r;
  int bottom = 16 + r;
  for (int x = left; x <= right; x++) { pix(x, top); pix(x, bottom); }
  for (int y = top + 1; y < bottom; y++) { pix(left, y); pix(right, y); }
}

void iniciarAnimacion() {
  if (estadoAnimacion != ANIM_IDLE) return;
  cronometroActivo = false;
  tiempo = 0;
  mostrarTiempo(0);
  BOCINA_ENCENDER();
  estadoAnimacion = ANIM_HORN;
  tiempoAnimacion = millis();
}

void abortarAnimacion() {
  if (estadoAnimacion != ANIM_IDLE) {
    BOCINA_APAGAR();
    estadoAnimacion = ANIM_IDLE;
  }
}

void procesarAnimacion() {
  if (estadoAnimacion == ANIM_IDLE) return;

  unsigned long ahora = millis();
  unsigned long t = ahora - tiempoAnimacion;

  switch (estadoAnimacion) {
    case ANIM_HORN:
      if (t >= 2000) {
        BOCINA_APAGAR();
        dmd.clearScreen(true);
        estadoAnimacion = ANIM_EXPLOSION;
        pasoAnimacion = 0; 
        tiempoAnimacion = ahora;
      }
      break;

    case ANIM_EXPLOSION:
      if (t >= 25) { 
        dmd.clearScreen(true);
        for (int i = 0; i < 3; i++) {
            int radio = pasoAnimacion - (i * 6); 
            if (radio >= 0 && radio <= 20) { 
                dibujarCuadradoCentrado(radio);
            }
        }
        pasoAnimacion++;
        if (pasoAnimacion > 34) { 
           estadoAnimacion = ANIM_BLINK_ON;
           mostrarTiempo(0);
           pasoAnimacion = 0; 
        }
        tiempoAnimacion = ahora;
      }
      break;
      
    case ANIM_BLINK_ON:
      if (t >= 70) {
         dmd.clearScreen(true);
         estadoAnimacion = ANIM_BLINK_OFF;
         tiempoAnimacion = ahora;
      }
      break;
      
    case ANIM_BLINK_OFF:
      if (t >= 70) {
         pasoAnimacion++; 
         if (pasoAnimacion >= 6) { 
            mostrarTiempo(0);
            estadoAnimacion = ANIM_IDLE;
         } else {
            mostrarTiempo(0);
            estadoAnimacion = ANIM_BLINK_ON;
         }
         tiempoAnimacion = ahora;
      }
      break;
      
    default:
      estadoAnimacion = ANIM_IDLE;
      break;
  }
}

void setup() {
  BOCINA_APAGAR();
  
  // Control de brillo por PWM de hardware (Timer2, NO interfiere con Timer1)
  pinMode(OE_PIN, OUTPUT);
  analogWrite(OE_PIN, brilloPWM);  // PWM constante, sin tocar la ISR
  
  Serial.begin(9600);
  BTEsclavo.begin(9600);
  
  Timer1.initialize(4000);
  Timer1.attachInterrupt(ScanDMD);
  dmd.clearScreen(true);
  mostrarTiempo(tiempo);
  
  Serial.println("[APOLLO V7.0 - ISR LIMPIA + BRILLO PWM]");
}

void loop() {
  byte cmd = 0;

  procesarAnimacion();

  if (Serial.available()) {
    char tecla = Serial.read();
    if      (tecla == 's' || tecla == 'S') cmd = 250;
    else if (tecla == 'p' || tecla == 'P') cmd = 251;
    else if (tecla == '0')                 cmd = 252;
    else if (tecla == '4')                 cmd = 253;
    else if (tecla == '2')                 cmd = 254;
  }
  else if (BTEsclavo.available()) {
    byte raw = BTEsclavo.read();
    
    // Limpiar buffer para evitar acumulación de bytes corruptos residuales
    while(BTEsclavo.available() > 0) {
      BTEsclavo.read();
    }
    
    if ((raw >= 1 && raw <= 99) || (raw >= 249 && raw <= 254)) {
      cmd = raw;
    }
  }

  if (cmd != 0) {
    abortarAnimacion();

    if (cmd == 250) { 
      if (!cronometroActivo && tiempo > 0) {
        cronometroActivo = true;
        previousMillis   = millis(); 
      }
    }
    else if (cmd == 251) { cronometroActivo = false; }
    else if (cmd == 249) { iniciarAnimacion(); }
    else if (cmd == 252) { tiempo = 0; cronometroActivo = false; mostrarTiempo(0); }
    else if (cmd == 253) { tiempo = 14; ultimoTiempo = 14; cronometroActivo = false; }
    else if (cmd == 254) { tiempo = 24; ultimoTiempo = 24; cronometroActivo = false; }
    else if (cmd >= 1 && cmd <= 99) { tiempo = cmd; ultimoTiempo = cmd; cronometroActivo = false; }

    if (cmd != 249 && cmd != 252) {
      mostrarTiempo(tiempo);
    }
  }

  if (cronometroActivo) {
    if (millis() - previousMillis >= intervalo) {
      previousMillis = millis();
      if (tiempo > 0) {
        tiempo--;
        mostrarTiempo(tiempo);
        if (tiempo == 0) iniciarAnimacion(); 
      }
    }
  }
}
