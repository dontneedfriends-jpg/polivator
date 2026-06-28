#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <Arduino.h>
#include "../Common/Types.h"
#include "../Common/IDisplay.h"
#include "../Calibration/Calibration.h"
#include "../Sensor/Sensor.h"
#include "../Settings/Settings.h"
#include "../WaterPump/WaterPump.h"

/// Keyboard handler with settings menu for Cardputer.
///
/// Modes (m_mode):
///   MODE_NORMAL     — direct keys: P (pump), A (auto), +/- (threshold), M (menu)
///   MODE_CALIB      — calibration: C, 1-5, D, W, R
///   MODE_SETTINGS   — settings menu navigation
///   MODE_WIFI_NETLIST  — scan results shown, select with 1-9
///   MODE_WIFI_SSID  — typing SSID
///   MODE_WIFI_PASS  — typing password
///   MODE_SENSOR_MENU  — sensor enable/rename list
///   MODE_SENSOR_RENAME — renaming a sensor
class KeyboardHandler {
public:
    KeyboardHandler(IDisplay* display,
                    Calibration* calibration,
                    SensorManager* sensorManager,
                    Settings* settings,
                    WaterPump* pump);

    /// Call from loop() every iteration.
    void tick();

private:
    // ─── Dependencies ──────────────────────────────────────────────────
    IDisplay*       m_display;
    Calibration*    m_cal;
    SensorManager*  m_sensorManager;
    Settings*       m_settings;
    WaterPump*      m_pump;

    // ─── Mode ──────────────────────────────────────────────────────────
    enum Mode : uint8_t {
        MODE_NORMAL,
        MODE_CALIB,
        MODE_SETTINGS,
        MODE_WIFI_NETLIST,
        MODE_WIFI_SSID,
        MODE_WIFI_PASS,
        MODE_SENSOR_MENU,
        MODE_SENSOR_RENAME,
    };
    Mode m_mode;

    // ─── Sub-state ─────────────────────────────────────────────────────
    uint8_t  m_selSensor;      // selected sensor index (0-4) for calibration
    uint8_t  m_menuItem;       // current menu item index
    uint8_t  m_sensorMenuItem; // current sensor index in sensor config menu
    char     m_textBuf[48];    // buffer for SSID/password/name input
    uint8_t  m_textLen;        // current length in m_textBuf
    uint8_t  m_textMax;        // max chars for current text input
    uint8_t  m_wifiCount;      // number of networks found
    char     m_wifiNetlist[9][32]; // up to 9 SSIDs from scan

    // ─── Helpers ───────────────────────────────────────────────────────
    void renderSettingsMenu();
    void renderWifiNetlist();
    void renderWifiStatus();
    void renderSensorMenu();
    void renderSystemInfo();

    void handleNormalKey(uint8_t code);
    void handleCalibKey(uint8_t code);
    void handleSettingsKey(uint8_t code);
    void handleWifiNetlistKey(uint8_t code);
    void handleTextInputKey(uint8_t code);       // SSID / password / rename
    void handleSensorMenuKey(uint8_t code);

    void commitWifiCredentials();
    void startWifiConnect();
    void doWifiScan();
    void applySensorRename();

    void showBanner(const char* line1, const char* line2 = nullptr,
                    const char* line3 = nullptr, const char* line4 = nullptr);
};

#endif // KEYBOARD_H
