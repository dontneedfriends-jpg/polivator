#include <Arduino.h>
#include <WiFi.h>
#include "Sensor.h"
#include "Calibration.h"
#include "Display.h"
#include "WebServer.h"

Sensor sensor(4);
Calibration calibration;
Display display;
WebServer webServer(&sensor, &calibration, &display);

void setup() {
  Serial.begin(115200);
  sensor.begin();
  calibration.begin();
  display.begin();
  // Load calibration into sensor
  sensor.setCalibration(calibration.getDryValue(), calibration.getWetValue());
  webServer.begin();
  display.update(sensor.readPercent(), sensor.readRaw(), WiFi.status() == WL_CONNECTED ? "connected" : "disconnected");
}

void loop() {
  static unsigned long lastSensorRead = 0;
  static unsigned long lastDisplayUpdate = 0;
  static int lastMoisture = 0;
  static int lastRaw = 0;
  static String lastWifiStatus = "disconnected";

  webServer.handleClient();

  unsigned long now = millis();

  if (now - lastSensorRead >= 2000) {
    lastMoisture = sensor.readPercent();
    lastRaw = sensor.readRaw();
    lastWifiStatus = WiFi.status() == WL_CONNECTED ? "connected" : (WiFi.getMode() == WIFI_AP ? "ap" : "disconnected");
    lastSensorRead = now;
  }

  if (now - lastDisplayUpdate >= 10000) {
    display.update(lastMoisture, lastRaw, lastWifiStatus);
    lastDisplayUpdate = now;
  }
}
