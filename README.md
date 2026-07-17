# Marcador de 24 y 14 Segundos - Básquetbol

Sistema inalámbrico de grado profesional para baloncesto, compuesto por una aplicación de control móvil y dos tableros LED independientes sincronizados por Bluetooth.

## Arquitectura del Sistema

El proyecto está diseñado bajo una topología de red simple pero robusta, dividida en tres componentes:

1. **Control Remoto (App Android):** Interfaz desarrollada en Flutter. Envía comandos ligeros (1 byte) para minimizar latencias. Maneja inicio, pausa, reinicio de tiempos y posee lógica de sincronización para evitar desfasajes si la app se suspende.
2. **Tablero Maestro (Arduino):** El cerebro del sistema. Escucha los comandos del teléfono, lleva el conteo principal del tiempo, actualiza su panel LED, dispara su relé (bocina) y retransmite las órdenes en tiempo real al tablero esclavo.
3. **Tablero Esclavo (Arduino):** Obedece las órdenes del maestro para espejar el tiempo. Incorpora una lógica "failsafe" (reloj interno de contingencia): si la red sufre interferencias por una fracción de segundo, el esclavo sigue contando y activa su propia bocina para no desfasar los tiempos del partido de cara al público.

## Funcionamiento Técnico (No-Blocking)

Ambos nodos utilizan máquinas de estado asíncronas para el manejo de las interrupciones gráficas y los relés. 
**No se utiliza la función `delay()`**. Esto significa que durante la animación de fin de tiempo o el sonido de la bocina, el microcontrolador no se congela y puede procesar instantáneamente cualquier corrección o reseteo enviado desde la mesa de control.

## Diagrama de Conexiones

### Nodo Maestro
* **Panel LED (DMD P1 / SPI):** Pines D6, D7, D8, D9, D11, D13.
* **Bluetooth 1 (Conexión con Celular):**
  * RX -> Pin D4
  * TX -> Pin D5 (Requiere divisor de voltaje resistivo 5V a 3.3V)
* **Bluetooth 2 (Transmisión a Esclavo):**
  * RX -> Pin D2
  * TX -> Pin D3
* **Módulo de Relé (Bocina):** Pin A0 (Lógica Inversa / Activo en LOW).

### Nodo Esclavo
* **Panel LED (DMD P1 / SPI):** Pines D6, D7, D8, D9, D11, D13.
* **Bluetooth (Recepción desde Maestro):**
  * RX -> Pin D2
  * TX -> Pin D3 (Requiere divisor de voltaje resistivo)
* **Módulo de Relé (Bocina):** Pin A0 (Lógica Inversa / Activo en LOW).

## Despliegue y Compilación

**Para los microcontroladores:**
Los ficheros `.ino` para el Maestro y el Esclavo están en la carpeta `arduino_v5/`. Se requieren las librerías `TimerOne` y `DMD` estándar.

**Para la aplicación:**
El proyecto móvil está en la raíz. Para generar el ejecutable de distribución:
```bash
flutter build apk
```
Existe una versión pre-compilada disponible directamente en el directorio `Releases/`.
