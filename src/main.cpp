#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include "Sensor.h"
#include "Calibration.h"
#include "Display.h"
#include "WebServer.h"
#include "Settings.h"
#include "Common/Types.h"

// Пять датчиков на GPIO2, GPIO12-15 (ADC1, свободные от дисплея и Serial)
// Дисплей занимает GPIO4 (BUSY), GPIO5 (CS), GPIO16 (RST), GPIO17 (DC), GPIO18 (SCK), GPIO21 (MOSI).
// GPIO1 и GPIO3 используются Serial/UART.
const uint8_t sensorPins[MAX_SENSORS] = {2, 12, 13, 14, 15};

Calibration calibration;
SensorManager sensorManager;
Display display;
Settings settings;
WebServer webServer(&sensorManager, &calibration, &display, &settings);

bool calibrating = false;
uint8_t activeSensorForSerial = 0; // индекс датчика для serial-калибровки
static bool forceDisplayUpdate = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  esp_task_wdt_reset();
  Serial.println("\n=== Polivator Multi-Sensor ===");
  Serial.print("Sensor pins: ");
  for (int i = 0; i < MAX_SENSORS; i++) {
    if (i > 0) Serial.print(", ");
    Serial.print(sensorPins[i]);
  }
  Serial.println();
  
  calibration.begin(sensorPins, MAX_SENSORS);
  esp_task_wdt_reset();
  sensorManager.begin(&calibration, &settings);
  esp_task_wdt_reset();
  display.begin();
  esp_task_wdt_reset();
  webServer.begin();
  esp_task_wdt_reset();
  ArduinoOTA.begin();
  esp_task_wdt_reset();
  
  // Отобразить статус после запуска
  sensorManager.readAll();
  {
    uint8_t count = sensorManager.getCount();
    const SensorConfig* configs = calibration.getConfigs();
    int percentages[MAX_SENSORS];
    int rawValues[MAX_SENSORS];
    for (uint8_t i = 0; i < count; i++) {
      percentages[i] = sensorManager.getPercent(i);
      rawValues[i] = sensorManager.getRaw(i);
    }
    const char* wifiStatus = WiFi.status() == WL_CONNECTED ? "connected" : (WiFi.getMode() == WIFI_AP ? "ap" : "disconnected");
    display.showStatus(count, configs, percentages, rawValues, wifiStatus);
  }
  esp_task_wdt_reset();
  Serial.println("Setup complete.");
}

void handleSerialCommands() {
  if (!Serial.available()) return;
  char c = Serial.read();
  
  if (c == 'c' || c == 'C') {
    calibrating = !calibrating;
    if (calibrating) {
      Serial.println("Calibration mode ON. Select sensor: 0-" + String(MAX_SENSORS-1));
      Serial.println("Send 0..4 to select sensor, then 'd' dry, 'w' wet, 'r' reset.");
      display.showCalibrationScreen("Calibration mode");
    } else {
      Serial.println("Calibration mode OFF");
    }
    return;
  }
  
  if (!calibrating) return;
  
  // Выбор датчика цифрой 0-4
  if (c >= '0' && c <= '4') {
    activeSensorForSerial = c - '0';
    Serial.print("Selected sensor #");
    Serial.println(activeSensorForSerial);
    char buf[32];
    snprintf(buf, sizeof(buf), "Sensor %u", activeSensorForSerial);
    display.showMessage(buf);
    return;
  }
  
  uint8_t idx = activeSensorForSerial;
  
  if (c == 'd') {
    int raw = sensorManager.getRaw(idx);
    calibration.setDryValue(idx, raw);
    sensorManager.setCalibration(idx, calibration.getDryValue(idx), calibration.getWetValue(idx));
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "Dry set S%u", idx);
      display.showMessage(buf);
    }
    forceDisplayUpdate = true;
    Serial.print("Dry calibration set for sensor ");
    Serial.println(idx);
  } else if (c == 'w') {
    int raw = sensorManager.getRaw(idx);
    calibration.setWetValue(idx, raw);
    sensorManager.setCalibration(idx, calibration.getDryValue(idx), calibration.getWetValue(idx));
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "Wet set S%u", idx);
      display.showMessage(buf);
    }
    forceDisplayUpdate = true;
    Serial.print("Wet calibration set for sensor ");
    Serial.println(idx);
  } else if (c == 'r') {
    calibration.resetToDefaults(idx);
    sensorManager.setCalibration(idx, calibration.getDryValue(idx), calibration.getWetValue(idx));
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "Reset S%u", idx);
      display.showMessage(buf);
    }
    forceDisplayUpdate = true;
    Serial.print("Calibration reset for sensor ");
    Serial.println(idx);
  } else if (c == 's') {
    // Показать статус всех датчиков
    for (int i = 0; i < MAX_SENSORS; i++) {
      Serial.print("S");
      Serial.print(i);
      Serial.print(": raw=");
      Serial.print(sensorManager.getRaw(i));
      Serial.print(" %=");
      Serial.print(sensorManager.getPercent(i));
      Serial.print(" dry=");
      Serial.print(calibration.getDryValue(i));
      Serial.print(" wet=");
      Serial.print(calibration.getWetValue(i));
      Serial.print(" pin=");
      Serial.println(calibration.getPin(i));
    }
  }
}

void loop() {
  esp_task_wdt_reset();
  static unsigned long lastSensorRead = 0;
  static unsigned long lastDisplayUpdate = 0;
  
  handleSerialCommands();
  webServer.handleClient();
  ArduinoOTA.handle();
  esp_task_wdt_reset();
  
  unsigned long now = millis();
  
  // Чтение датчиков с заданным интервалом
  unsigned long readIntervalMs = (unsigned long)settings.getReadInterval() * 1000UL;
  if (now - lastSensorRead >= readIntervalMs) {
    sensorManager.readAll();
    // Отправить SSE
    webServer.sendStatusEvent(&sensorManager);
    lastSensorRead = now;
    esp_task_wdt_reset();
  }
  
  // Обнаружение изменений конфигурации (калибровочных значений)
  static bool configInit = false;
  static int prevDry[MAX_SENSORS];
  static int prevWet[MAX_SENSORS];
  if (!configInit) {
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
      prevDry[i] = calibration.getDryValue(i);
      prevWet[i] = calibration.getWetValue(i);
    }
    configInit = true;
  }
  bool configChanged = false;
  for (uint8_t i = 0; i < MAX_SENSORS; i++) {
    if (calibration.getDryValue(i) != prevDry[i] || calibration.getWetValue(i) != prevWet[i]) {
      configChanged = true;
      prevDry[i] = calibration.getDryValue(i);
      prevWet[i] = calibration.getWetValue(i);
    }
  }
  if (configChanged) {
    forceDisplayUpdate = true;
  }
  
  // Обновление дисплея с заданным интервалом или при принудительном обновлении
  unsigned long displayIntervalMs = (unsigned long)settings.getDisplayInterval() * 1000UL;
  if (now - lastDisplayUpdate >= displayIntervalMs || forceDisplayUpdate) {
    forceDisplayUpdate = false;
    if (calibrating) {
      display.showCalibrationScreen("Calibration mode");
    } else {
      uint8_t count = sensorManager.getCount();
      const SensorConfig* configs = calibration.getConfigs();
      int percentages[MAX_SENSORS];
      int rawValues[MAX_SENSORS];
      for (uint8_t i = 0; i < count; i++) {
        percentages[i] = sensorManager.getPercent(i);
        rawValues[i] = sensorManager.getRaw(i);
      }
      const char* wifiStatus = WiFi.status() == WL_CONNECTED ? "connected" : (WiFi.getMode() == WIFI_AP ? "ap" : "disconnected");
      display.showStatus(count, configs, percentages, rawValues, wifiStatus);
    }
    lastDisplayUpdate = now;
    esp_task_wdt_reset();
  }
  
  delay(10);
}
