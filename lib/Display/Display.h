#ifndef DISPLAY_H
#define DISPLAY_H

#include <GxEPD2_3C.h>
#include "../Common/Types.h"

// Default SPI pins for ESP32-S3 WeActStudio tri-color 2.13" e-ink display
#ifndef EINK_CS
#define EINK_CS 5
#endif
#ifndef EINK_DC
#define EINK_DC 17
#endif
#ifndef EINK_RST
#define EINK_RST 16
#endif
#ifndef EINK_BUSY
#define EINK_BUSY 4
#endif
#ifndef EINK_MOSI
#define EINK_MOSI 21
#endif
#ifndef EINK_SCLK
#define EINK_SCLK 18
#endif

class Display {
public:
  Display();
  ~Display();
  void begin();
  // Queue an async redraw — returns immediately, actual update happens on the display task.
  void showStatus(uint8_t sensorCount, const SensorConfig* configs, const int* percentages, const int* rawValues, const char* wifiStatus);
  void showCalibrationScreen(const char* step = "");
  void showMessage(const char* msg);
  void deepSleep();
  void forceFullRefresh();
  // Start/stop the dedicated display task (run on core 0). Keeps web/OTA responsive during e-ink refresh.
  void startTask();
  void stopTask();
  bool isBusy() const;

private:
  // Tri-color display, but we use only BLACK and WHITE
  GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT> m_display = GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT>(GxEPD2_213_Z98c(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY));
  int m_prevPercentages[5];
  int m_prevRaw[5];
  char m_prevWifi[16];
  bool m_firstUpdate;
  bool m_initialFullRefreshDone;
  unsigned long m_fullRefreshCounter;
  bool m_taskRunning;
  volatile bool m_updatePending;
  volatile bool m_calibrationPending;
  volatile bool m_messagePending;
  char m_calibStep[24];
  char m_messageText[32];
  // Snapshot for the queued status update
  SensorConfig m_snapConfigs[5];
  int m_snapPercentages[5];
  int m_snapRaw[5];
  uint8_t m_snapCount;
  char m_snapWifi[16];
  // Set by the task while it owns the SPI bus + GxEPD2
  volatile bool m_busy;

  bool hasChanged(uint8_t sensorCount, const int* percentages, const int* rawValues, const char* wifiStatus);
  void drawStatus(uint8_t sensorCount, const SensorConfig* configs, const int* percentages, const int* rawValues, const char* wifiStatus);
  // Synchronous render helpers used by the task
  void renderStatus();
  void renderCalibration(const char* step);
  void renderMessage(const char* msg);
  static void taskThunk(void* arg);
  void taskLoop();

public:
  // True once the display task has been started.
  bool isTaskRunning() const { return m_taskRunning; }
};

#endif // DISPLAY_H
