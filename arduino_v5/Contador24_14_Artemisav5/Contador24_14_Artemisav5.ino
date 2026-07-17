/*=============================================================================
  PROYECTO: Temporizador LED DMD (Básquet) - NODO: ARTEMISA (MAESTRO)
  VERSIÓN:  v5.0 - [Infalible / Grado Profesional]
  
  DESCRIPCIÓN: 
  Artemisa es el cerebro principal. Recibe comandos de una App Flutter, 
  cuenta el tiempo, controla un panel DMD y un relé local, y reenvía en tiempo 
  real los comandos a un nodo esclavo (Apollo).
  * V5: Animaciones y bocina 100% asíncronas (no-blocking) sin usar delay().
=============================================================================*/

#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>
#include <SoftwareSerial.h>
#include "SystemFont5x7.h"

// --- CONFIGURACIÓN DE PANTALLA ---
#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN   2
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

// --- CONFIGURACIÓN BLUETOOTH ---
SoftwareSerial BTApp(4, 5); // RX, TX
SoftwareSerial BTApollo(2, 3); // RX, TX

// --- CONFIGURACIÓN DE HARDWARE ---
#define PIN_RELE A0

// --- VARIABLES DE ESTADO DEL TIEMPO ---
int tiempo                    = 24;
int ultimoTiempo              = 24;
bool cronometroActivo         = false;
unsigned long previousMillis  = 0;
const long intervalo          = 1000;
unsigned long ultimaVezRecibido = 0;

// --- MÁQUINA DE ESTADOS: ANIMACIÓN Y BOCINA ---
enum AnimState {
  ANIM_IDLE,
  ANIM_HORN,
  ANIM_BLINK_OFF,
  ANIM_BLINK_ON,
  ANIM_CURTAIN,
  ANIM_PRE_FLASH,
  ANIM_FLASH_OFF_1,
  ANIM_FLASH_ON,
  ANIM_FLASH_OFF_2
};

AnimState estadoAnimacion = ANIM_IDLE;
unsigned long tiempoAnimacion = 0;
int pasoAnimacion = 0;

// ─── ISR DEL DMD ────────────────────────────────────────
void ScanDMD() { dmd.scanDisplayBySPI(); }

// ─── PRIMITIVAS DE DIBUJO (Optimizado) ──────────────────
void pix(int x, int y) {
  if (x >= 0 && x < 32 && y >= 0 && y < 32) {
    dmd.writePixel(x, y, GRAPHICS_NORMAL, 1);
  }
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

// ─── DISEÑO DE DÍGITOS ESTILIZADOS ──────────────────────
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
  mostrarNumero(v / 10, 0);   // Decena
  mostrarNumero(v % 10, 16);  // Unidad
}

// ─── CONTROL ASÍNCRONO DE ANIMACIÓN Y BOCINA ────────────
void iniciarAnimacion() {
  if (estadoAnimacion != ANIM_IDLE) return;
  cronometroActivo = false;
  tiempo = 0;
  mostrarTiempo(0);
  
  digitalWrite(PIN_RELE, LOW); // Activa bocina
  estadoAnimacion = ANIM_HORN;
  tiempoAnimacion = millis();
}

void abortarAnimacion() {
  if (estadoAnimacion != ANIM_IDLE) {
    digitalWrite(PIN_RELE, HIGH); // Silenciar bocina
    estadoAnimacion = ANIM_IDLE;
  }
}

void procesarAnimacion() {
  if (estadoAnimacion == ANIM_IDLE) return;

  unsigned long ahora = millis();
  unsigned long transcurrido = ahora - tiempoAnimacion;

  switch (estadoAnimacion) {
    case ANIM_HORN:
      if (transcurrido >= 2000) {
        digitalWrite(PIN_RELE, HIGH); // Apaga bocina
        estadoAnimacion = ANIM_BLINK_OFF;
        pasoAnimacion = 0;
        tiempoAnimacion = ahora;
      }
      break;
    
    case ANIM_BLINK_OFF:
      if (transcurrido >= 25) {
        dmd.clearScreen(true);
        estadoAnimacion = ANIM_BLINK_ON;
        tiempoAnimacion = ahora;
      }
      break;

    case ANIM_BLINK_ON:
      if (transcurrido >= 25) {
        mostrarTiempo(0);
        pasoAnimacion++;
        if (pasoAnimacion >= 6) {
          estadoAnimacion = ANIM_CURTAIN;
          pasoAnimacion = 0;
        } else {
          estadoAnimacion = ANIM_BLINK_OFF;
        }
        tiempoAnimacion = ahora;
      }
      break;

    case ANIM_CURTAIN:
      if (transcurrido >= 40) {
        int col = pasoAnimacion;
        for (int y = 0; y < 32; y++) dmd.writePixel(col, y, GRAPHICS_NORMAL, 1);
        for (int y = 0; y < 32; y++) dmd.writePixel(31 - col, y, GRAPHICS_NORMAL, 1);
        pasoAnimacion++;
        if (pasoAnimacion >= 16) {
          estadoAnimacion = ANIM_PRE_FLASH;
        }
        tiempoAnimacion = ahora;
      }
      break;

    case ANIM_PRE_FLASH:
      if (transcurrido >= 200) {
        dmd.clearScreen(true);
        estadoAnimacion = ANIM_FLASH_OFF_1;
        tiempoAnimacion = ahora;
      }
      break;
      
    case ANIM_FLASH_OFF_1:
      if (transcurrido >= 80) {
        for (int x = 0; x < 32; x++) {
          for (int y = 0; y < 32; y++) {
            dmd.writePixel(x, y, GRAPHICS_NORMAL, 1);
          }
        }
        estadoAnimacion = ANIM_FLASH_ON;
        tiempoAnimacion = ahora;
      }
      break;

    case ANIM_FLASH_ON:
      if (transcurrido >= 150) {
        dmd.clearScreen(true);
        estadoAnimacion = ANIM_FLASH_OFF_2;
        tiempoAnimacion = ahora;
      }
      break;
      
    case ANIM_FLASH_OFF_2:
      if (transcurrido >= 80) {
        mostrarTiempo(0);
        estadoAnimacion = ANIM_IDLE; // Fin de animación
      }
      break;
      
    default:
      estadoAnimacion = ANIM_IDLE;
      break;
  }
}

// ─── PROCESADOR CENTRAL DE COMANDOS ──────────────────────
void procesarCmd(byte cmd) {
  // CRÍTICO: Cualquier comando del usuario aborta la bocina/animación instantáneamente
  abortarAnimacion();

  if (cmd == 250) { // START
    if (!cronometroActivo && tiempo > 0) {
      cronometroActivo = true;
      previousMillis   = millis(); 
    }
  }
  else if (cmd == 251) { // PAUSE
    cronometroActivo = false;
  }
  else if (cmd == 252) { // SET ZERO (Silencioso)
    tiempo = 0; 
    cronometroActivo = false;
  }
  else if (cmd == 253) { // RESET 14s
    tiempo = 14; 
    ultimoTiempo = 14;
    cronometroActivo = false; 
  }
  else if (cmd == 254) { // RESET 24s
    tiempo = 24; 
    ultimoTiempo = 24;
    cronometroActivo = false; 
  }
  else if (cmd >= 1 && cmd <= 99) { // TIEMPO MANUAL CUSTOM
    tiempo = cmd; 
    ultimoTiempo = cmd;
    cronometroActivo = false; 
  }
  mostrarTiempo(tiempo);
}

// ─── SETUP ───────────────────────────────────────────────
void setup() {
  digitalWrite(PIN_RELE, HIGH); 
  pinMode(PIN_RELE, OUTPUT);

  Serial.begin(9600);
  BTApollo.begin(9600); 
  BTApp.begin(9600);
  
  BTApp.listen(); // Forzar escucha del celular

  Timer1.initialize(4000); // 4ms
  Timer1.attachInterrupt(ScanDMD);
  dmd.clearScreen(true);

  mostrarTiempo(tiempo);

  Serial.println("\n[SISTEMA LISTO] ARTEMISA V5 (100% ASINCRONO) INICIADA");
}

// ─── LOOP PRINCIPAL (Totalmente No-Bloqueante) ───────────
void loop() {
  byte cmd = 0;

  // 1. Procesador de Bocina y Animación en 2do Plano
  procesarAnimacion();

  // 2. Escucha Serial (PC)
  if (Serial.available()) {
    char tecla = Serial.read();
    if      (tecla == 's' || tecla == 'S') cmd = 250;
    else if (tecla == 'p' || tecla == 'P') cmd = 251;
    else if (tecla == '0')                 cmd = 252;
    else if (tecla == '4')                 cmd = 253;
    else if (tecla == '2')                 cmd = 254;
  }
  // 3. Escucha Bluetooth (App)
  else if (BTApp.available()) {
    byte raw = BTApp.read();
    if ((raw >= 1 && raw <= 99) || (raw >= 250 && raw <= 254)) {
      BTApollo.write(raw); // Espejo a Apollo

      if (millis() - ultimaVezRecibido >= 100) {
        ultimaVezRecibido = millis();
        cmd = raw;
      }
    }
  }

  // 4. Ejecutar Comandos
  if (cmd != 0) {
    if (!BTApp.available()) BTApollo.write(cmd); // Mandar a Apollo si vino del Serial
    procesarCmd(cmd);
  }

  // 5. Motor de Tiempo
  if (cronometroActivo) {
    if (millis() - previousMillis >= intervalo) {
      previousMillis = millis(); 
      
      if (tiempo > 0) {
        tiempo--;
        mostrarTiempo(tiempo);

        if (tiempo == 0) {
          BTApollo.write((byte)249); // Señal de disparo para Apollo
          iniciarAnimacion();        // Iniciar máquina de estados local
        }
      }
    }
  }
}
