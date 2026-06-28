#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Sensor.h"
#include "Calibration.h"
#include "WebServer.h"
#include "Settings.h"
#include "WaterPump.h"
#include "Common/Types.h"

// ─── Board-specific includes ──────────────────────────────────────────────
#ifdef USE_CARDPUTER
#include <M5Cardputer.h>
#include "DisplayCardputer.h"
#include "Ads1115Reader.h"
#include "Keyboard.h"
#else
#include "Display.h"
#endif
// ───────────────────────────────────────────────────────────────────────────

// ─── Pin / config ─────────────────────────────────────────────────────────
#ifdef USE_CARDPUTER
// ADS1115 channels 0-3 for sensors. GPIO1 for external relay.
const uint8_t sensorPins[MAX_SENSORS] = {0, 1, 2, 3, 0};   // ch 0-3, extra channel dummy
const uint8_t PUMP_RELAY_PIN = 1;
#else
// Original ESP32-S3 with direct ADC: GPIO2, GPIO8†, GPIO12-14
// † GPIO8 is inside the SPI flash bus — use only if your board supports it.
const uint8_t sensorPins[MAX_SENSORS] = {2, 8, 12, 13, 14};
const uint8_t PUMP_RELAY_PIN = 38;
#endif

// ─── Globals ──────────────────────────────────────────────────────────────
Calibration calibration;
SensorManager sensorManager;
Settings settings;
WaterPump pump;

#ifdef USE_CARDPUTER
DisplayCardputer display;
Ads1115Reader adsReader(0x48);
#else
Display display;
#endif

WebServer webServer(&sensorManager, &calibration, &display, &settings, &pump);

#ifdef USE_CARDPUTER
KeyboardHandler keyboard(&display, &calibration, &sensorManager, &settings, &pump);
#endif

bool calibrating = false;
uint8_t activeSensorForSerial = 0; // индекс датчика для serial-калибровки
static bool forceDisplayUpdate = false;

// ─── Forward declarations ─────────────────────────────────────────────────
void handleSerialCommands();

// ─── Setup ────────────────────────────────────────────────────────────────
void setup() {
#ifdef USE_CARDPUTER
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    M5Cardputer.Display.setBrightness(100);
    Serial.begin(115200);
    delay(500);
#else
    Serial.begin(115200);
    delay(1000);
#endif

    esp_task_wdt_reset();
    Serial.println("\n=== Polivator Multi-Sensor ===");
    Serial.print("Sensor pins/channels: ");
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (i > 0) Serial.print(", ");
        Serial.print(sensorPins[i]);
    }
    Serial.println();

    settings.begin();
    calibration.begin(sensorPins, MAX_SENSORS);
    esp_task_wdt_reset();

    // Wire up the optional ADC reader before SensorManager::begin()
#ifdef USE_CARDPUTER
    adsReader.begin();
    sensorManager.setReader(&adsReader);
    Serial.println("ADS1115 reader ready");
#endif
    sensorManager.begin(&calibration, &settings);
    esp_task_wdt_reset();

    pump.begin(PUMP_RELAY_PIN, true); // active HIGH: HIGH = relay on
    esp_task_wdt_reset();

    display.begin();
    display.startTask();
    esp_task_wdt_reset();

    webServer.begin();
    esp_task_wdt_reset();
    ArduinoOTA.begin();
    esp_task_wdt_reset();

    // Initial sensor read + display
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
        const char* wifiStatus = WiFi.status() == WL_CONNECTED ? "connected"
            : (WiFi.getMode() == WIFI_AP ? "ap" : "disconnected");
        display.showStatus(count, configs, percentages, rawValues, wifiStatus);
    }
    esp_task_wdt_reset();
    Serial.println("Setup complete.");
}

// ─── Loop ─────────────────────────────────────────────────────────────────
void loop() {
    esp_task_wdt_reset();
    static unsigned long lastSensorRead = 0;
    static unsigned long lastDisplayUpdate = 0;

    handleSerialCommands();
#ifdef USE_CARDPUTER
    M5Cardputer.update();  // refresh keyboard + PMU state
    keyboard.tick();
#endif
    webServer.handleClient();
    ArduinoOTA.handle();
    esp_task_wdt_reset();
    pump.tick(&sensorManager, &calibration);

    unsigned long now = millis();

    // Battery level polling (Cardputer only)
#ifdef USE_CARDPUTER
    {
        static unsigned long lastBatCheck = 0;
        if (now - lastBatCheck > 30000) {
            int bat = M5Cardputer.Power.getBatteryLevel();
            if (bat >= 0) display.setBatteryLevel((uint8_t)bat);
            lastBatCheck = now;
        }
    }
#endif

    // Sensor reading with configured interval
    unsigned long readIntervalMs = (unsigned long)settings.getReadInterval() * 1000UL;
    if (now - lastSensorRead >= readIntervalMs) {
        sensorManager.readAll();
        webServer.sendStatusEvent(&sensorManager);
        lastSensorRead = now;
        esp_task_wdt_reset();
    }

    // Detect calibration config changes
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
        if (calibration.getDryValue(i) != prevDry[i] ||
            calibration.getWetValue(i) != prevWet[i]) {
            configChanged = true;
            prevDry[i] = calibration.getDryValue(i);
            prevWet[i] = calibration.getWetValue(i);
        }
    }
    if (configChanged) {
        forceDisplayUpdate = true;
    }

    // Display update with configured interval
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
            const char* wifiStatus = WiFi.status() == WL_CONNECTED ? "connected"
                : (WiFi.getMode() == WIFI_AP ? "ap" : "disconnected");
            display.showStatus(count, configs, percentages, rawValues, wifiStatus);
        }
        lastDisplayUpdate = now;
        esp_task_wdt_reset();
    }

    vTaskDelay(pdMS_TO_TICKS(1));
}

// ─── Serial calibration commands ──────────────────────────────────────────
void handleSerialCommands() {
    if (!Serial.available()) return;
    char c = Serial.read();

    if (c == 'c' || c == 'C') {
        calibrating = !calibrating;
        if (calibrating) {
            Serial.println("Calibration mode ON. Select sensor: 0-" + String(MAX_SENSORS - 1));
            Serial.println("Send 0..4 to select sensor, then 'd' dry, 'w' wet, 'r' reset.");
            display.showCalibrationScreen("Calibration mode");
        } else {
            Serial.println("Calibration mode OFF");
        }
        return;
    }

    if (!calibrating) return;

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
