#include "Display.h"
#include <SPI.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

// Constructor: initialize display with default pins
Display::Display() : m_display(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY) {
}

Display::~Display() {
}

void Display::begin() {
  // Initialize SPI with custom pins
  SPI.begin(EINK_SCLK, -1, EINK_MOSI, -1);
  // Initialize the display
  m_display.init(115200, true, 4000000, EINK_MOSI, -1);
  // Configure for landscape orientation
  m_display.setRotation(1);
  // Fill with white and display
  m_display.fillScreen(GxEPD_WHITE);
  m_display.display(false); // full refresh
}

void Display::showStatus(float moisturePercent, int rawValue, bool calibrating, const char* wifiStatus) {
  m_display.setFullWindow();
  m_display.fillScreen(GxEPD_WHITE);
  m_display.setTextColor(GxEPD_BLACK);
  m_display.setFont(&FreeSans9pt7b);
  
  int16_t tbx, tby; uint16_t tbw, tbh;
  // Center text at top
  if (calibrating) {
    m_display.getTextBounds("Calibrating...", 0, 0, &tbx, &tby, &tbw, &tbh);
    m_display.setCursor((m_display.width() - tbw) / 2, tbh + 5);
    m_display.print("Calibrating...");
  } else {
    m_display.setCursor(10, 30);
    m_display.print("Moisture: ");
    m_display.print(moisturePercent, 1);
    m_display.println("%");
    
    m_display.setCursor(10, 60);
    m_display.print("Raw: ");
    m_display.println(rawValue);
    
    m_display.setCursor(10, 90);
    m_display.print("WiFi: ");
    m_display.println(wifiStatus);
  }
  
  m_display.display(false); // full refresh
}

void Display::showCalibrationScreen(const char* step) {
  m_display.setFullWindow();
  m_display.fillScreen(GxEPD_WHITE);
  m_display.setTextColor(GxEPD_BLACK);
  m_display.setFont(&FreeSans12pt7b);
  
  // Center "Calibrating..." at top
  int16_t tbx, tby; uint16_t tbw, tbh;
  m_display.getTextBounds("Calibrating...", 0, 0, &tbx, &tby, &tbw, &tbh);
  m_display.setCursor((m_display.width() - tbw) / 2, tbh + 10);
  m_display.println("Calibrating...");
  
  // Display step below
  m_display.setFont(&FreeSans9pt7b);
  m_display.getTextBounds(step, 0, 0, &tbx, &tby, &tbw, &tbh);
  m_display.setCursor((m_display.width() - tbw) / 2, m_display.height() / 2);
  m_display.println(step);
  
  m_display.display(false);
}

void Display::showMessage(const char* msg) {
  m_display.setFullWindow();
  m_display.fillScreen(GxEPD_WHITE);
  m_display.setTextColor(GxEPD_BLACK);
  m_display.setFont(&FreeSans9pt7b);
  
  int16_t tbx, tby; uint16_t tbw, tbh;
  m_display.getTextBounds(msg, 0, 0, &tbx, &tby, &tbw, &tbh);
  m_display.setCursor((m_display.width() - tbw) / 2, (m_display.height() + tbh) / 2);
  m_display.println(msg);
  
  m_display.display(false);
}

void Display::deepSleep() {
  m_display.powerDown(); // or m_display.hibernate()? powerDown is enough
  // Optionally: m_display.sleep(); // but powerDown may be sufficient
}
