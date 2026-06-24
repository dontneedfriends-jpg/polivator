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

bool Settings::begin() {
  prefs.begin("settings", false);
  m_readInterval = prefs.getInt("readInt", m_readInterval);
  m_displayInterval = prefs.getInt("dispInt", m_displayInterval);
  m_webInterval = prefs.getInt("webInt", m_webInterval);
  m_updatePriority = prefs.getInt("priority", m_updatePriority);
  m_sleepMode = prefs.getInt("sleep", m_sleepMode);
  m_otaEnabled = prefs.getBool("ota", m_otaEnabled);
  m_debugEnabled = prefs.getBool("debug", m_debugEnabled);
  m_displayUnit = prefs.getInt("unit", m_displayUnit);
  m_adcSamples = prefs.getInt("adc", m_adcSamples);
  return true;
}

void Settings::save() {
  prefs.putInt("readInt", m_readInterval);
  prefs.putInt("dispInt", m_displayInterval);
  prefs.putInt("webInt", m_webInterval);
  prefs.putInt("priority", m_updatePriority);
  prefs.putInt("sleep", m_sleepMode);
  prefs.putBool("ota", m_otaEnabled);
  prefs.putBool("debug", m_debugEnabled);
  prefs.putInt("unit", m_displayUnit);
  prefs.putInt("adc", m_adcSamples);
}

int Settings::getReadInterval() const { return m_readInterval; }
void Settings::setReadInterval(int interval) {
  if (interval < 1) interval = 1;
  if (interval > 3600) interval = 3600;
  m_readInterval = interval;
  save();
}

int Settings::getDisplayInterval() const { return m_displayInterval; }
void Settings::setDisplayInterval(int interval) {
  if (interval < 1) interval = 1;
  if (interval > 3600) interval = 3600;
  m_displayInterval = interval;
  save();
}

int Settings::getWebInterval() const { return m_webInterval; }
void Settings::setWebInterval(int interval) {
  if (interval < 1) interval = 1;
  if (interval > 3600) interval = 3600;
  m_webInterval = interval;
  save();
}

int Settings::getUpdatePriority() const { return m_updatePriority; }
void Settings::setUpdatePriority(int priority) {
  if (priority < 0) priority = 0;
  if (priority > 2) priority = 2;
  m_updatePriority = priority;
  save();
}

int Settings::getSleepMode() const { return m_sleepMode; }
void Settings::setSleepMode(int mode) {
  if (mode < 0) mode = 0;
  if (mode > 2) mode = 2;
  m_sleepMode = mode;
  save();
}

bool Settings::getOtaEnabled() const { return m_otaEnabled; }
void Settings::setOtaEnabled(bool enabled) {
  m_otaEnabled = enabled;
  save();
}

bool Settings::getDebugEnabled() const { return m_debugEnabled; }
void Settings::setDebugEnabled(bool enabled) {
  m_debugEnabled = enabled;
  save();
}

int Settings::getDisplayUnit() const { return m_displayUnit; }
void Settings::setDisplayUnit(int unit) {
  if (unit < 0) unit = 0;
  if (unit > 2) unit = 2;
  m_displayUnit = unit;
  save();
}

int Settings::getAdcSamples() const { return m_adcSamples; }
void Settings::setAdcSamples(int samples) {
  if (samples < 1) samples = 1;
  if (samples > 100) samples = 100;
  m_adcSamples = samples;
  save();
}
