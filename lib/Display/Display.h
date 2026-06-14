#ifndef DISPLAY_H
#define DISPLAY_H

#include <GxEPD2_BW.h>
#include <GxEPD2_213_BN.h>

// Default SPI pins for ESP32-S3 WeActStudio e-ink display
#ifndef EINK_CS
#define EINK_CS 10
#endif
#ifndef EINK_DC
#define EINK_DC 9
#endif
#ifndef EINK_RST
#define EINK_RST 8
#endif
#ifndef EINK_BUSY
#define EINK_BUSY 7
#endif
#ifndef EINK_MOSI
#define EINK_MOSI 11
#endif
#ifndef EINK_SCLK
#define EINK_SCLK 12
#endif

class Display {
public:
  Display();
  ~Display();
  void begin();
  void showStatus(float moisturePercent, int rawValue, bool calibrating, const char* wifiStatus);
  void showCalibrationScreen(const char* step);
  void showMessage(const char* msg);
  void deepSleep();

private:
  GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> m_display;
};

#endif // DISPLAY_H
