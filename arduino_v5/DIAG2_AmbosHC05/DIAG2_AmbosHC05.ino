/*=============================================================================
  DIAGNÓSTICO #2: ¿LOS DOS HC-05 CONVIVEN?
  
  Este sketch activa AMBOS HC-05 pero SIN DMD ni Timer1.
  Si el diag #1 pasó pero este falla → los 2 módulos BT se interfieren.
  
  INSTRUCCIONES:
  1. Desconecta los paneles DMD
  2. Conecta AMBOS HC-05 (App y Apollo)
  3. Sube este sketch al Arduino MAESTRO
  4. Abre Monitor Serial a 9600
  5. Observa ambos LEDs
  
  RESULTADO:
  - Si BTApollo muere en ~5s → Los módulos se interfieren entre sí
  - Si ambos viven → El problema es la interacción con el DMD/Timer1
    Pasa al diagnóstico #3.
=============================================================================*/

#include <SoftwareSerial.h>

SoftwareSerial BTApp(4, 5);      // RX:4, TX:5
SoftwareSerial BTApollo(2, 3);   // RX:2, TX:3

void setup() {
  Serial.begin(9600);
  BTApollo.begin(9600);
  BTApp.begin(9600);
  BTApp.listen();  // Igual que en tu código real
  
  Serial.println(F("========================================"));
  Serial.println(F("  DIAG #2: AMBOS HC-05 (Sin DMD)"));
  Serial.println(F("  BTApp escuchando, BTApollo transmitiendo"));
  Serial.println(F("========================================"));
}

void loop() {
  static unsigned long ultimo = 0;
  static int contador = 0;
  
  // Simular recepción de la app
  if (BTApp.available()) {
    byte raw = BTApp.read();
    Serial.print(F("App envio: "));
    Serial.println(raw);
    
    // Reenviar a esclavo (como hace tu código real)
    BTApollo.write(raw);
    Serial.println(F("Reenviado a BTApollo"));
  }
  
  if (millis() - ultimo >= 2000) {
    ultimo = millis();
    contador++;
    
    BTApollo.write((byte)254);
    
    Serial.print(F("["));
    Serial.print(millis() / 1000);
    Serial.print(F("s] Pulso #"));
    Serial.print(contador);
    Serial.println(F(" - Ambos LEDs vivos?"));
  }
}
