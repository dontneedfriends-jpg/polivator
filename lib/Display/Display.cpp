#include "Display.h"
#include <SPI.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <string.h>
#include <cstdlib>
#include <esp_task_wdt.h>

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
    m_fullRefreshCounter(0),
    m_taskRunning(false),
    m_updatePending(false),
    m_calibrationPending(false),
    m_messagePending(false),
    m_snapCount(0),
    m_busy(false) {
  memset(m_prevPercentages, -1, sizeof(m_prevPercentages));
  memset(m_prevRaw, -1, sizeof(m_prevRaw));
  m_prevWifi[0] = '\0';
  m_calibStep[0] = '\0';
  m_messageText[0] = '\0';
  m_snapWifi[0] = '\0';
}

Display::~Display() {
}

void Display::begin() {
  // ESP32-S3: SPI.begin(SCK, MISO, MOSI, SS)
  // MISO не используется дисплеем, но передадим -1 в виде uint8_t для совместимости.
  SPI.begin(EINK_SCLK, -1, EINK_MOSI, EINK_CS);
  m_display.init(115200, true);
  m_display.setRotation(1);
  m_firstUpdate = true;
  m_initialFullRefreshDone = false;
  // Initial clear will be performed by the display task on the first update — keep setup() fast.
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
  int w = m_display.width();
  int h = m_display.height();

  // Top bar: WiFi status (small font)
  m_display.setTextColor(GxEPD_BLACK);
  m_display.setFont(&FreeMonoBold9pt7b);
  m_display.setCursor(4, 12);
  m_display.print("WiFi:");
  m_display.print(wifiStatus);

  // Divider under top bar
  m_display.drawLine(0, 16, w, 16, GxEPD_BLACK);

  if (sensorCount == 0) {
    m_display.setCursor(4, h - 4);
    m_display.print("No sensors");
    return;
  }

  // Vertical column layout: only enabled sensors
  uint8_t visible[5];
  uint8_t vCount = 0;
  for (uint8_t i = 0; i < 5 && i < sensorCount; i++) {
    if (configs[i].enabled) visible[vCount++] = i;
  }
  if (vCount == 0) {
    m_display.setCursor(4, h / 2);
    m_display.print("No sensors");
    return;
  }

  int topY = 20;
  int bottomY = h - 12;
  int colH = bottomY - topY;
  int colW = 14;
  int gap = (w - vCount * colW) / (vCount + 1);
  if (gap < 4) gap = 4;

  m_display.setFont(&FreeMonoBold9pt7b);

  for (uint8_t k = 0; k < vCount; k++) {
    uint8_t i = visible[k];
    int colX = gap + k * (colW + gap);
    int colY = topY;

    m_display.drawRect(colX, colY, colW, colH, GxEPD_BLACK);

    int pct = percentages[i];
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    int fillH = (colH - 2) * pct / 100;
    if (fillH > 0) {
      m_display.fillRect(colX + 1, colY + colH - 1 - fillH, colW - 2, fillH, GxEPD_BLACK);
    }

    char label[4];
    snprintf(label, sizeof(label), "%d", i + 1);
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(label, 0, 0, &tbx, &tby, &tbw, &tbh);
    m_display.setCursor(colX + (colW - tbw) / 2, h - 2);
    m_display.print(label);

    char pctStr[8];
    if (configs[i].dryRaw != configs[i].wetRaw) {
      snprintf(pctStr, sizeof(pctStr), "%d%%", pct);
    } else {
      snprintf(pctStr, sizeof(pctStr), "-");
    }
    m_display.getTextBounds(pctStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    int textX = colX + colW + 2;
    int textY = colY + (colH + tbh) / 2;
    if (textX + tbw <= w) {
      m_display.setCursor(textX, textY);
      m_display.print(pctStr);
    }
  }
}

void Display::showStatus(uint8_t sensorCount, const SensorConfig* configs, const int* percentages, const int* rawValues, const char* wifiStatus) {
  if (!hasChanged(sensorCount, percentages, rawValues, wifiStatus)) {
    return;
  }

  if (!m_taskRunning) {
    renderStatusImpl:
    renderStatus();
    return;
  }

  // Snapshot for the display task (avoid touching caller's buffers across cores)
  uint8_t cap = sensorCount > 5 ? 5 : sensorCount;
  m_snapCount = cap;
  for (uint8_t i = 0; i < cap; i++) {
    m_snapConfigs[i] = configs[i];
    m_snapPercentages[i] = percentages[i];
    m_snapRaw[i] = rawValues[i];
  }
  strncpy(m_snapWifi, wifiStatus, sizeof(m_snapWifi) - 1);
  m_snapWifi[sizeof(m_snapWifi) - 1] = '\0';
  m_updatePending = true;
}

void Display::renderStatus() {
  uint8_t sensorCount = m_snapCount;
  if (sensorCount > 5) sensorCount = 5;

  // Always use partial window — fast and flicker-free.
  // Every Nth update force full refresh to clear ghosting.
  bool fullRefresh = m_firstUpdate || (m_fullRefreshCounter % 10 == 0);

  if (fullRefresh) {
    m_display.setFullWindow();
  } else {
    m_display.setPartialWindow(0, 0, m_display.width(), m_display.height());
  }
  m_display.firstPage();
  do {
    m_display.fillScreen(GxEPD_WHITE);
    drawStatus(sensorCount, m_snapConfigs, m_snapPercentages, m_snapRaw, m_snapWifi);
  } while (m_display.nextPage());

  if (fullRefresh) {
    m_display.powerOff();
  }

  for (uint8_t i = 0; i < sensorCount; i++) {
    m_prevPercentages[i] = m_snapPercentages[i];
    m_prevRaw[i] = m_snapRaw[i];
  }
  strncpy(m_prevWifi, m_snapWifi, sizeof(m_prevWifi) - 1);
  m_prevWifi[sizeof(m_prevWifi) - 1] = '\0';

  m_firstUpdate = false;
  m_fullRefreshCounter++;
}

void Display::showCalibrationScreen(const char* step) {
  if (!m_taskRunning) { renderCalibration(step); return; }
  strncpy(m_calibStep, step ? step : "", sizeof(m_calibStep) - 1);
  m_calibStep[sizeof(m_calibStep) - 1] = '\0';
  m_calibrationPending = true;
}

void Display::renderCalibration(const char* step) {
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
  if (!m_taskRunning) { renderMessage(msg); return; }
  strncpy(m_messageText, msg ? msg : "", sizeof(m_messageText) - 1);
  m_messageText[sizeof(m_messageText) - 1] = '\0';
  m_messagePending = true;
}

void Display::renderMessage(const char* msg) {
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

bool Display::isBusy() const {
  return m_busy;
}

void Display::startTask() {
  if (m_taskRunning) return;
  m_taskRunning = true;
  // Pin the display refresh to core 0 so loop() on core 1 stays responsive.
  // 8 KB stack is needed for GxEPD2 page buffers + FreeMono fonts.
  xTaskCreatePinnedToCore(&Display::taskThunk, "display", 8192, this, 1, nullptr, 0);
}

void Display::stopTask() {
  m_taskRunning = false;
}

void Display::taskThunk(void* arg) {
  static_cast<Display*>(arg)->taskLoop();
}

void Display::taskLoop() {
  while (m_taskRunning) {
    if (m_messagePending) {
      m_messagePending = false;
      m_busy = true;
      renderMessage(m_messageText);
      m_busy = false;
      continue;
    }
    if (m_calibrationPending) {
      m_calibrationPending = false;
      m_busy = true;
      renderCalibration(m_calibStep);
      m_busy = false;
      continue;
    }
    if (m_updatePending) {
      m_updatePending = false;
      m_busy = true;
      renderStatus();
      m_busy = false;
      continue;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
  vTaskDelete(nullptr);
}
