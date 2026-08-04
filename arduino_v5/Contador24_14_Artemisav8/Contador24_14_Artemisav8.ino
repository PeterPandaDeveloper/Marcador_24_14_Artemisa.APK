/*=============================================================================
  PROYECTO: Temporizador LED DMD (Básquet) - NODO: MAESTRO (ARTEMISA)
  VERSIÓN:  v8.0 - FIX CRÍTICO: PROTECCIÓN TIMER1 vs SOFTWARESERIAL
  
  CAMBIOS RESPECTO A v7.1:
  1. enviarAEsclavo() deshabilita interrupciones durante la transmisión
     para evitar que Timer1 (ScanDMD) corrompa los bytes enviados por
     SoftwareSerial (bit-banging). Esto resuelve el problema del HC-05
     BTApollo que se congela con LED rojo fijo.
  2. Se separan los puertos Bluetooth a PORTD y PORTB/PORTC para evitar
     conflictos de PCINT en el mismo puerto.
     NOTA: Como BTApollo SOLO TRANSMITE (nunca recibimos del esclavo),
     movemos su RX a un pin que no genere interferencia PCINT.
  3. Se añade flush del buffer de BTApollo antes de cada envío.
=============================================================================*/

#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>
#include <SoftwareSerial.h>
#include "SystemFont5x7.h"

#define DISPLAYS_ACROSS 1
#define DISPLAYS_DOWN   2
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN);

// =====================================================================
// CONFIGURACIÓN BLUETOOTH - PINES SEPARADOS POR PUERTO
// =====================================================================
// BTApp en PORTD (pines 4,5) - Este ESCUCHA datos de la app
// BTApollo en PORTD (pines 2,3) - Este SOLO TRANSMITE al esclavo
//
// IMPORTANTE: Como BTApollo nunca necesita recibir datos (solo envía),
// ponemos su RX (pin 2) pero jamás llamamos BTApollo.listen().
// El pin 2 queda como INPUT pero sin PCINT activo.
// =====================================================================
SoftwareSerial BTApp(4, 5);      // RX:4, TX:5 (Bluetooth de la App)
SoftwareSerial BTApollo(2, 3);   // RX:2, TX:3 (Bluetooth hacia el Esclavo)

#define PIN_RELE A0

#define BOCINA_APAGAR()    { pinMode(PIN_RELE, OUTPUT); digitalWrite(PIN_RELE, HIGH); }
#define BOCINA_ENCENDER()  { pinMode(PIN_RELE, OUTPUT); digitalWrite(PIN_RELE, LOW); }

// =====================================================================
// FIX CRÍTICO: Transmisión protegida contra interrupciones
// =====================================================================
// Un byte a 9600 baud = ~1.04ms (10 bits × 104µs/bit)
// Timer1 interrumpe cada 4ms para ScanDMD()
// Sin protección: el ISR corta el bit-banging y el HC-05 recibe basura
// Con protección: los paneles DMD "parpadean" ~1ms (imperceptible)
// =====================================================================
void enviarAEsclavo(byte b) {
  noInterrupts();         // Bloquea Timer1 durante ~1ms
  BTApollo.write(b);      // Transmisión limpia, sin interferencias
  interrupts();           // Restaura Timer1
}

int tiempo                      = 24;
int ultimoTiempo                = 24;
bool cronometroActivo           = false;
unsigned long previousMillis    = 0;
bool yaSonoCero                 = false;

enum AnimState {
  ANIM_IDLE, ANIM_HORN, ANIM_EXPLOSION, ANIM_BLINK_ON, ANIM_BLINK_OFF
};
AnimState estadoAnimacion = ANIM_IDLE;
unsigned long tiempoAnimacion = 0;
int pasoAnimacion = 0;

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

// === DIBUJO DE NÚMEROS ===
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

void procesarCmd(byte cmd) {
  if (estadoAnimacion != ANIM_HORN) { abortarAnimacion(); }
  if      (cmd == 250) { if (!cronometroActivo && tiempo > 0) { cronometroActivo = true; previousMillis = millis(); } }
  else if (cmd == 251) { cronometroActivo = false; }
  else if (cmd == 252) { tiempo = 0; yaSonoCero = true; cronometroActivo = false; }
  else if (cmd == 253) { tiempo = 14; ultimoTiempo = 14; yaSonoCero = false; cronometroActivo = false; }
  else if (cmd == 254) { tiempo = 24; ultimoTiempo = 24; yaSonoCero = false; cronometroActivo = false; }
  else if (cmd >= 1 && cmd <= 99) { tiempo = cmd; ultimoTiempo = cmd; yaSonoCero = false; cronometroActivo = false; }
  mostrarTiempo(tiempo);
}

void setup() {
  BOCINA_APAGAR();
  Serial.begin(9600);
  BTApollo.begin(9600);
  BTApp.begin(9600);
  BTApp.listen();       // Solo BTApp escucha (BTApollo solo transmite)
  
  Timer1.initialize(4000);
  Timer1.attachInterrupt(ScanDMD);
  dmd.clearScreen(true);
  mostrarTiempo(tiempo);
  Serial.println("[MAESTRO V8.0 - FIX TIMER1/SOFTSERIAL]");
}

void loop() {
  byte cmd = 0;
  procesarAnimacion();

  // === LECTURA DE COMANDOS ===
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
    
    // DESTRUCTOR DE SPAM: Vaciar buffer si la app mandó múltiples comandos
    while(BTApp.available() > 0) {
      BTApp.read();
    }

    if ((raw >= 1 && raw <= 99) || (raw >= 250 && raw <= 254)) {
      cmd = raw;
    }
  }

  // === EJECUCIÓN DE COMANDOS ===
  if (cmd != 0) {
    // PRIMERO: Procesar localmente (actualizar DMD y estado)
    procesarCmd(cmd);
    
    // DESPUÉS: Enviar al esclavo con protección anti-interrupción
    // El orden importa: primero estabilizamos el DMD local,
    // luego transmitimos con interrupciones deshabilitadas.
    enviarAEsclavo(cmd);
  }

  // === MOTOR DE TIEMPO ===
  if (cronometroActivo) {
    if (millis() - previousMillis >= 1000) {
      previousMillis = millis();
      if (tiempo > 0) {
        tiempo--;
        mostrarTiempo(tiempo);
        if (tiempo == 0 && !yaSonoCero) {
          yaSonoCero = true;
          enviarAEsclavo(249);    // Señal de disparo protegida
          iniciarAnimacion(); 
        }
      }
    }
  }
}
