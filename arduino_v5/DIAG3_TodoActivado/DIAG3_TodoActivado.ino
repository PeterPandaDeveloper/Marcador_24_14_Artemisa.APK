/*=============================================================================
  DIAGNÓSTICO #3: ¿EL DMD/TIMER1 MATA AL HC-05?
  
  Este sketch activa TODO: ambos HC-05 + DMD + Timer1.
  Si el diag #2 pasó pero este falla → El Timer1/DMD está matando al HC-05.
  
  INSTRUCCIONES:
  1. Conecta TODO (paneles DMD, ambos HC-05, relé)
  2. Sube este sketch al Arduino MAESTRO
  3. Abre Monitor Serial a 9600
  4. Observa el LED de BTApollo
  
  RESULTADO:
  - Si BTApollo muere → Confirmado: el Timer1/SPI del DMD mata al HC-05
    Solución: usar Hardware Serial (pines 0,1) para BTApollo
  - Si vive → El problema está en una parte específica de tu código
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

void ScanDMD() { dmd.scanDisplayBySPI(); }

void setup() {
  Serial.begin(9600);
  BTApollo.begin(9600);
  BTApp.begin(9600);
  BTApp.listen();
  
  Timer1.initialize(4000);
  Timer1.attachInterrupt(ScanDMD);
  dmd.clearScreen(true);
  
  // Escribir algo en el DMD para que tenga trabajo real
  dmd.selectFont(SystemFont5x7);
  dmd.drawString(2, 4, "24", 2, GRAPHICS_NORMAL);
  
  Serial.println(F("========================================"));
  Serial.println(F("  DIAG #3: TODO ACTIVADO"));
  Serial.println(F("  DMD + Timer1 + Ambos HC-05"));
  Serial.println(F("========================================"));
}

void loop() {
  static unsigned long ultimo = 0;
  static int contador = 0;
  
  if (BTApp.available()) {
    byte raw = BTApp.read();
    while(BTApp.available()) BTApp.read();
    
    Serial.print(F("App: "));
    Serial.println(raw);
    BTApollo.write(raw);
  }
  
  if (millis() - ultimo >= 2000) {
    ultimo = millis();
    contador++;
    
    BTApollo.write((byte)254);
    
    Serial.print(F("["));
    Serial.print(millis() / 1000);
    Serial.print(F("s] #"));
    Serial.print(contador);
    Serial.println(F(" - BTApollo LED vivo?"));
  }
}
