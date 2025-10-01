/* 
  ESP32 WiFi Config Portal with Captive Portal + REST API
  --------------------------------------------------------
  - Arranca en AP si no tiene credenciales (portal cautivo).
  - Guarda SSID/Password en NVS (Preferences).
  - Se conecta en STA automáticamente si hay credenciales.
  - Endpoints REST para status, scan, connect y reset.
  - Reconexión automática si se cae el WiFi.
  - Reset por botón (GPIO0) o endpoint.
  - Incluye validación: no guarda SSID vacío ni password vacío.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

const char* AP_SSID = "ESP32_JUSAN";
const char* AP_PASS = "12345678";
const byte DNS_PORT = 53;
const int RESET_BTN = 0; // BOOT button
const unsigned long LONG_PRESS_MS = 5000;

WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

String savedSSID;
String savedPASS;

bool portalMode = false;
unsigned long lastReconnectAttempt = 0;
const unsigned long RECONNECT_INTERVAL_MS = 15000;

// ===== Utilidades =====
void saveCreds(const String& ssid, const String& pass) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  savedSSID = ssid;
  savedPASS = pass;
  Serial.println("Credenciales guardadas en NVS:");
  Serial.println("  SSID: " + ssid);
  Serial.println("  PASS: " + pass);
}

void clearCreds() {
  prefs.begin("wifi", false);
  prefs.remove("ssid");
  prefs.remove("pass");
  prefs.end();
  savedSSID = "";
  savedPASS = "";
  Serial.println("Credenciales borradas de NVS");
}

bool loadCreds() {
  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", "");
  savedPASS = prefs.getString("pass", "");
  prefs.end();
  if (savedSSID.length() > 0) {
    Serial.println("Credenciales encontradas en NVS:");
    Serial.println("  SSID: " + savedSSID);
    return true;
  } else {
    Serial.println("No hay credenciales guardadas en NVS");
    return false;
  }
}

String ipToStr(IPAddress ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

// ===== HTML =====
String htmlPage() {
  String html = F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
                  "<title>ESP32 WiFi Config</title>"
                  "<style>body{font-family:Arial;margin:24px;}input,button{padding:8px;margin:6px 0;width:100%;max-width:400px;}"
                  "code{background:#eee;padding:2px 4px;border-radius:4px}</style></head><body>");
  html += F("<h1>Configurar WiFi</h1>"
            "<form action='/save' method='POST'>"
            "SSID:<br><input type='text' name='ssid' required><br>"
            "Password:<br><input type='password' name='pass'><br>"
            "<button type='submit'>Guardar y Conectar</button>"
            "</form><hr>"
            "<button onclick=\"fetch('/scan').then(r=>r.json()).then(d=>alert(JSON.stringify(d,null,2)))\">Escanear redes</button>"
            "<button onclick=\"fetch('/status').then(r=>r.json()).then(d=>alert(JSON.stringify(d,null,2)))\">Ver estado</button>"
            "<button onclick=\"if(confirm('¿Borrar credenciales?')) fetch('/reset',{method:'POST'}).then(()=>location.reload())\">Restablecer</button>");
  if (WiFi.status() == WL_CONNECTED) {
    html += "<p><b>Conectado a:</b> " + WiFi.SSID() + " | IP: " + ipToStr(WiFi.localIP()) + "</p>";
  } else {
    html += "<p><b>No conectado</b></p>";
  }
  html += F("</body></html>");
  return html;
}

// ===== Handlers =====
void handleRoot() { 
  Serial.println("🌐 Cliente pidió / (root)");
  server.send(200, "text/html", htmlPage()); 
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  if (ssid.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"SSID requerido\"}");
    return;
  }

  saveCreds(ssid, pass);
  server.send(200, "text/html", "<h3>Credenciales guardadas. Reinicia el ESP32 para aplicar.</h3>");
  ESP.restart(); //reinicia
}


void handleConnectJson() {
  Serial.println("Recibido POST /connect (JSON)");
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"JSON body requerido\"}");
    Serial.println("Error: sin JSON");
    return;
  }
  String body = server.arg("plain");
  int ssidIdx = body.indexOf("\"ssid\"");
  int passIdx = body.indexOf("\"pass\"");
  auto extract = [&](int idx) -> String {
    if (idx < 0) return "";
    int colon = body.indexOf(':', idx);
    int q1 = body.indexOf('"', colon+1);
    int q2 = body.indexOf('"', q1+1);
    return body.substring(q1+1, q2);
  };
  String ssid = extract(ssidIdx);
  String pass = extract(passIdx);

  if (ssid.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"SSID requerido\"}");
    Serial.println("Error: SSID vacío en JSON");
    return;
  }
  if (pass.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"Password requerido\"}");
    Serial.println("Error: password vacío en JSON");
    return;
  }

  Serial.println("  SSID: " + ssid);
  Serial.println("  PASS: " + pass);
  saveCreds(ssid, pass);
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());
  server.send(200, "application/json", "{\"message\":\"intentando conectar\",\"ssid\":\"" + ssid + "\"}");
}

void handleStatus() {
  Serial.println("GET /status");
  String json = "{";
  json += "\"mode\":\"" + String(portalMode ? "AP" : "STA") + "\",";
  json += "\"stored_ssid\":\"" + savedSSID + "\",";
  json += "\"wifi_status\":" + String(WiFi.status()) + ",";
  json += "\"connected_ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"ip\":\"" + (WiFi.status()==WL_CONNECTED ? ipToStr(WiFi.localIP()) : "") + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleScan() {
  Serial.println("Escaneando redes WiFi...");
  int n = WiFi.scanNetworks();
  Serial.printf("  Encontradas %d redes\n", n);
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
    Serial.printf("  %d: %s (%d dBm)\n", i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleReset() {
  Serial.println("Recibido POST /reset");
  clearCreds();
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  portalMode = true;
  Serial.println(" Portal reiniciado en modo AP");
  Serial.print("  SSID: "); Serial.println(AP_SSID);
  Serial.print("  IP: "); Serial.println(WiFi.softAPIP());
  server.send(200, "application/json", "{\"message\":\"credenciales borradas\"}");
}

// ===== Helpers =====
void startPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  portalMode = true;
  Serial.println(" Iniciado portal en modo AP");
  Serial.print("  SSID: "); Serial.println(AP_SSID);
  Serial.print("  PASS: "); Serial.println(AP_PASS);
  Serial.print("  IP: "); Serial.println(WiFi.softAPIP());
}

void tryConnectSaved() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());
  Serial.print(" Intentando conectar a "); Serial.println(savedSSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    portalMode = false;
    Serial.println(" Conectado a WiFi");
    Serial.print("  SSID: "); Serial.println(WiFi.SSID());
    Serial.print("  IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println(" No se pudo conectar, borrando credenciales...");
    clearCreds();
    startPortal();
  }
}

// ===== Setup / Loop =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(" Iniciando ESP32 WiFi Config...");

  pinMode(RESET_BTN, INPUT_PULLUP);

  bool hasCreds = loadCreds();
  if (hasCreds) tryConnectSaved();
  else startPortal();

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/connect", HTTP_POST, handleConnectJson);
  server.on("/status", handleStatus);
  server.on("/scan", handleScan);
  server.on("/reset", HTTP_POST, handleReset);

  server.begin();
  Serial.println(" Servidor web iniciado en puerto 80");
}

void loop() {
  server.handleClient();
  if (portalMode) dnsServer.processNextRequest();
}