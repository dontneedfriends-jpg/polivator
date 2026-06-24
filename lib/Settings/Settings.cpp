#include "Settings.h"

Settings::Settings()
  : m_readInterval(2),
    m_displayInterval(30),
    m_webInterval(2),
    m_updatePriority(0),
    m_sleepMode(0),
    m_otaEnabled(true),
    m_debugEnabled(true),
    m_displayUnit(0),
    m_adcSamples(10) {}

int Settings::getReadInterval() const { return m_readInterval; }
void Settings::setReadInterval(int interval) {
  if (interval < 1) interval = 1;
  if (interval > 3600) interval = 3600;
  m_readInterval = interval;
}

int Settings::getDisplayInterval() const { return m_displayInterval; }
void Settings::setDisplayInterval(int interval) {
  if (interval < 1) interval = 1;
  if (interval > 3600) interval = 3600;
  m_displayInterval = interval;
}

int Settings::getWebInterval() const { return m_webInterval; }
void Settings::setWebInterval(int interval) {
  if (interval < 1) interval = 1;
  if (interval > 3600) interval = 3600;
  m_webInterval = interval;
}

int Settings::getUpdatePriority() const { return m_updatePriority; }
void Settings::setUpdatePriority(int priority) {
  if (priority < 0) priority = 0;
  if (priority > 2) priority = 2;
  m_updatePriority = priority;
}

int Settings::getSleepMode() const { return m_sleepMode; }
void Settings::setSleepMode(int mode) {
  if (mode < 0) mode = 0;
  if (mode > 2) mode = 2;
  m_sleepMode = mode;
}

bool Settings::getOtaEnabled() const { return m_otaEnabled; }
void Settings::setOtaEnabled(bool enabled) {
  m_otaEnabled = enabled;
}

bool Settings::getDebugEnabled() const { return m_debugEnabled; }
void Settings::setDebugEnabled(bool enabled) {
  m_debugEnabled = enabled;
}

int Settings::getDisplayUnit() const { return m_displayUnit; }
void Settings::setDisplayUnit(int unit) {
  if (unit < 0) unit = 0;
  if (unit > 2) unit = 2;
  m_displayUnit = unit;
}

int Settings::getAdcSamples() const { return m_adcSamples; }
void Settings::setAdcSamples(int samples) {
  if (samples < 1) samples = 1;
  if (samples > 100) samples = 100;
  m_adcSamples = samples;
}
