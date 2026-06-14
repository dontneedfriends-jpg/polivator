#include "WebServer.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Sensor.h"
#include "Calibration.h"
#include "Display.h"

WebServer::WebServer(Sensor* sensor, Calibration* calibration, Display* display)
  : m_sensor(sensor), m_calibration(calibration), m_display(display), server(80) {
}

WebServer::~WebServer() {
}

void WebServer::begin() {
  Serial.println("Starting WebServer...");
  preferences.begin("wifi", false);
  
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  
  if (ssid.length() > 0 && connectToSavedWiFi()) {
    Serial.println("Connected to saved WiFi.");
  } else {
    setupAP();
  }
  
  setupRoutes();
  server.begin();
  Serial.println("WebServer started.");
}

void WebServer::handleClient() {
  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
  }
}

void WebServer::stop() {
  server.end();
  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
  } else {
    WiFi.disconnect();
  }
}

bool WebServer::connectToSavedWiFi() {
  String ssid = preferences.getString("ssid", "");
  String pass = preferences.getString("pass", "");
  if (ssid.length() == 0) return false;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(100);
  }
  
  return WiFi.status() == WL_CONNECTED;
}

void WebServer::setupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Polivator-Config");
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  dnsServer.start(53, "*", apIP);
}

void WebServer::setupRoutes() {
  // Root: redirect to /wifi when in AP mode, else show status
  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (WiFi.getMode() == WIFI_AP) {
      request->redirect("/wifi");
    } else {
      request->redirect("/api/status");
    }
  });
  
  // GET /api/status
  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc(256);
    doc["moisture"] = m_sensor->readPercent();
    doc["raw"] = m_sensor->readRaw();
    doc["voltage"] = m_sensor->readVoltage();
    doc["dry"] = m_calibration->getDryValue();
    doc["wet"] = m_calibration->getWetValue();
    doc["wifi"] = WiFi.status() == WL_CONNECTED ? "connected" : (WiFi.getMode() == WIFI_AP ? "ap" : "disconnected");
    serializeJson(doc, *response);
    request->send(response);
  });
  
  // POST /api/calibrate/dry
  server.on("/api/calibrate/dry", HTTP_POST, [this](AsyncWebServerRequest *request) {
    int raw = m_sensor->readRaw();
    m_calibration->setDryValue(raw);
    request->send(200, "text/plain", "Dry threshold set");
  });
  
  // POST /api/calibrate/wet
  server.on("/api/calibrate/wet", HTTP_POST, [this](AsyncWebServerRequest *request) {
    int raw = m_sensor->readRaw();
    m_calibration->setWetValue(raw);
    request->send(200, "text/plain", "Wet threshold set");
  });
  
  // POST /api/calibrate/reset
  server.on("/api/calibrate/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
    m_calibration->resetToDefaults();
    request->send(200, "text/plain", "Calibration reset");
  });
  
  // GET /api/calibration
  server.on("/api/calibration", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    DynamicJsonDocument doc(128);
    doc["dry"] = m_calibration->getDryValue();
    doc["wet"] = m_calibration->getWetValue();
    serializeJson(doc, *response);
    request->send(response);
  });
  
  // GET /wifi - captive portal page
  server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Polivator WiFi Config</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: Arial; text-align: center; margin: 20px; }
input { width: 90%; padding: 8px; margin: 10px 0; }
button { padding: 10px 20px; background: #4CAF50; color: white; border: none; cursor: pointer; }
</style>
</head>
<body>
<h2>WiFi Configuration</h2>
<form action="/wifi/connect" method="POST">
<label for="ssid">SSID:</label><br>
<input type="text" id="ssid" name="ssid" required><br>
<label for="password">Password:</label><br>
<input type="password" id="password" name="password"><br>
<button type="submit">Save and Connect</button>
</form>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });
  
  // POST /wifi/connect
  server.on("/wifi/connect", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String pass = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";
      preferences.putString("ssid", ssid);
      preferences.putString("pass", pass);
      request->send(200, "text/plain", "Credentials saved. Rebooting...");
      delay(1000);
      ESP.restart();
    } else {
      request->send(400, "text/plain", "Missing ssid");
    }
  });
  
  // Handle 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
  
  // SSE events endpoint (optional)
  events.onConnect([](AsyncEventSourceClient *client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID: %u\n", client->lastId());
    }
    client->send("hello", NULL, millis(), 1000);
  });
  server.addHandler(&events);
}
