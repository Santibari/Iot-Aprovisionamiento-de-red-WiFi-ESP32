# Iot-Aprovisionamiento-de-red-WiFi-ESP32
# ESP32 WiFi Config Portal

Permite configurar SSID y Password desde un portal cautivo, guardar en memoria NVS y reconectarse automáticamente.

## Funcionalidad
- Arranca en AP (`ESP32_Config` / `12345678`) si no hay credenciales.
- Portal HTML en `http://192.168.4.1`.
- Guarda SSID/Password en NVS.
- Conecta automáticamente en STA.
- Endpoints REST (`/status`, `/scan`, `/connect`, `/reset`).
- Reset por botón (GPIO0) o endpoint.

## Endpoints
- `GET /` → Portal HTML
- `POST /save` → Guarda credenciales (form)
- `POST /connect` → Guarda credenciales (JSON)
- `GET /status` → Estado en JSON
- `GET /scan` → Redes disponibles
- `POST /reset` → Borrar credenciales y volver a AP

## Cómo usar
1. Sube `main.ino` al ESP32.
2. Conéctate al AP `ESP32_Config`.
3. Abre `http://192.168.4.1`.
4. Ingresa SSID y Password.
5. Revisa con `GET /status` si conectó.
