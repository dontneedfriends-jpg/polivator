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
  display.showStatus(sensor.readPercent(), sensor.readRaw(), false, WiFi.status() == WL_CONNECTED ? "connected" : "disconnected");
}

void loop() {
  static unsigned long lastSensorRead = 0;
  static unsigned long lastDisplayUpdate = 0;
  static float lastMoisture = 0.0f;
  static int lastRaw = 0;
  static String lastWifiStatus = "disconnected";
  static bool calibrating = false;

  // Handle serial commands for calibration
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') {
      calibrating = !calibrating;
      if (calibrating) {
        Serial.println("Calibration mode: ON");
      } else {
        Serial.println("Calibration mode: OFF");
      }
    }
  }

  webServer.handleClient();

  unsigned long now = millis();

  if (now - lastSensorRead >= 2000) {
    lastMoisture = sensor.readPercent();
    lastRaw = sensor.readRaw();
    lastWifiStatus = WiFi.status() == WL_CONNECTED ? "connected" : (WiFi.getMode() == WIFI_AP ? "ap" : "disconnected");
    lastSensorRead = now;
  }

  if (now - lastDisplayUpdate >= 10000) {
    display.showStatus(lastMoisture, lastRaw, calibrating, lastWifiStatus.c_str());
    lastDisplayUpdate = now;
  }

  delay(10);
}
