# Contador 24s / 14s Sincrónico - Versión 1.0.0

🎉 ¡Primera versión oficial del Sistema Maestro-Esclavo (Artemisa/Apollo) con integración móvil!

Esta versión incluye la arquitectura a prueba de fallos, diseño de interfaz dinámica Pressman y soporte para íconos redondeados nativos.

## 📦 Descargas (APKs Disponibles)

Para maximizar el rendimiento y reducir el peso (de 47 MB a tan solo ~15 MB), hemos dividido la aplicación en dos versiones según el procesador del teléfono:

1. **`Contador24s_Legacy_32bit.apk` (Recomendada para teléfonos antiguos)**
   - **Arquitectura:** `armeabi-v7a` (32 bits).
   - **Dispositivos Legacy:** Teléfonos de gamas de entrada antiguas, dispositivos con Android 10 o inferior, tablets económicas, y modelos como el *Samsung Core 01*, *Samsung J series*, *Moto E antiguos*. 
   - **Cuándo usarla:** Si tu teléfono es un modelo básico o de hace varios años, descarga esta versión para garantizar 100% de compatibilidad.

2. **`Contador24s_Modern_64bit.apk` (Recomendada para teléfonos actuales)**
   - **Arquitectura:** `arm64-v8a` (64 bits).
   - **Dispositivos Modernos:** Cualquier teléfono gama media o alta desde el 2016 en adelante (Ej: *Samsung S series, A series modernos, Xiaomi Redmi/Poco, Pixel, etc.*).
   - **Cuándo usarla:** Si tienes un dispositivo reciente, esta versión aprovechará al máximo la velocidad de tu procesador.

---

### 🚀 Novedades de esta Versión:
- **Rediseño UI:** Contador centralizado y adaptable a cualquier tamaño de pantalla.
- **Feedback Visual:** El ícono de Bluetooth reacciona al estado de conexión.
- **Protocolo Inalámbrico Optimizado:** Nuevo comando silencioso (`252`) para evitar bocinazos accidentales al reiniciar el contador desde la app.
- **Ícono Nativo:** Fondo negro sólido con logo perfectamente centrado y escalado para Android.
