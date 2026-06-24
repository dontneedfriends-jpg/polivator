#include "Display.h"
#include <SPI.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <string.h>
#include <cstdlib>

// Droplet icon 16x16 monochrome bitmap
const uint8_t droplet_16x16[] PROGMEM = {
  0b00000010, 0b00000000,
  0b00000110, 0b00000000,
  0b00001110, 0b00000000,
  0b00011111, 0b00000000,
  0b00111111, 0b10000000,
  0b01111111, 0b11000000,
  0b01111111, 0b11000000,
  0b11111111, 0b11100000,
  0b11111111, 0b11100000,
  0b01111111, 0b11000000,
  0b01111111, 0b11000000,
  0b00111111, 0b10000000,
  0b00011111, 0b00000000,
  0b00001110, 0b00000000,
  0b00000100, 0b00000000,
  0b00000000, 0b00000000
};

Display::Display()
  : m_firstUpdate(true),
    m_initialFullRefreshDone(false),
    m_fullRefreshCounter(0) {
  memset(m_prevPercentages, -1, sizeof(m_prevPercentages));
  memset(m_prevRaw, -1, sizeof(m_prevRaw));
  m_prevWifi[0] = '\0';
}

Display::~Display() {
}

void Display::begin() {
  // ESP32-S3: SPI.begin(SCK, MISO, MOSI, SS)
  // MISO не используется дисплеем, но передадим -1 в виде uint8_t для совместимости.
  SPI.begin(EINK_SCLK, -1, EINK_MOSI, EINK_CS);
  m_display.init(115200, true);
  m_display.setRotation(1);
  // Initial full refresh to clear ghosting and verify display is alive
  m_display.setFullWindow();
  m_display.firstPage();
  do {
    m_display.fillScreen(GxEPD_WHITE);
  } while (m_display.nextPage());
  m_firstUpdate = true;
  m_initialFullRefreshDone = true;
  // Keep power on briefly; then power off only if no updates scheduled soon
  // For stability, do NOT powerOff here; let first showStatus handle it.
}

bool Display::hasChanged(uint8_t sensorCount, const int* percentages, const int* rawValues, const char* wifiStatus) {
  if (m_firstUpdate) return true;
  if (sensorCount > 5) sensorCount = 5;
  for (uint8_t i = 0; i < sensorCount; i++) {
    if (abs(percentages[i] - m_prevPercentages[i]) >= 1 || abs(rawValues[i] - m_prevRaw[i]) >= 5) {
      return true;
    }
  }
  if (strcmp(wifiStatus, m_prevWifi) != 0) return true;
  return false;
}

void Display::drawStatus(uint8_t sensorCount, const SensorConfig* configs, const int* percentages, const int* rawValues, const char* wifiStatus) {
  // Top bar: WiFi status (small font)
  m_display.setTextColor(GxEPD_BLACK);
  m_display.setFont(&FreeMonoBold9pt7b);
  m_display.setCursor(5, 14);
  m_display.print("WiFi: ");
  m_display.print(wifiStatus);

  // Center: droplet icon and large percentage (first sensor)
  if (sensorCount > 0) {
    // Draw droplet icon
    int16_t iconX = 25;
    int16_t iconY = 48;
    m_display.drawBitmap(iconX, iconY, droplet_16x16, 16, 16, GxEPD_BLACK);

    // Percentage text
    char pctStr[8];
    if (configs[0].dryRaw != configs[0].wetRaw) {
      snprintf(pctStr, sizeof(pctStr), "%d%%", percentages[0]);
    } else {
      snprintf(pctStr, sizeof(pctStr), "?");
    }
    m_display.setFont(&FreeMonoBold18pt7b);
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(pctStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    int16_t textX = iconX + 16 + 10;
    int16_t textY = iconY + 14;
    m_display.setCursor(textX, textY);
    m_display.print(pctStr);

    // Bottom bar: sensor name and raw value
    m_display.setFont(&FreeMonoBold9pt7b);
    char bottomStr[32];
    snprintf(bottomStr, sizeof(bottomStr), "%s  %d", configs[0].name, rawValues[0]);
    m_display.setCursor(5, m_display.height() - 6);
    m_display.print(bottomStr);
  }
}

void Display::showStatus(uint8_t sensorCount, const SensorConfig* configs, const int* percentages, const int* rawValues, const char* wifiStatus) {
  if (!hasChanged(sensorCount, percentages, rawValues, wifiStatus)) {
    return;
  }

  // Do a full refresh on first update, then mostly partial updates for speed.
  // Every 10th update force full refresh to clear ghosting.
  bool fullRefresh = m_firstUpdate || (m_fullRefreshCounter % 10 == 0);

  if (fullRefresh) {
    m_display.setFullWindow();
  } else {
    m_display.setPartialWindow(0, 0, m_display.width(), m_display.height());
  }
  m_display.firstPage();
  do {
    m_display.fillScreen(GxEPD_WHITE);
    drawStatus(sensorCount, configs, percentages, rawValues, wifiStatus);
  } while (m_display.nextPage());

  // Keep display powered during partial updates for faster subsequent refreshes.
  // Only power off after full refresh to reduce power consumption.
  if (fullRefresh) {
    m_display.powerOff();
  }

  // Update previous values
  if (sensorCount > 5) sensorCount = 5;
  for (uint8_t i = 0; i < sensorCount && i < 5; i++) {
    m_prevPercentages[i] = percentages[i];
    m_prevRaw[i] = rawValues[i];
  }
  strncpy(m_prevWifi, wifiStatus, sizeof(m_prevWifi) - 1);
  m_prevWifi[sizeof(m_prevWifi) - 1] = '\0';

  m_firstUpdate = false;
  m_fullRefreshCounter++;
}

void Display::showCalibrationScreen(const char* step) {
  m_display.setFullWindow();
  m_display.firstPage();
  do {
    m_display.fillScreen(GxEPD_WHITE);
    m_display.setTextColor(GxEPD_BLACK);
    m_display.setFont(&FreeMonoBold9pt7b);

    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds("Calibrating...", 0, 0, &tbx, &tby, &tbw, &tbh);
    m_display.setCursor((m_display.width() - tbw) / 2, tbh + 10);
    m_display.println("Calibrating...");

    if (step && step[0]) {
      m_display.getTextBounds(step, 0, 0, &tbx, &tby, &tbw, &tbh);
      m_display.setCursor((m_display.width() - tbw) / 2, m_display.height() / 2);
      m_display.println(step);
    }
  } while (m_display.nextPage());
  m_display.powerOff();
  m_firstUpdate = true;
}

void Display::showMessage(const char* msg) {
  m_display.setFullWindow();
  m_display.firstPage();
  do {
    m_display.fillScreen(GxEPD_WHITE);
    m_display.setTextColor(GxEPD_BLACK);
    m_display.setFont(&FreeMonoBold9pt7b);

    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(msg, 0, 0, &tbx, &tby, &tbw, &tbh);
    m_display.setCursor((m_display.width() - tbw) / 2, (m_display.height() + tbh) / 2);
    m_display.println(msg);
  } while (m_display.nextPage());
  m_display.powerOff();
  m_firstUpdate = true;
}

void Display::deepSleep() {
  m_display.powerOff();
}

void Display::forceFullRefresh() {
  m_firstUpdate = true;
}
