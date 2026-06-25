#include "Display.h"
#include <SPI.h>
#include <Fonts/TomThumb.h>
#include <Fonts/Picopixel.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
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

// --- Pixel-art icons 8x8 (Flipper-style monochrome) ---
// Drop icon: 8x8, drawn in 8 bytes
static const uint8_t icon_drop_8x8[] PROGMEM = {
  0b00011000,
  0b00111100,
  0b01111110,
  0b11111111,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000,
};
// WiFi icon (3 bars) 8x8
static const uint8_t icon_wifi_8x8[] PROGMEM = {
  0b00000000,
  0b00011000,
  0b00111100,
  0b01011010,
  0b00011000,
  0b00011000,
  0b00000000,
  0b00000000,
};
// WiFi off (X) 8x8
static const uint8_t icon_wifi_off_8x8[] PROGMEM = {
  0b00000000,
  0b00011000,
  0b00111100,
  0b01011010,
  0b00011000,
  0b01100110,
  0b00000000,
  0b00000000,
};
// Plant/pot icon 8x8
static const uint8_t icon_plant_8x8[] PROGMEM = {
  0b00011000,
  0b00111100,
  0b01100110,
  0b01100110,
  0b00111100,
  0b01100110,
  0b11111111,
  0b01100110,
};
// Gear (settings) 8x8
static const uint8_t icon_gear_8x8[] PROGMEM = {
  0b00111100,
  0b01011010,
  0b11100111,
  0b11011011,
  0b11011011,
  0b11100111,
  0b01011010,
  0b00111100,
};

static void drawIcon(GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT>& disp,
                     int16_t x, int16_t y, const uint8_t* icon, uint16_t color) {
  disp.drawBitmap(x, y, icon, 8, 8, color);
}

void Display::drawStatus(uint8_t sensorCount, const SensorConfig* configs, const int* percentages, const int* rawValues, const char* wifiStatus) {
  int w = m_display.width();
  int h = m_display.height();

  // ===== STATUS BAR (top, 16 px) =====
  const int SB_H = 16;
  m_display.fillRect(0, 0, w, SB_H, GxEPD_BLACK);
  m_display.setTextColor(GxEPD_WHITE);
  m_display.setFont(&FreeSans9pt7b);

  m_display.setCursor(4, 12);
  m_display.print("POLIVATOR");

  bool wifiOk = (strcmp(wifiStatus, "connected") == 0);
  bool wifiAp = (strcmp(wifiStatus, "ap") == 0);
  const uint8_t* wifiIcon = (wifiOk || wifiAp) ? icon_wifi_8x8 : icon_wifi_off_8x8;
  drawIcon(m_display, w - 50, 4, wifiIcon, GxEPD_WHITE);
  m_display.setCursor(w - 38, 12);
  m_display.print(wifiOk ? "STA" : (wifiAp ? "AP" : "OFF"));

  // ===== BODY =====
  m_display.setTextColor(GxEPD_BLACK);
  m_display.fillRect(0, SB_H, w, h - SB_H, GxEPD_WHITE);

  uint8_t visible[5];
  uint8_t vCount = 0;
  uint8_t focusIdx = 0;
  int focusPct = 200;
  for (uint8_t i = 0; i < 5 && i < sensorCount; i++) {
    if (configs[i].enabled) {
      if (percentages[i] < focusPct) {
        focusPct = percentages[i];
        focusIdx = vCount;
      }
      visible[vCount++] = i;
    }
  }

  if (vCount == 0) {
    m_display.setFont(&FreeSans9pt7b);
    m_display.setCursor(8, h / 2);
    m_display.print("-- no sensors --");
    return;
  }

  // ===== HERO PANEL =====
  int heroH = 60;
  if (h < 96) heroH = h - 36;
  if (heroH < 50) heroH = 50;

  m_display.drawRect(0, SB_H, w, heroH, GxEPD_BLACK);
  m_display.drawRect(2, SB_H + 2, w - 4, heroH - 4, GxEPD_BLACK);

  {
    uint8_t i = visible[focusIdx];
    int pct = percentages[i];
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    // Big percent — FreeMonoBold18pt (right side, vertical center)
    char pctStr[8];
    if (configs[i].dryRaw != configs[i].wetRaw) {
      snprintf(pctStr, sizeof(pctStr), "%d%%", pct);
    } else {
      snprintf(pctStr, sizeof(pctStr), "?");
    }
    m_display.setFont(&FreeMonoBold18pt7b);
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(pctStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    m_display.setCursor(w - tbw - 6, SB_H + (heroH + tbh) / 2);
    m_display.print(pctStr);

    // Drop icon (2x scale = 16x16) at top-left of left half
    for (int dy = 0; dy < 8; dy++) {
      for (int dx = 0; dx < 8; dx++) {
        if (pgm_read_byte(&icon_drop_8x8[dy]) & (0x80 >> dx)) {
          m_display.fillRect(8 + dx * 2, SB_H + 4 + dy * 2, 2, 2, GxEPD_BLACK);
        }
      }
    }

    // Sensor name (FreeSans9pt, below drop icon, no prefix dot)
    m_display.setFont(&FreeSans9pt7b);
    m_display.setCursor(8, SB_H + 28);
    m_display.print("S");
    m_display.print((int)(i + 1));
    m_display.print(" ");
    m_display.print(configs[i].name);
  }

  // Mini progress bar inside hero (4 px tall)
  {
    uint8_t i = visible[focusIdx];
    int pct = percentages[i];
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    int barX = 6;
    int barY = SB_H + heroH - 10;
    int barW = w - 12;
    m_display.drawRect(barX, barY, barW, 4, GxEPD_BLACK);
    int fillW = (barW - 2) * pct / 100;
    if (fillW > 0) {
      m_display.fillRect(barX + 1, barY + 1, fillW, 2, GxEPD_BLACK);
    }
  }

  // ===== LIST OF OTHER SENSORS =====
  int listTop = SB_H + heroH + 2;
  int listH = h - listTop - 2;
  if (listH < 12) return;
  int otherCount = vCount - 1;
  int rowH = otherCount > 0 ? listH / otherCount : 0;
  if (rowH > 16) rowH = 16;
  if (rowH < 12) rowH = 12;

  int drawn = 0;
  for (uint8_t k = 0; k < vCount; k++) {
    if (k == focusIdx) continue;
    uint8_t i = visible[k];
    int rowY = listTop + drawn * rowH;
    if (rowY + rowH > h) break;
    drawn++;

    // Plant icon (8x8)
    drawIcon(m_display, 2, rowY + 2, icon_plant_8x8, GxEPD_BLACK);

    // Sensor id (S1..S5) — FreeSans9pt
    m_display.setFont(&FreeSans9pt7b);
    char label[4];
    snprintf(label, sizeof(label), "S%d", i + 1);
    m_display.setCursor(13, rowY + 9);
    m_display.print(label);

    // Percent value (right aligned) — FreeMonoBold12pt
    char pctStr[8];
    if (configs[i].dryRaw != configs[i].wetRaw) {
      snprintf(pctStr, sizeof(pctStr), "%d%%", percentages[i]);
    } else {
      snprintf(pctStr, sizeof(pctStr), "-");
    }
    m_display.setFont(&FreeMonoBold12pt7b);
    int16_t tbx, tby; uint16_t tbw, tbh;
    m_display.getTextBounds(pctStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    m_display.setCursor(w - tbw - 2, rowY + 9);
    m_display.print(pctStr);

    // Mini bar between id and percent
    int barX = 28;
    int barY = rowY + 4;
    int barW = w - barX - tbw - 6;
    int barH = 4;
    if (barW > 4) {
      m_display.drawRect(barX, barY, barW, barH, GxEPD_BLACK);
      int fill = (barW - 2) * percentages[i] / 100;
      if (fill < 0) fill = 0;
      if (fill > 100) fill = 100;
      fill = (barW - 2) * fill / 100;
      if (fill > 0) m_display.fillRect(barX + 1, barY + 1, fill, barH - 2, GxEPD_BLACK);
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
