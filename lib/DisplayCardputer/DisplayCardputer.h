#ifndef DISPLAY_CARDPUTER_H
#define DISPLAY_CARDPUTER_H

#include "../Common/IDisplay.h"
#include <M5GFX.h>

class DisplayCardputer : public IDisplay {
public:
    DisplayCardputer();
    ~DisplayCardputer() override = default;

    // IDisplay interface
    void begin() override;
    void showStatus(uint8_t sensorCount,
                    const SensorConfig* configs,
                    const int* percentages,
                    const int* rawValues,
                    const char* wifiStatus) override;
    void showCalibrationScreen(const char* step = "") override;
    void showMessage(const char* msg) override;
    void deepSleep() override;
    void forceFullRefresh() override;
    void startTask() override;
    void stopTask() override;
    bool isBusy() const override;
    bool isTaskRunning() const override;

    // Cardputer extras
    void setBatteryLevel(int pct);

private:
    static void displayTaskFunc(void* param);

    // Rendering
    void renderDashboard(uint8_t sensorCount,
                         const SensorConfig* configs,
                         const int* percentages,
                         const int* rawValues,
                         const char* wifiStatus);
    void renderCalibration(const char* step);
    void renderMessage(const char* msg);

    // UI components
    void drawStatusBar(const char* wifiStatus);
    void drawHeroCard(int& y, int pct, const char* name, uint8_t sensorIdx);
    void drawSensorRow(int y, int pct, const char* name, uint8_t sensorIdx, bool highlight);
    void drawProgressBar(int x, int y, int w, int h, int pct, uint16_t color);
    void drawPanel(int x, int y, int w, int h, uint16_t bg, uint16_t border);
    void drawFooter();

    // Icons
    void drawIconDrop(int x, int y, uint16_t color);
    void drawIconWiFi(int x, int y, int strength);
    void drawIconBattery(int x, int y, int pct);
    void drawIconPump(int x, int y, uint16_t color);
    void drawIconGear(int x, int y, uint16_t color);
    void drawIconCheck(int x, int y, uint16_t color);

    M5GFX* m_gfx;

    // Task
    TaskHandle_t m_taskHandle;
    bool m_taskRunning;
    volatile bool m_busy;
    bool m_firstUpdate;

    // Shared data (written from main thread, read from display task)
    volatile bool m_statusPending;
    volatile bool m_calibrationPending;
    volatile bool m_messagePending;

    uint8_t m_sensorCount;
    SensorConfig m_configs[MAX_SENSORS];
    int m_percentages[MAX_SENSORS];
    int m_rawValues[MAX_SENSORS];
    char m_wifiStatus[32];
    char m_calibStep[128];
    char m_messageText[128];

    int m_batteryPct;
};

#endif // DISPLAY_CARDPUTER_H
