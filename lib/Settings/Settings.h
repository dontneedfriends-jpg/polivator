#ifndef SETTINGS_H
#define SETTINGS_H

class Settings {
public:
  Settings();
  // Sensor reading interval, seconds
  int getReadInterval() const;
  void setReadInterval(int interval);
  // Display full refresh interval, seconds
  int getDisplayInterval() const;
  void setDisplayInterval(int interval);
  // Web / SSE update interval, seconds
  int getWebInterval() const;
  void setWebInterval(int interval);
  // 0 = time priority, 1 = change priority, 2 = balanced
  int getUpdatePriority() const;
  void setUpdatePriority(int priority);
  // Deep sleep mode: 0 = off, 1 = light, 2 = deep
  int getSleepMode() const;
  void setSleepMode(int mode);
  // OTA enabled
  bool getOtaEnabled() const;
  void setOtaEnabled(bool enabled);
  // Serial debug enabled
  bool getDebugEnabled() const;
  void setDebugEnabled(bool enabled);
  // Display unit: 0 = percentage, 1 = raw ADC, 2 = voltage
  int getDisplayUnit() const;
  void setDisplayUnit(int unit);
  // Number of samples per ADC reading
  int getAdcSamples() const;
  void setAdcSamples(int samples);
private:
  int m_readInterval;
  int m_displayInterval;
  int m_webInterval;
  int m_updatePriority;
  int m_sleepMode;
  bool m_otaEnabled;
  bool m_debugEnabled;
  int m_displayUnit;
  int m_adcSamples;
};

#endif
