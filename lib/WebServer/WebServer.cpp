#include "WebServer.h"
#include "../Settings/Settings.h"
#include "../WaterPump/WaterPump.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Sensor.h"
#include "Calibration.h"
#include "Display.h"
#include "index.h"
#include <Update.h>
#include <esp_task_wdt.h>

const char* DEFAULT_AUTH_USERNAME = "admin";
const char* DEFAULT_AUTH_PASSWORD = "admin";

WebServer::WebServer(SensorManager* sensorManager, Calibration* calibration, IDisplay* display, Settings* settings, WaterPump* pump)
  : m_sensorManager(sensorManager), m_calibration(calibration), m_display(display), m_settings(settings), m_pump(pump), server(80) {
}

WebServer::~WebServer() {
}

bool WebServer::begin() {
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
  Serial.println("WebServer::begin OK");
  Serial.println("WebServer started.");
  return true;
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

void WebServer::sendStatusEvent(SensorManager* sensorManager) {
  JsonDocument doc;
  JsonArray sensors = doc["sensors"].to<JsonArray>();
  for (int i = 0; i < MAX_SENSORS; i++) {
    JsonObject sensor = sensors.add<JsonObject>();
    sensor["name"] = m_calibration->getName(i);
    sensor["pin"] = m_calibration->getPin(i);
    sensor["raw"] = sensorManager->getRaw(i);
    sensor["percentage"] = sensorManager->getPercent(i);
    sensor["voltage"] = sensorManager->getVoltage(i);
    sensor["dry"] = m_calibration->getDryValue(i);
    sensor["wet"] = m_calibration->getWetValue(i);
    sensor["enabled"] = m_calibration->getConfigs()[i].enabled;
  }
  doc["wifi"] = WiFi.status() == WL_CONNECTED ? "connected" : (WiFi.getMode() == WIFI_AP ? "ap" : "disconnected");
  if (WiFi.status() == WL_CONNECTED) {
    doc["ssid"] = WiFi.SSID();
  } else {
    doc["ssid"] = preferences.getString("ssid", "");
  }
  if (m_pump) {
    JsonObject pumpObj = doc["pump"].to<JsonObject>();
    pumpObj["running"] = m_pump->isRunning();
    pumpObj["remainingMs"] = m_pump->remainingMs();
    pumpObj["autoMode"] = m_pump->isAutoModeEnabled();
    pumpObj["threshold"] = m_pump->getAutoThreshold();
    pumpObj["flowRate"] = m_pump->getFlowRate();
  }
  String output;
  serializeJson(doc, output);
  events.send(output.c_str(), "status", millis());
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
    esp_task_wdt_reset();
  }
  
  return WiFi.status() == WL_CONNECTED;
}

void WebServer::setupAP() {
  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Polivator-Config");
  
  dnsServer.start(53, "*", apIP);
}

int WebServer::getIndexFromParam(AsyncWebServerRequest *request) {
  if (!request->hasParam("index")) {
    return -1;
  }
  String indexStr = request->getParam("index")->value();
  int index = indexStr.toInt();
  if (index < 0 || index >= MAX_SENSORS) {
    return -1;
  }
  return index;
}

void WebServer::setupRoutes() {
  // Root: serve the web UI page
  server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_content);
    request->send(response);
  });
  
  // GET /api/status
  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    JsonArray sensors = doc["sensors"].to<JsonArray>();
    for (int i = 0; i < MAX_SENSORS; i++) {
      JsonObject sensor = sensors.add<JsonObject>();
      sensor["name"] = m_calibration->getName(i);
      sensor["pin"] = m_calibration->getPin(i);
      sensor["raw"] = m_sensorManager->getRaw(i);
      sensor["percentage"] = m_sensorManager->getPercent(i);
      sensor["voltage"] = m_sensorManager->getVoltage(i);
      sensor["dry"] = m_calibration->getDryValue(i);
      sensor["wet"] = m_calibration->getWetValue(i);
      sensor["enabled"] = m_calibration->getConfigs()[i].enabled;
    }
    doc["wifi"] = WiFi.status() == WL_CONNECTED ? "connected" : (WiFi.getMode() == WIFI_AP ? "ap" : "disconnected");
    serializeJson(doc, *response);
    request->send(response);
  });

  // POST /api/calibrate/dry?index=0
  server.on("/api/calibrate/dry", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    int index = getIndexFromParam(request);
    if (index < 0) {
      request->send(400, "text/plain", "Missing or invalid 'index' query parameter");
      return;
    }
    int raw = m_sensorManager->getRaw(index);
    m_calibration->setDryValue(index, raw);
    m_sensorManager->setCalibration(index, m_calibration->getDryValue(index), m_calibration->getWetValue(index));
    request->send(200, "text/plain", "Dry threshold set for sensor " + String(index));
  });
  
  // POST /api/calibrate/wet?index=0
  server.on("/api/calibrate/wet", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    int index = getIndexFromParam(request);
    if (index < 0) {
      request->send(400, "text/plain", "Missing or invalid 'index' query parameter");
      return;
    }
    int raw = m_sensorManager->getRaw(index);
    m_calibration->setWetValue(index, raw);
    m_sensorManager->setCalibration(index, m_calibration->getDryValue(index), m_calibration->getWetValue(index));
    request->send(200, "text/plain", "Wet threshold set for sensor " + String(index));
  });
  
  // POST /api/calibrate/reset?index=0
  server.on("/api/calibrate/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    int index = getIndexFromParam(request);
    if (index < 0) {
      request->send(400, "text/plain", "Missing or invalid 'index' query parameter");
      return;
    }
    m_calibration->resetToDefaults(index);
    m_sensorManager->setCalibration(index, m_calibration->getDryValue(index), m_calibration->getWetValue(index));
    request->send(200, "text/plain", "Calibration reset for sensor " + String(index));
  });
  
  // POST /api/config/sensor?index=0  (body: {"name": "Sensor1"})
  server.on("/api/config/sensor", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    int index = getIndexFromParam(request);
    if (index < 0) {
      request->send(400, "text/plain", "Missing or invalid 'index' query parameter");
      return;
    }
    // Parse JSON body
    if (!request->contentType().startsWith("application/json")) {
      request->send(400, "text/plain", "Content-Type must be application/json");
      return;
    }
    String body = request->arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      request->send(400, "text/plain", "Invalid JSON");
      return;
    }
    const char* name = doc["name"];
    if (name) {
      m_calibration->setName(index, name);
      request->send(200, "text/plain", "Name set for sensor " + String(index));
      return;
    }
    if (doc["pin"].is<int>()) {
      uint8_t pin = doc["pin"];
      m_calibration->setPin(index, pin);
      m_sensorManager->setCalibration(index, m_calibration->getDryValue(index), m_calibration->getWetValue(index));
      request->send(200, "text/plain", "Pin set for sensor " + String(index));
      return;
    }
    request->send(400, "text/plain", "Missing 'name' or 'pin' field");
  });
  
  // DELETE /api/sensor?index=N
  server.on("/api/sensor", HTTP_DELETE, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    int index = getIndexFromParam(request);
    if (index < 0) {
      request->send(400, "text/plain", "Missing or invalid 'index' query parameter");
      return;
    }
    m_calibration->removeSensor(index);
    m_display->forceFullRefresh();
    request->send(200, "text/plain", "Sensor " + String(index) + " removed");
  });
  
  // POST /api/sensor/enable?index=N
  server.on("/api/sensor/enable", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    int index = getIndexFromParam(request);
    if (index < 0) {
      request->send(400, "text/plain", "Missing or invalid 'index' query parameter");
      return;
    }
    m_calibration->enableSensor(index);
    m_display->forceFullRefresh();
    request->send(200, "text/plain", "Sensor " + String(index) + " enabled");
  });
  
  // GET /api/calibration
  server.on("/api/calibration", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    JsonArray sensors = doc["sensors"].to<JsonArray>();
    for (int i = 0; i < MAX_SENSORS; i++) {
      JsonObject sensor = sensors.add<JsonObject>();
      sensor["name"] = m_calibration->getName(i);
      sensor["dry"] = m_calibration->getDryValue(i);
      sensor["wet"] = m_calibration->getWetValue(i);
    }
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
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
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
  
  // GET /api/wifi
  server.on("/api/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (preferences.getString("ssid", "").length() > 0) {
      if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
        request->requestAuthentication();
        return;
      }
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    doc["ssid"] = preferences.getString("ssid", "");
    serializeJson(doc, *response);
    request->send(response);
  });
  
  // POST /api/config/wifi - accepts form-urlencoded only
  server.on("/api/config/wifi", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (preferences.getString("ssid", "").length() > 0) {
      if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
        request->requestAuthentication();
        return;
      }
    }
    if (!request->hasParam("ssid", true)) {
      request->send(400, "text/plain", "Missing 'ssid' field");
      return;
    }
    String ssid = request->getParam("ssid", true)->value();
    String pass = request->hasParam("pass", true) ? request->getParam("pass", true)->value() : "";
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    request->send(200, "text/plain", "Credentials saved. Rebooting...");
    delay(1000);
    ESP.restart();
  });
  
  // GET /api/settings
  server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    doc["readInterval"] = m_settings->getReadInterval();
    doc["displayInterval"] = m_settings->getDisplayInterval();
    doc["webInterval"] = m_settings->getWebInterval();
    doc["updatePriority"] = m_settings->getUpdatePriority();
    doc["sleepMode"] = m_settings->getSleepMode();
    doc["otaEnabled"] = m_settings->getOtaEnabled();
    doc["debugEnabled"] = m_settings->getDebugEnabled();
    doc["displayUnit"] = m_settings->getDisplayUnit();
    doc["adcSamples"] = m_settings->getAdcSamples();
    serializeJson(doc, *response);
    request->send(response);
  });
  
  // POST /api/settings
  server.on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    if (!request->contentType().startsWith("application/json")) {
      request->send(400, "text/plain", "Content-Type must be application/json");
      return;
    }
    String body = request->arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      request->send(400, "text/plain", "Invalid JSON");
      return;
    }
    if (doc["readInterval"].is<int>()) {
      m_settings->setReadInterval(doc["readInterval"]);
    }
    if (doc["displayInterval"].is<int>()) {
      m_settings->setDisplayInterval(doc["displayInterval"]);
    }
    if (doc["webInterval"].is<int>()) {
      m_settings->setWebInterval(doc["webInterval"]);
    }
    if (doc["updatePriority"].is<int>()) {
      m_settings->setUpdatePriority(doc["updatePriority"]);
    }
    if (doc["sleepMode"].is<int>()) {
      m_settings->setSleepMode(doc["sleepMode"]);
    }
    if (doc["otaEnabled"].is<bool>()) {
      m_settings->setOtaEnabled(doc["otaEnabled"]);
    }
    if (doc["debugEnabled"].is<bool>()) {
      m_settings->setDebugEnabled(doc["debugEnabled"]);
    }
    if (doc["displayUnit"].is<int>()) {
      m_settings->setDisplayUnit(doc["displayUnit"]);
    }
    if (doc["adcSamples"].is<int>()) {
      m_settings->setAdcSamples(doc["adcSamples"]);
    }
    request->send(200, "text/plain", "Settings saved");
  });
  
  // POST /api/pump/run  body: {"seconds": 5}
  server.on("/api/pump/run", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    if (!m_pump) {
      request->send(503, "text/plain", "Pump not initialized");
      return;
    }
    if (!request->contentType().startsWith("application/json")) {
      request->send(400, "text/plain", "Content-Type must be application/json");
      return;
    }
    String body = request->arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      request->send(400, "text/plain", "Invalid JSON");
      return;
    }
    float seconds = doc["seconds"] | 0.0f;
    if (seconds < 0) seconds = 0;
    if (seconds > 300) seconds = 300; // safety cap: 5 minutes
    uint32_t durationMs = (uint32_t)(seconds * 1000.0f);
    m_pump->run(durationMs);
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument out;
    out["running"] = m_pump->isRunning();
    out["durationMs"] = durationMs;
    out["flowRate"] = m_pump->getFlowRate();
    out["volumeMl"] = (float)durationMs / 1000.0f * m_pump->getFlowRate();
    serializeJson(out, *response);
    request->send(response);
  });

  // POST /api/pump/stop
  server.on("/api/pump/stop", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    if (m_pump) m_pump->stop();
    request->send(200, "text/plain", "Pump stopped");
  });

  // GET /api/pump  -> status + settings
  server.on("/api/pump", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    doc["running"] = m_pump && m_pump->isRunning();
    doc["remainingMs"] = m_pump ? m_pump->remainingMs() : 0;
    doc["autoMode"] = m_pump && m_pump->isAutoModeEnabled();
    doc["threshold"] = m_pump ? m_pump->getAutoThreshold() : 0;
    doc["flowRate"] = m_pump ? m_pump->getFlowRate() : 0.0f;
    serializeJson(doc, *response);
    request->send(response);
  });

  // POST /api/pump/config  body: {"autoMode": true, "threshold": 30, "flowRate": 5.0}
  server.on("/api/pump/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    if (!m_pump) {
      request->send(503, "text/plain", "Pump not initialized");
      return;
    }
    if (!request->contentType().startsWith("application/json")) {
      request->send(400, "text/plain", "Content-Type must be application/json");
      return;
    }
    String body = request->arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      request->send(400, "text/plain", "Invalid JSON");
      return;
    }
    if (doc["autoMode"].is<bool>()) m_pump->setAutoModeEnabled(doc["autoMode"]);
    if (doc["threshold"].is<int>()) m_pump->setAutoThreshold(doc["threshold"]);
    if (doc["flowRate"].is<float>() || doc["flowRate"].is<int>()) m_pump->setFlowRate(doc["flowRate"]);
    request->send(200, "text/plain", "Pump config saved");
  });

  // POST /api/pump/calibrate  body: {"seconds": 10, "measuredMl": 50}
  // Запускает мотор на seconds секунд, после возврата сохраняет flowRate = measuredMl / seconds.
  server.on("/api/pump/calibrate", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    if (!m_pump) {
      request->send(503, "text/plain", "Pump not initialized");
      return;
    }
    if (!request->contentType().startsWith("application/json")) {
      request->send(400, "text/plain", "Content-Type must be application/json");
      return;
    }
    String body = request->arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
      request->send(400, "text/plain", "Invalid JSON");
      return;
    }
    float seconds = doc["seconds"] | 0.0f;
    float measuredMl = doc["measuredMl"] | 0.0f;
    if (seconds < 1.0f) seconds = 1.0f;
    if (seconds > 60.0f) seconds = 60.0f;
    if (measuredMl <= 0.0f) {
      request->send(400, "text/plain", "measuredMl must be > 0");
      return;
    }
    m_pump->run((uint32_t)(seconds * 1000.0f));
    float newFlow = measuredMl / seconds;
    m_pump->setFlowRate(newFlow);
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument out;
    out["ok"] = true;
    out["flowRate"] = newFlow;
    out["durationSec"] = seconds;
    out["measuredMl"] = measuredMl;
    out["note"] = "Pump started, fill a measuring cup for 'seconds' seconds, then re-run with measuredMl.";
    serializeJson(out, *response);
    request->send(response);
  });

  // OTA firmware update endpoint
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
      request->requestAuthentication();
      return;
    }
    // Will be handled by onUpload
  }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    this->handleUpdate(request, filename, index, data, len, final);
  });
  
  // Handle 404
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
  
  // SSE events endpoint (auth-protected via Authorization header parsed from request)
  events.onConnect([](AsyncEventSourceClient *client) {
    if (client->lastId()) {
      Serial.printf("Client reconnected! Last message ID: %u\n", client->lastId());
    }
    client->send("hello", NULL, millis(), 1000);
  });
  server.addHandler(&events);
}

void WebServer::handleUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // Re-check auth on body (in case the headers were skipped on the upload)
  if (!request->authenticate(DEFAULT_AUTH_USERNAME, DEFAULT_AUTH_PASSWORD)) {
    request->requestAuthentication();
    return;
  }

  // Abort any in-progress update if it takes too long (10 min)
  if (index == 0) {
    uploadStartTime = millis();
    if (updateInProgress) {
      Update.abort();
    }
    updateInProgress = true;
    Serial.printf("Update Start: %s\n", filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
      updateInProgress = false;
      request->send(400, "text/plain", "OTA update begin failed");
      return;
    }
  }
  if (updateInProgress && millis() - uploadStartTime > 600000UL) {
    Update.abort();
    updateInProgress = false;
    request->send(400, "text/plain", "OTA update timed out");
    return;
  }
  if (Update.write(data, len) != len) {
    Update.printError(Serial);
    Update.abort();
    updateInProgress = false;
    request->send(400, "text/plain", "OTA update write failed");
    return;
  }
  if (final) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\n", index + len);
      updateInProgress = false;
      request->send(200, "text/plain", "Update successful. Rebooting...");
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
      updateInProgress = false;
      request->send(400, "text/plain", "OTA update end failed");
    }
  }
}
