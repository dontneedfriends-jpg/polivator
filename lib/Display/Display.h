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
  void showStatus(uint8_t sensorCount, const SensorConfig* configs, const int* percentages, const int* rawValues, const char* wifiStatus);
  void showCalibrationScreen(const char* step = "");
  void showMessage(const char* msg);
  void deepSleep();
  void forceFullRefresh();

private:
  // Tri-color display, but we use only BLACK and WHITE
  GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT> m_display = GxEPD2_3C<GxEPD2_213_Z98c, GxEPD2_213_Z98c::HEIGHT>(GxEPD2_213_Z98c(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY));
  int m_prevPercentages[5];
  int m_prevRaw[5];
  char m_prevWifi[16];
  bool m_firstUpdate;
  bool m_initialFullRefreshDone;
  unsigned long m_fullRefreshCounter;

  bool hasChanged(uint8_t sensorCount, const int* percentages, const int* rawValues, const char* wifiStatus);
  void drawStatus(uint8_t sensorCount, const SensorConfig* configs, const int* percentages, const int* rawValues, const char* wifiStatus);
};

#endif // DISPLAY_H
