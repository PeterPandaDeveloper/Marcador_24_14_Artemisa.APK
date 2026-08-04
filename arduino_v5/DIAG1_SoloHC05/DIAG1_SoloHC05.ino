/*=============================================================================
  DIAGNÓSTICO #1: ¿EL HC-05 BTAPOLLO ESTÁ VIVO?
  
  Este sketch prueba SOLAMENTE el HC-05 de BTApollo (pines 2,3).
  SIN DMD, SIN Timer1, SIN el otro Bluetooth, SIN relé, SIN nada más.
  
  INSTRUCCIONES:
  1. Desconecta físicamente los paneles DMD
  2. Desconecta el HC-05 de la App (BTApp)
  3. Sube este sketch al Arduino MAESTRO
  4. Abre el Monitor Serial a 9600
  5. Observa el LED del HC-05 BTApollo
  
  RESULTADO ESPERADO:
  - El monitor serial imprime cada 2 segundos
  - El LED del HC-05 parpadea normalmente (buscando conexión) o
    doble parpadeo (conectado al esclavo)
  
  SI EL LED MUERE EN ~5 SEGUNDOS: → El módulo HC-05 está DAÑADO o
    hay un problema de cableado (EN/KEY flotante, soldadura fría, etc.)
  
  SI EL LED SIGUE VIVO INDEFINIDAMENTE: → El hardware está bien,
    el problema es la interacción con el DMD u otro componente.
    Pasa al diagnóstico #2.
=============================================================================*/

#include <SoftwareSerial.h>

SoftwareSerial BTApollo(2, 3);  // RX:2, TX:3

void setup() {
  Serial.begin(9600);
  BTApollo.begin(9600);
  
  Serial.println(F("========================================"));
  Serial.println(F("  DIAG #1: SOLO HC-05 BTApollo"));
  Serial.println(F("  Sin DMD, sin Timer1, sin nada mas"));
  Serial.println(F("  Observa: el LED del HC-05 vive o muere?"));
  Serial.println(F("========================================"));
}

void loop() {
  static unsigned long ultimo = 0;
  static int contador = 0;
  
  if (millis() - ultimo >= 2000) {
    ultimo = millis();
    contador++;
    
    // Enviar un byte al HC-05 (como lo haría normalmente)
    BTApollo.write((byte)254);
    
    Serial.print(F("["));
    Serial.print(millis() / 1000);
    Serial.print(F("s] Pulso #"));
    Serial.print(contador);
    Serial.print(F(" enviado. LED del HC-05 BTApollo: "));
    
    if (contador <= 3) {
      Serial.println(F("Observando..."));
    } else if (contador <= 5) {
      Serial.println(F("** Si murio aqui, es HARDWARE **"));
    } else {
      Serial.println(F("VIVO! El modulo funciona bien."));
    }
  }
}
