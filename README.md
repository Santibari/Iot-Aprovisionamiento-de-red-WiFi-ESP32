# Proyecto IoT: Aprovisionamiento WiFi con ESP32

Este proyecto implementa una solución IoT basada en el microcontrolador **ESP32** que permite la configuración dinámica de la red WiFi sin necesidad de reprogramar el dispositivo. Utiliza un portal cautivo local para que el usuario final pueda ingresar el SSID y la contraseña de su red.

## 🧰 Requisitos del sistema

- Microcontrolador ESP32
- Entorno de desarrollo Arduino IDE
- Librerías: WiFi.h, WebServer.h, Preferences.h
- Conexión serial para monitoreo
- Navegador web para acceder al portal cautivo

## ⚙️ Funcionalidades

- Inicio en modo AP si no hay credenciales guardadas
- Interfaz web para ingresar SSID y contraseña
- Almacenamiento de credenciales en memoria no volátil (Preferences)
- Reconexión automática a la red configurada
- Botón físico para restablecer configuración
- Documentación técnica y diagramas UML
- Endpoints documentados y colección Postman

## 🚀 Instrucciones de uso

1. Subir el código al ESP32 desde Arduino IDE
2. Conectarse al punto de acceso `ESP32_Config` con contraseña `12345678`
3. Acceder a `http://192.168.4.1` desde el navegador
4. Ingresar SSID y contraseña de la red WiFi
5. El ESP32 intentará conectarse y mostrará el resultado por serial

![Untitled](https://github.com/user-attachments/assets/09b0a8db-fcc8-4d83-9ec4-39001af1038c)

