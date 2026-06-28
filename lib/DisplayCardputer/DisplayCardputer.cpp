#include "DisplayCardputer.h"
#include <M5Cardputer.h>

// ─── Colour palette ────────────────────────────────────────────────────────
static constexpr uint16_t C_BG       = 0x18E3;  // deep navy #1a1a2e
static constexpr uint16_t C_CARD     = 0x2104;  // card bg   #21213e
static constexpr uint16_t C_BAR      = 0x2A4B;  // bar fill  #2a2a5e
static constexpr uint16_t C_GREEN    = 0x07E0;  // moisture ok
static constexpr uint16_t C_YELLOW   = 0xFFE0;  // warning
static constexpr uint16_t C_RED      = 0xF800;  // critical
static constexpr uint16_t C_BLUE     = 0x041F;  // accent
static constexpr uint16_t C_CYAN     = 0x07FF;  // progress fill
static constexpr uint16_t C_WHITE    = 0xFFFF;
static constexpr uint16_t C_GRAY     = 0x8410;  // secondary text
static constexpr uint16_t C_DIM      = 0x4208;  // border/dim
static constexpr uint16_t C_SB_BG    = 0x1082;  // status bar

// ─── Constructor ────────────────────────────────────────────────────────────
DisplayCardputer::DisplayCardputer()
    : m_gfx(nullptr), m_taskHandle(nullptr), m_taskRunning(false),
      m_busy(false), m_firstUpdate(true), m_batteryPct(-1),
      m_statusPending(false), m_calibrationPending(false),
      m_messagePending(false) {}

void DisplayCardputer::begin() {
    m_gfx = &M5Cardputer.Display;
    m_gfx->setRotation(1);
    m_gfx->setColorDepth(16);
    m_gfx->fillScreen(C_BG);
    m_gfx->setTextWrap(true);

    // Show boot screen
    m_gfx->setFont(&fonts::FreeMonoBold9pt7b);
    m_gfx->setTextColor(C_WHITE);
    m_gfx->setCursor(8, 110);
    m_gfx->println("Polivator");
    m_gfx->setCursor(8, 130);
    m_gfx->setTextColor(C_GRAY);
    m_gfx->println("Starting...");

    delay(300);
}

// ─── Icons ───────────────────────────────────────────────────────────────────

void DisplayCardputer::drawIconDrop(int x, int y, uint16_t color) {
    // Simple water drop shape: triangle point + rounded bottom
    m_gfx->fillTriangle(x+8, y,   x+2, y+10,  x+14, y+10,  color);
    m_gfx->fillRect(x+2, y+9,  13, 5, color);
    m_gfx->drawFastHLine(x+4, y+14, 9, C_BG);  // notch
}

void DisplayCardputer::drawIconWiFi(int x, int y, int strength) {
    // Draw arcs for signal strength (0-3 bars)
    uint16_t c = (strength > 0) ? C_WHITE : C_DIM;
    for (int i = 0; i < 3; i++) {
        int r = 8 - i * 2;
        bool active = (strength > i);
        c = active ? C_WHITE : C_DIM;
        // Arc segments — simplified as dots and lines
        if (active) {
            int sy = y + 14 - r * 2;
            m_gfx->drawArc(x + 8, sy, r, r-1, 30, 150, c);
        }
    }
    // Dot at bottom
    m_gfx->fillCircle(x + 8, y + 14, 2, C_WHITE);
}

void DisplayCardputer::drawIconBattery(int x, int y, int pct) {
    // Battery outline
    m_gfx->drawRect(x, y, 16, 8, C_WHITE);
    m_gfx->fillRect(x + 16, y + 2, 2, 4, C_WHITE);  // terminal
    // Fill level
    int fillW = (pct * 14) / 100;
    if (fillW > 0) {
        uint16_t c = (pct > 30) ? C_GREEN : (pct > 15) ? C_YELLOW : C_RED;
        m_gfx->fillRect(x + 1, y + 1, fillW, 6, c);
    }
}

void DisplayCardputer::drawIconPump(int x, int y, uint16_t color) {
    // Simple water drop + arrow (pumping symbol)
    m_gfx->fillCircle(x + 8, y + 8, 6, color);
    m_gfx->fillTriangle(x+4, y+10,  x+8, y+2,  x+12, y+10,  C_BG);
}

void DisplayCardputer::drawIconGear(int x, int y, uint16_t color) {
    // Simple gear: circle with small teeth
    m_gfx->fillCircle(x + 8, y + 8, 5, color);
    m_gfx->drawCircle(x + 8, y + 8, 4, C_BG);
    for (int a = 0; a < 360; a += 45) {
        float rad = a * (PI / 180.0);
        int tx = x + 8 + cos(rad) * 5;
        int ty = y + 8 + sin(rad) * 5;
        m_gfx->fillCircle(tx, ty, 2, color);
    }
}

void DisplayCardputer::drawIconCheck(int x, int y, uint16_t color) {
    m_gfx->drawLine(x+2, y+8,  x+6, y+12,  color);
    m_gfx->drawLine(x+6, y+12, x+14, y+2,  color);
}

// ─── UI Components ───────────────────────────────────────────────────────────

void DisplayCardputer::drawPanel(int x, int y, int w, int h, uint16_t bg, uint16_t border) {
    if (border)
        m_gfx->fillRoundRect(x, y, w, h, 4, border);
    m_gfx->fillRoundRect(x+1, y+1, w-2, h-2, 3, bg);
}

void DisplayCardputer::drawProgressBar(int x, int y, int w, int h, int pct, uint16_t color) {
    m_gfx->fillRoundRect(x, y, w, h, h/2, C_DIM);          // track
    if (pct > 0) {
        int fillW = (w - 4) * max(0, min(100, pct)) / 100;
        if (fillW > 2)
            m_gfx->fillRoundRect(x + 2, y + 2, fillW, h - 4, (h-4)/2, color);
    }
}

// ─── Status bar ──────────────────────────────────────────────────────────────

void DisplayCardputer::drawStatusBar(const char* wifiStatus) {
    // Background strip
    m_gfx->fillRect(0, 0, 135, 16, C_SB_BG);

    // WiFi icon + status
    drawIconWiFi(2, 0, wifiStatus && strcmp(wifiStatus, "off") != 0 ? 3 : 0);

    // Battery
    if (m_batteryPct >= 0) {
        drawIconBattery(100, 4, m_batteryPct);
    }

    // Separator line
    m_gfx->drawFastHLine(0, 16, 135, C_DIM);
}

// ─── Hero card (driest sensor) ──────────────────────────────────────────────

void DisplayCardputer::drawHeroCard(int& y, int pct, const char* name, uint8_t sensorIdx) {
    int w = 131;
    int h = 60;
    int x = 2;

    // Color based on moisture
    uint16_t accent = (pct > 60) ? C_GREEN : (pct > 25) ? C_YELLOW : C_RED;

    // Card background
    drawPanel(x, y, w, h, C_CARD, accent);

    // Icon
    drawIconDrop(x + 8, y + 14, accent);

    // Large percentage
    m_gfx->setFont(&fonts::FreeMono12pt7b);
    m_gfx->setTextColor(C_WHITE);
    m_gfx->setCursor(x + 35, y + 26);
    m_gfx->printf("%d%%", pct);

    // Sensor label
    m_gfx->setFont(&fonts::FreeMonoBold9pt7b);
    m_gfx->setTextColor(C_GRAY);
    m_gfx->setCursor(x + 35, y + 42);
    m_gfx->print(name);

    // Progress bar
    drawProgressBar(x + 8, y + 48, w - 16, 8, pct, accent);

    y += h + 4;
}

// ─── Sensor row (compact) ───────────────────────────────────────────────────

void DisplayCardputer::drawSensorRow(int y, int pct, const char* name, uint8_t sensorIdx, bool highlight) {
    // Only draw if we have room
    if (y > 230) return;

    int x = 2;
    int w = 131;
    int h = 20;

    uint16_t accent = (pct > 60) ? C_GREEN : (pct > 25) ? C_YELLOW : C_RED;
    uint16_t bg = highlight ? C_CARD : C_BG;

    // Background line
    m_gfx->fillRect(x, y, w, h, bg);

    // Mini indicator bar on the left
    m_gfx->fillRect(x, y + 2, 2, h - 4, accent);

    // Percentage
    m_gfx->setFont(&fonts::FreeMonoBold9pt7b);
    m_gfx->setTextColor(C_WHITE);
    m_gfx->setCursor(x + 8, y + 13);
    m_gfx->printf("%d%%", pct);

    // Name
    m_gfx->setTextColor(C_GRAY);

    // Compact progress bar on the right
    int barX = 60;
    int barW = w - barX - 4;
    drawProgressBar(barX, y + 4, barW, 12, pct, accent);
}

// ─── Footer ──────────────────────────────────────────────────────────────────

void DisplayCardputer::drawFooter() {
    m_gfx->fillRect(0, 228, 135, 12, C_SB_BG);
    m_gfx->drawFastHLine(0, 228, 135, C_DIM);
    m_gfx->setFont(&fonts::Font0);
    m_gfx->setTextColor(C_GRAY);
    m_gfx->setCursor(2, 237);
    m_gfx->print("S:Menu  C:Calib  +|-");
}

// ─── Dashboard ───────────────────────────────────────────────────────────────

void DisplayCardputer::renderDashboard(uint8_t sensorCount,
                                        const SensorConfig* configs,
                                        const int* percentages,
                                        const int* rawValues,
                                        const char* wifiStatus) {
    m_busy = true;

    m_gfx->fillScreen(C_BG);

    // Status bar
    drawStatusBar(wifiStatus);

    // Find driest active sensor for hero card
    int heroIdx = -1;
    int heroPct = 999;
    for (int i = 0; i < sensorCount; i++) {
        if (configs[i].enabled && percentages[i] < heroPct) {
            heroPct = percentages[i];
            heroIdx = i;
        }
    }
    if (heroIdx < 0) heroIdx = 0;

    int y = 20;

    // Hero card for driest sensor
    if (heroIdx >= 0 && heroIdx < sensorCount) {
        drawHeroCard(y, percentages[heroIdx], configs[heroIdx].name, heroIdx);
    }

    // Compact rows for remaining sensors
    for (int i = 0; i < sensorCount; i++) {
        if (i == heroIdx || !configs[i].enabled) continue;
        drawSensorRow(y, percentages[i], configs[i].name, i, false);
        y += 22;
    }

    // Footer
    drawFooter();

    m_firstUpdate = false;
    m_busy = false;
}

// ─── Calibration screen ──────────────────────────────────────────────────────

void DisplayCardputer::renderCalibration(const char* step) {
    m_busy = true;
    m_gfx->fillScreen(C_BG);
    m_gfx->setTextColor(C_WHITE);

    // Header bar
    m_gfx->fillRect(0, 0, 135, 20, C_BLUE);
    m_gfx->setFont(&fonts::FreeMonoBold9pt7b);
    m_gfx->setCursor(4, 14);
    m_gfx->print("Settings / Calib");
    m_gfx->drawFastHLine(0, 20, 135, C_DIM);

    // Content
    m_gfx->setFont(&fonts::FreeMonoBold9pt7b);
    m_gfx->setCursor(4, 38);
    if (step && step[0]) {
        // Multi-line: split by \n and print
        const char* p = step;
        int y = 38;
        while (*p && y < 224) {
            const char* nl = strchr(p, '\n');
            int len = (nl) ? (nl - p) : strlen(p);
            char line[64];
            int cp = min(len, 63);
            strncpy(line, p, cp);
            line[cp] = '\0';
            m_gfx->setCursor(4, y);
            m_gfx->print(line);
            y += 14;
            p = (nl) ? nl + 1 : p + len;
        }
    } else {
        m_gfx->print("Ready");
    }

    m_firstUpdate = false;
    m_busy = false;
}

// ─── Message screen ──────────────────────────────────────────────────────────

void DisplayCardputer::renderMessage(const char* msg) {
    m_busy = true;
    m_gfx->fillScreen(C_BG);
    m_gfx->setTextColor(C_WHITE);

    // Header bar
    m_gfx->fillRect(0, 0, 135, 20, C_SB_BG);
    m_gfx->setFont(&fonts::FreeMonoBold9pt7b);
    m_gfx->setCursor(4, 14);
    m_gfx->print("Polivator");
    m_gfx->drawFastHLine(0, 20, 135, C_DIM);

    // Content
    m_gfx->setFont(&fonts::FreeMonoBold9pt7b);
    m_gfx->setCursor(4, 55);
    if (msg && msg[0]) {
        const char* p = msg;
        int y = 55;
        while (*p && y < 224) {
            const char* nl = strchr(p, '\n');
            int len = (nl) ? (nl - p) : strlen(p);
            char line[64];
            int cp = min(len, 63);
            strncpy(line, p, cp);
            line[cp] = '\0';
            m_gfx->setCursor(4, y);
            m_gfx->print(line);
            y += 14;
            p = (nl) ? nl + 1 : p + len;
        }
    } else {
        m_gfx->print("Ready");
    }

    m_firstUpdate = false;
    m_busy = false;
}

// ─── IDisplay interface ──────────────────────────────────────────────────────

void DisplayCardputer::showStatus(uint8_t sensorCount,
                                   const SensorConfig* configs,
                                   const int* percentages,
                                   const int* rawValues,
                                   const char* wifiStatus) {
    if (m_taskRunning) {
        // Queue data for the display task
        m_sensorCount = min(sensorCount, (uint8_t)MAX_SENSORS);
        memcpy(m_configs, configs, sizeof(SensorConfig) * MAX_SENSORS);
        memcpy(m_percentages, percentages, sizeof(int) * MAX_SENSORS);
        memcpy(m_rawValues, rawValues, sizeof(int) * MAX_SENSORS);
        strncpy(m_wifiStatus, wifiStatus ? wifiStatus : "", 31);
        m_wifiStatus[31] = '\0';
        m_statusPending = true;
    } else {
        renderDashboard(sensorCount, configs, percentages, rawValues, wifiStatus);
    }
}

void DisplayCardputer::showCalibrationScreen(const char* step) {
    if (m_taskRunning) {
        strncpy(m_calibStep, step ? step : "", 127);
        m_calibStep[127] = '\0';
        m_calibrationPending = true;
    } else {
        renderCalibration(step);
    }
}

void DisplayCardputer::showMessage(const char* msg) {
    if (m_taskRunning) {
        strncpy(m_messageText, msg ? msg : "", 127);
        m_messageText[127] = '\0';
        m_messagePending = true;
    } else {
        renderMessage(msg);
    }
}

void DisplayCardputer::deepSleep() {
    m_gfx->fillScreen(0);
    m_gfx->setBrightness(0);
}

void DisplayCardputer::forceFullRefresh() {
    m_firstUpdate = true;
}

bool DisplayCardputer::isBusy() const { return m_busy; }
bool DisplayCardputer::isTaskRunning() const { return m_taskRunning; }

// ─── Task management ─────────────────────────────────────────────────────────

void DisplayCardputer::displayTaskFunc(void* param) {
    auto* self = static_cast<DisplayCardputer*>(param);
    while (true) {
        if (self->m_statusPending) {
            self->m_statusPending = false;
            self->renderDashboard(self->m_sensorCount,
                                   self->m_configs,
                                   self->m_percentages,
                                   self->m_rawValues,
                                   self->m_wifiStatus);
        } else if (self->m_calibrationPending) {
            self->m_calibrationPending = false;
            self->renderCalibration(self->m_calibStep);
        } else if (self->m_messagePending) {
            self->m_messagePending = false;
            self->renderMessage(self->m_messageText);
        }
        delay(30);  // yield to other tasks
    }
}

void DisplayCardputer::startTask() {
    if (m_taskRunning) return;
    m_taskRunning = true;
    xTaskCreatePinnedToCore(
        displayTaskFunc, "DisplayTask", 4096,
        this, 1, &m_taskHandle, 0
    );
}

void DisplayCardputer::stopTask() {
    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
    }
    m_taskRunning = false;
}

void DisplayCardputer::setBatteryLevel(int pct) {
    m_batteryPct = pct;
}
