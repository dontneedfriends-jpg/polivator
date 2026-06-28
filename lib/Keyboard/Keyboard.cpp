#include "Keyboard.h"
#include <M5Cardputer.h>
#include <Preferences.h>
#include <string.h>

// For non-numeric HID codes that we handle as actions
static constexpr uint8_t HID_A       = 0x04;
static constexpr uint8_t HID_B       = 0x05;
static constexpr uint8_t HID_C       = 0x06;
static constexpr uint8_t HID_D       = 0x07;
static constexpr uint8_t HID_M       = 0x10;
static constexpr uint8_t HID_P       = 0x14;
static constexpr uint8_t HID_R       = 0x15;
static constexpr uint8_t HID_S       = 0x16;
static constexpr uint8_t HID_W       = 0x1A;
static constexpr uint8_t HID_ENTER   = 0x28;
static constexpr uint8_t HID_ESC     = 0x29;
static constexpr uint8_t HID_BACKSP  = 0x2A;
static constexpr uint8_t HID_TAB     = 0x2B;
static constexpr uint8_t HID_SPACE   = 0x2C;
static constexpr uint8_t HID_MINUS   = 0x2D;
static constexpr uint8_t HID_EQUAL   = 0x2E;
static constexpr uint8_t HID_DOT     = 0x36;
static constexpr uint8_t HID_SLASH   = 0x38;

// Helper: get printable character from HID code (US layout, no shift).
static char hidToAscii(uint8_t code) {
    if (code >= HID_A && code <= HID_Z) return 'a' + (code - HID_A);
    if (code >= HID_1 && code <= HID_9) return '1' + (code - HID_1);
    if (code == HID_0)    return '0';
    if (code == HID_DOT)  return '.';
    if (code == HID_MINUS) return '-';
    if (code == HID_EQUAL) return '=';
    if (code == HID_SLASH) return '/';
    if (code == HID_SPACE) return ' ';
    return '\0';
}

// Helper: check if HID code is a letter (a-z or A-Z).
static bool isAlpha(uint8_t code) {
    return code >= HID_A && code <= HID_Z;  // 0x04-0x1D
}

// ─── Constructor ──────────────────────────────────────────────────────────

KeyboardHandler::KeyboardHandler(IDisplay* display,
                                 Calibration* calibration,
                                 SensorManager* sensorManager,
                                 Settings* settings,
                                 WaterPump* pump)
    : m_display(display),
      m_cal(calibration),
      m_sensorManager(sensorManager),
      m_settings(settings),
      m_pump(pump),
      m_mode(MODE_NORMAL),
      m_selSensor(0),
      m_menuItem(0),
      m_sensorMenuItem(0),
      m_textLen(0),
      m_textMax(0),
      m_wifiCount(0) {
    m_textBuf[0] = '\0';
}

// ─── Public tick ──────────────────────────────────────────────────────────

void KeyboardHandler::tick() {
    if (!M5Cardputer.Keyboard.isChange()) return;
    static bool wasPressed = false;
    bool nowPressed = M5Cardputer.Keyboard.isPressed();
    if (nowPressed && !wasPressed) {
        auto key = M5Cardputer.Keyboard.key();
        uint8_t code = key.keys;

        switch (m_mode) {
            case MODE_NORMAL:         handleNormalKey(code);      break;
            case MODE_CALIB:          handleCalibKey(code);       break;
            case MODE_SETTINGS:       handleSettingsKey(code);    break;
            case MODE_WIFI_NETLIST:   handleWifiNetlistKey(code); break;
            case MODE_WIFI_SSID:
            case MODE_WIFI_PASS:
            case MODE_SENSOR_RENAME:  handleTextInputKey(code);   break;
            case MODE_SENSOR_MENU:    handleSensorMenuKey(code);  break;
        }
    }
    wasPressed = nowPressed;
}

// ─── Mode: NORMAL ─────────────────────────────────────────────────────────

void KeyboardHandler::handleNormalKey(uint8_t code) {
    switch (code) {
        case HID_C:   toggleCalibration();   break;
        case HID_P:   togglePump();           break;
        case HID_A:   toggleAutoMode();       break;
        case HID_EQUAL: adjustThreshold(5);   break;
        case HID_MINUS: adjustThreshold(-5);  break;
        case HID_S:   enterSettings();        break;
        default: break;
    }
}

// ─── Enter / exit from settings menu ──────────────────────────────────────

void KeyboardHandler::enterSettings() {
    m_mode = MODE_SETTINGS;
    m_menuItem = 0;
    renderSettingsMenu();
    Serial.println("Keyboard: settings menu");
}

void KeyboardHandler::exitToNormal(const char* msg) {
    m_mode = MODE_NORMAL;
    if (msg) {
        m_display->showMessage(msg);
    }
}

// ─── Mode: SETTINGS ───────────────────────────────────────────────────────

void KeyboardHandler::handleSettingsKey(uint8_t code) {
    // Tab cycles through menu items
    if (code == HID_TAB) {
        m_menuItem = (m_menuItem + 1) % 9;  // 0-8 = 9 items
        renderSettingsMenu();
        return;
    }
    if (code == HID_ENTER) {
        switch (m_menuItem) {
            case 0: doWifiScan();           break;   // WiFi Setup
            case 1: enterSensorMenu();       break;   // Sensor Config
            case 2: adjustReadInterval(1);   break;   // Read Interval
            case 3: adjustDisplayInterval(1);break;   // Display Interval
            case 4: cycleDisplayUnit();      break;   // Display Unit
            case 5: adjustAdcSamples(1);     break;   // ADC Samples
            case 6: cycleSleepMode();        break;   // Sleep Mode
            case 7: showInfo();              break;   // System Info
            case 8: exitToNormal("Settings closed"); break;
        }
        return;
    }
    // Quick number shortcut (1-9)
    if (code >= 0x1E && code <= 0x26) {
        uint8_t n = code - 0x1E;
        if (n < 9) {
            m_menuItem = n;
            renderSettingsMenu();  // show highlight only, need Enter to activate
        }
        return;
    }
    if (code == HID_ESC) {
        exitToNormal(nullptr);
        return;
    }
}

// ─── Render: Settings menu ────────────────────────────────────────────────

void KeyboardHandler::renderSettingsMenu() {
    char buf[256];
    int pos = 0;

    int ri = m_settings->getReadInterval();
    int di = m_settings->getDisplayInterval();
    int du = m_settings->getDisplayUnit();
    int as = m_settings->getAdcSamples();
    int sm = m_settings->getSleepMode();

    pos += snprintf(buf + pos, sizeof(buf) - pos, "=== SETTINGS ===\n");

    const char* row0 = (m_menuItem == 0) ? " > " : "   ";
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s1. WiFi Setup\n", row0);

    const char* row1 = (m_menuItem == 1) ? " > " : "   ";
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s2. Sensor Config\n", row1);

    const char* row2 = (m_menuItem == 2) ? " > " : "   ";
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s3. Read Int: %ds\n", row2, ri);

    const char* row3 = (m_menuItem == 3) ? " > " : "   ";
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s4. Disp Int: %ds\n", row3, di);

    const char* row4 = (m_menuItem == 4) ? " > " : "   ";
    const char* units[] = {"%", "raw", "V"};
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s5. Unit: %s\n", row4, units[du < 3 ? du : 0]);

    const char* row5 = (m_menuItem == 5) ? " > " : "   ";
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s6. ADC smpl: %d\n", row5, as);

    const char* row6 = (m_menuItem == 6) ? " > " : "   ";
    const char* sleep[] = {"Off", "Light", "Deep"};
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s7. Sleep: %s\n", row6, sleep[sm < 3 ? sm : 0]);

    const char* row7 = (m_menuItem == 7) ? " > " : "   ";
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s8. System Info\n", row7);

    const char* row8 = (m_menuItem == 8) ? " > " : "   ";
    pos += snprintf(buf + pos, sizeof(buf) - pos, "%s9. Exit\n", row8);

    pos += snprintf(buf + pos, sizeof(buf) - pos, "\nTab:next  Enter:sel  Esc:back");
    m_display->showCalibrationScreen(buf);
}

// ─── Settings sub-actions ─────────────────────────────────────────────────

void KeyboardHandler::adjustReadInterval(int delta) {
    int v = m_settings->getReadInterval() + delta;
    if (v < 1) v = 1; if (v > 300) v = 300;
    m_settings->setReadInterval(v);
    renderSettingsMenu();
}

void KeyboardHandler::adjustDisplayInterval(int delta) {
    int v = m_settings->getDisplayInterval() + delta;
    if (v < 1) v = 1; if (v > 600) v = 600;
    m_settings->setDisplayInterval(v);
    renderSettingsMenu();
}

void KeyboardHandler::cycleDisplayUnit() {
    int v = m_settings->getDisplayUnit() + 1;
    if (v > 2) v = 0;
    m_settings->setDisplayUnit(v);
    renderSettingsMenu();
}

void KeyboardHandler::adjustAdcSamples(int delta) {
    int v = m_settings->getAdcSamples() + delta;
    if (v < 1) v = 1; if (v > 100) v = 100;
    m_settings->setAdcSamples(v);
    renderSettingsMenu();
}

void KeyboardHandler::cycleSleepMode() {
    int v = m_settings->getSleepMode() + 1;
    if (v > 2) v = 0;
    m_settings->setSleepMode(v);
    renderSettingsMenu();
}

// ─── WiFi ─────────────────────────────────────────────────────────────────

void KeyboardHandler::doWifiScan() {
    m_display->showMessage("Scanning WiFi...");
    Serial.println("WiFi scan start");
    m_wifiCount = 0;
    int n = WiFi.scanNetworks();
    if (n < 0) {
        showBanner("Scan failed", "Try again later");
        return;
    }
    if (n > 9) n = 9;  // show at most 9
    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        // Truncate long SSIDs
        snprintf(m_wifiNetlist[i], sizeof(m_wifiNetlist[i]), "%s (%d)", ssid.c_str(), rssi);
    }
    m_wifiCount = n;
    if (n == 0) {
        showBanner("No networks found",
                    "Press Tab to retry",
                    "", "Esc: back");
        // Stay in wifi netlist mode so Tab can rescan
        m_mode = MODE_WIFI_NETLIST;
        m_menuItem = 0;
        return;
    }
    m_mode = MODE_WIFI_NETLIST;
    m_menuItem = 0;
    renderWifiNetlist();
    Serial.printf("WiFi: found %d networks\n", n);
}

void KeyboardHandler::renderWifiNetlist() {
    char buf[256];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "=== WiFi Networks ===\n\n");
    for (uint8_t i = 0; i < m_wifiCount; i++) {
        const char* arrow = (i == m_menuItem) ? " > " : "   ";
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%s%d. %s\n",
                        arrow, i + 1, m_wifiNetlist[i]);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "\n1-9:select  Tab:rescan  Esc:back");
    m_display->showCalibrationScreen(buf);
}

void KeyboardHandler::handleWifiNetlistKey(uint8_t code) {
    if (code == HID_ESC) {
        m_mode = MODE_SETTINGS;
        m_menuItem = 0;
        renderSettingsMenu();
        return;
    }
    if (code == HID_TAB) {
        doWifiScan();  // rescan
        return;
    }
    // 1-9 select network
    uint8_t idx = 255;
    if (code == 0x27) idx = 0;      // HID_0 → shouldn't happen but guard
    if (code >= 0x1E && code <= 0x26) idx = code - 0x1E;  // HID_1..HID_9
    if (idx >= m_wifiCount) return;

    // Extract SSID from the stored line (before " (RSSI)")
    char ssid[32];
    strncpy(ssid, m_wifiNetlist[idx], sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';
    char* paren = strchr(ssid, ' ');
    if (paren) *paren = '\0';

    // Save SSID to NVS immediately
    {
        Preferences prefs;
        prefs.begin("wifi", false);
        prefs.putString("ssid", ssid);
        prefs.end();
    }

    Serial.printf("WiFi: selected SSID=%s\n", ssid);

    // Enter password input
    m_mode = MODE_WIFI_PASS;
    m_textBuf[0] = '\0';
    m_textLen = 0;
    m_textMax = 31;

    char banner[128];
    snprintf(banner, sizeof(banner), "Password for:\n%s\n\nType, Enter=save\nEsc:cancel", ssid);
    m_display->showCalibrationScreen(banner);
}

void KeyboardHandler::commitWifiCredentials() {
    // Unused — now done inline in handleTextInputKey.
}

void KeyboardHandler::startWifiConnect() {
    Preferences prefs;
    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");
    prefs.end();

    if (ssid.length() == 0) {
        m_display->showMessage("No SSID saved");
        m_mode = MODE_SETTINGS;
        m_menuItem = 0;
        renderSettingsMenu();
        return;
    }

    showBanner("Connecting...", ssid.c_str());
    Serial.printf("WiFi: connecting to %s\n", ssid.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    // Give it a moment (non-blocking)
    m_display->showMessage("WiFi connecting...");
    delay(500);

    // Return to settings menu; connection proceeds asynchronously
    m_mode = MODE_SETTINGS;
    m_menuItem = 0;
    renderSettingsMenu();
}

// ─── Text input mode (SSID, password, sensor name) ────────────────────────

void KeyboardHandler::handleTextInputKey(uint8_t code) {
    if (code == HID_ESC) {
        // Cancel input — go back to previous menu
        if (m_mode == MODE_WIFI_SSID || m_mode == MODE_WIFI_PASS) {
            m_mode = MODE_WIFI_NETLIST;
            renderWifiNetlist();
        } else if (m_mode == MODE_SENSOR_RENAME) {
            m_mode = MODE_SENSOR_MENU;
            renderSensorMenu();
        }
        return;
    }

    if (code == HID_ENTER) {
        // Confirm input
        if (m_mode == MODE_WIFI_SSID) {
            // Save SSID to NVS and enter password mode
            {
                Preferences prefs;
                prefs.begin("wifi", false);
                prefs.putString("ssid", m_textBuf);
                prefs.end();
            }
            m_mode = MODE_WIFI_PASS;
            m_textBuf[0] = '\0';
            m_textLen = 0;
            m_textMax = 31;
            char buf[80];
            snprintf(buf, sizeof(buf), "Password:\n\nType, Enter=save\nEsc:cancel");
            m_display->showCalibrationScreen(buf);
        } else if (m_mode == MODE_WIFI_PASS) {
            // Save password to NVS and start connecting
            {
                Preferences prefs;
                prefs.begin("wifi", false);
                prefs.putString("pass", m_textBuf);
                prefs.end();
            }
            startWifiConnect();
        } else if (m_mode == MODE_SENSOR_RENAME) {
            applySensorRename();
        }
        return;
    }

    if (code == HID_BACKSP) {
        if (m_textLen > 0) {
            m_textLen--;
            m_textBuf[m_textLen] = '\0';
        }
        // Refresh display with current text
        if (m_mode == MODE_SENSOR_RENAME) {
            char buf[80];
            snprintf(buf, sizeof(buf), "Rename S%d:\n%s_",
                     m_sensorMenuItem + 1, m_textBuf);
            m_display->showCalibrationScreen(buf);
        } else {
            char buf[80];
            const char* label = (m_mode == MODE_WIFI_SSID) ? "SSID" : "Password";
            snprintf(buf, sizeof(buf), "%s:\n%s_", label, m_textBuf);
            m_display->showCalibrationScreen(buf);
        }
        return;
    }

    // Helper to refresh the text input display
    auto refreshInputDisplay = [&]() {
        if (m_mode == MODE_SENSOR_RENAME) {
            char buf[80];
            snprintf(buf, sizeof(buf), "Rename S%d:\n%s_",
                     m_sensorMenuItem + 1, m_textBuf);
            m_display->showCalibrationScreen(buf);
        } else {
            char buf[80];
            const char* label = (m_mode == MODE_WIFI_SSID) ? "SSID" : "Password";
            snprintf(buf, sizeof(buf), "%s:\n%s_", label, m_textBuf);
            m_display->showCalibrationScreen(buf);
        }
    };

    // Letters (a-z) — no shift handling for simplicity
    if (isAlpha(code)) {
        char ascii = 'a' + (code - HID_A);
        if (m_textLen < m_textMax) {
            m_textBuf[m_textLen++] = ascii;
            m_textBuf[m_textLen] = '\0';
        }
        refreshInputDisplay();
        return;
    }

    // Digits 1-9
    if (code >= 0x1E && code <= 0x26) {
        if (m_textLen < m_textMax) {
            m_textBuf[m_textLen++] = '1' + (code - 0x1E);
            m_textBuf[m_textLen] = '\0';
        }
        refreshInputDisplay();
        return;
    }
    if (code == 0x27) {   // HID_0
        if (m_textLen < m_textMax) {
            m_textBuf[m_textLen++] = '0';
            m_textBuf[m_textLen] = '\0';
        }
        refreshInputDisplay();
        return;
    }

    // Space, dot, minus, slash
    if (code == HID_SPACE || code == HID_DOT || code == HID_MINUS || code == HID_SLASH) {
        char c = hidToAscii(code);
        if (c && m_textLen < m_textMax) {
            m_textBuf[m_textLen++] = c;
            m_textBuf[m_textLen] = '\0';
        }
        refreshInputDisplay();
        return;
    }
}

// ─── Sensor config menu ───────────────────────────────────────────────────

void KeyboardHandler::enterSensorMenu() {
    m_mode = MODE_SENSOR_MENU;
    m_sensorMenuItem = 0;
    renderSensorMenu();
}

void KeyboardHandler::renderSensorMenu() {
    char buf[256];
    int pos = 0;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "=== Sensors ===\n\n");
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        const SensorConfig* cfg = m_cal->getConfigs();
        const char* arrow = (i == m_sensorMenuItem) ? " > " : "   ";
        const char* state = cfg[i].enabled ? "ON" : "OFF";
        const char* name = cfg[i].name;
        pos += snprintf(buf + pos, sizeof(buf) - pos,
                        "%sS%d [%s] %s\n", arrow, i + 1, state, name);
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "\nEnter:toggle  R:rename  Esc:back");
    m_display->showCalibrationScreen(buf);
}

void KeyboardHandler::handleSensorMenuKey(uint8_t code) {
    // Tab — next sensor
    if (code == HID_TAB) {
        m_sensorMenuItem = (m_sensorMenuItem + 1) % MAX_SENSORS;
        renderSensorMenu();
        return;
    }
    // 1-5 — direct select
    if (code >= HID_1 && code <= HID_5) {
        m_sensorMenuItem = code - HID_1;
        renderSensorMenu();
        return;
    }
    if (code == HID_ENTER) {
        // Toggle enabled
        uint8_t idx = m_sensorMenuItem;
        const SensorConfig* cfg = m_cal->getConfigs();
        if (cfg[idx].enabled) {
            m_cal->removeSensor(idx);
        } else {
            m_cal->enableSensor(idx);
        }
        // Re-read configs and refresh
        renderSensorMenu();
        return;
    }
    if (code == HID_R) {
        // Enter rename mode
        m_mode = MODE_SENSOR_RENAME;
        m_textBuf[0] = '\0';
        m_textLen = 0;
        m_textMax = 15;  // short sensor names
        char buf[80];
        snprintf(buf, sizeof(buf), "Rename S%d:\n_", m_sensorMenuItem + 1);
        m_display->showCalibrationScreen(buf);
        return;
    }
    if (code == HID_ESC) {
        m_mode = MODE_SETTINGS;
        m_menuItem = 0;
        renderSettingsMenu();
        return;
    }
}

void KeyboardHandler::applySensorRename() {
    if (m_textLen > 0) {
        m_cal->setName(m_sensorMenuItem, m_textBuf);
    }
    m_mode = MODE_SENSOR_MENU;
    renderSensorMenu();
}

// ─── System Info screen ───────────────────────────────────────────────────

void KeyboardHandler::showInfo() {
    char buf[192];
    int pos = 0;

    pos += snprintf(buf + pos, sizeof(buf) - pos, "=== System Info ===\n\n");

    // WiFi status
    if (WiFi.status() == WL_CONNECTED) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "WiFi: STA %s\n",
                        WiFi.localIP().toString().c_str());
    } else if (WiFi.getMode() == WIFI_AP) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "WiFi: AP mode\n");
    } else {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "WiFi: disconnected\n");
    }

    // Uptime
    unsigned long sec = millis() / 1000;
    int h = sec / 3600;
    int m = (sec % 3600) / 60;
    int s = sec % 60;
    pos += snprintf(buf + pos, sizeof(buf) - pos, "Up: %dh %dm %ds\n", h, m, s);

    // Sensors
    int en = 0;
    const SensorConfig* cfg = m_cal->getConfigs();
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        if (cfg[i].enabled) en++;
    }
    pos += snprintf(buf + pos, sizeof(buf) - pos, "Sensors: %d enabled\n", en);

    // Battery from Cardputer PMU
    int bat = M5Cardputer.Power.getBatteryLevel();
    if (bat >= 0) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "Battery: %d%%\n", bat);
    }

    // Heap
    pos += snprintf(buf + pos, sizeof(buf) - pos, "Heap: %d KB\n",
                    ESP.getFreeHeap() / 1024);

    pos += snprintf(buf + pos, sizeof(buf) - pos, "\nEsc: back");
    m_display->showCalibrationScreen(buf);
    // Info stays on screen; next key press handleSettingsKey redraws menu
}

// ─── Mode: CALIBRATION ────────────────────────────────────────────────────

void KeyboardHandler::handleCalibKey(uint8_t code) {
    switch (code) {
        case HID_C:    toggleCalibration();   break;
        case 0x1E: selectSensor(0); break;   // HID_1
        case 0x1F: selectSensor(1); break;   // HID_2
        case 0x20: selectSensor(2); break;   // HID_3
        case 0x21: selectSensor(3); break;   // HID_4
        case 0x22: selectSensor(4); break;   // HID_5
        case HID_D:    markDry();              break;
        case HID_W:    markWet();              break;
        case HID_R:    resetSensor();           break;
        case HID_ESC:
            m_mode = MODE_NORMAL;
            m_display->showMessage("Calibration cancelled");
            break;
        default: break;
    }
}

void KeyboardHandler::toggleCalibration() {
    if (m_mode == MODE_CALIB) {
        m_mode = MODE_NORMAL;
        m_display->showMessage("Calibration closed");
        Serial.println("Calibration mode OFF");
    } else {
        m_mode = MODE_CALIB;
        m_selSensor = 0;
        showBanner("Calibration mode",
                   "1-5: select sensor",
                   "D: dry  W: wet  R: reset",
                   "C: exit");
        Serial.println("Calibration mode ON");
    }
}

void KeyboardHandler::selectSensor(uint8_t idx) {
    if (idx >= MAX_SENSORS) return;
    m_selSensor = idx;
    const SensorConfig* cfg = m_cal->getConfigs();
    char buf[48];
    snprintf(buf, sizeof(buf), "Selected S%d [%s]",
             idx + 1, cfg[idx].enabled ? "ON" : "OFF");
    m_display->showCalibrationScreen(buf);
    Serial.printf("Keyboard: selected sensor %d\n", idx + 1);
}

void KeyboardHandler::markDry() {
    if (!m_sensorManager) return;
    const SensorConfig* cfg = m_cal->getConfigs();
    if (!cfg[m_selSensor].enabled) {
        m_display->showMessage("Sensor disabled");
        return;
    }
    int raw = m_sensorManager->getRaw(m_selSensor);
    if (raw < 0) raw = 0;
    m_cal->setDryValue(m_selSensor, raw);
    m_sensorManager->setCalibration(m_selSensor,
        m_cal->getDryValue(m_selSensor),
        m_cal->getWetValue(m_selSensor));
    char buf[48];
    snprintf(buf, sizeof(buf), "Dry S%d = %d", m_selSensor + 1, raw);
    m_display->showCalibrationScreen(buf);
    Serial.printf("Calib: dry S%d = %d\n", m_selSensor + 1, raw);
}

void KeyboardHandler::markWet() {
    if (!m_sensorManager) return;
    const SensorConfig* cfg = m_cal->getConfigs();
    if (!cfg[m_selSensor].enabled) {
        m_display->showMessage("Sensor disabled");
        return;
    }
    int raw = m_sensorManager->getRaw(m_selSensor);
    if (raw < 0) raw = 0;
    m_cal->setWetValue(m_selSensor, raw);
    m_sensorManager->setCalibration(m_selSensor,
        m_cal->getDryValue(m_selSensor),
        m_cal->getWetValue(m_selSensor));
    char buf[48];
    snprintf(buf, sizeof(buf), "Wet S%d = %d", m_selSensor + 1, raw);
    m_display->showCalibrationScreen(buf);
    Serial.printf("Calib: wet S%d = %d\n", m_selSensor + 1, raw);
}

void KeyboardHandler::resetSensor() {
    if (!m_sensorManager) return;
    m_cal->resetToDefaults(m_selSensor);
    m_sensorManager->setCalibration(m_selSensor,
        m_cal->getDryValue(m_selSensor),
        m_cal->getWetValue(m_selSensor));
    m_display->showMessage("Calibration reset");
    Serial.printf("Calib: reset S%d\n", m_selSensor + 1);
}

// ─── Pump / Auto / Threshold (also used in NORMAL mode) ──────────────────

void KeyboardHandler::togglePump() {
    if (!m_pump) return;
    if (m_pump->isRunning()) {
        m_pump->stop();
        m_display->showMessage("Pump OFF");
        Serial.println("Pump stopped");
    } else {
        m_pump->run(5000);  // 5 seconds
        m_display->showMessage("Pump ON (5s)");
        Serial.println("Pump started (5s)");
    }
}

void KeyboardHandler::adjustThreshold(int delta) {
    if (!m_pump) return;
    int cur = m_pump->getAutoThreshold();
    int v = cur + delta;
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    m_pump->setAutoThreshold(v);
    char buf[32];
    snprintf(buf, sizeof(buf), "Threshold: %d%%", v);
    m_display->showMessage(buf);
}

void KeyboardHandler::toggleAutoMode() {
    if (!m_pump) return;
    bool on = !m_pump->isAutoModeEnabled();
    m_pump->setAutoModeEnabled(on);
    m_display->showMessage(on ? "Auto ON" : "Auto OFF");
    Serial.printf("Auto mode: %s\n", on ? "ON" : "OFF");
}

// ─── Helper ───────────────────────────────────────────────────────────────

void KeyboardHandler::showBanner(const char* line1, const char* line2,
                                 const char* line3, const char* line4) {
    char buf[192];
    int pos = 0;
    if (line1) pos += snprintf(buf + pos, sizeof(buf) - pos, "%s\n", line1);
    if (line2) pos += snprintf(buf + pos, sizeof(buf) - pos, "%s\n", line2);
    if (line3) pos += snprintf(buf + pos, sizeof(buf) - pos, "%s\n", line3);
    if (line4) pos += snprintf(buf + pos, sizeof(buf) - pos, "%s\n", line4);
    m_display->showCalibrationScreen(buf);
}
