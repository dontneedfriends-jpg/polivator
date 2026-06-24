#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Preferences.h>
#include <ArduinoJson.h>
#include "../Common/Types.h"

class Calibration {
public:
    Calibration();
    bool begin(const uint8_t* defaultPins, uint8_t count);
    void loadAllConfigs(SensorConfig* configs, const uint8_t* defaultPins, uint8_t count);
    void saveAllConfigs(const SensorConfig* configs, uint8_t count);
    void writeCalibration(const SensorConfig* configs, uint8_t count);
    void setDryRaw(uint8_t index, int value);
    void setWetRaw(uint8_t index, int value);
    void setName(uint8_t index, const char* name);
    void setPin(uint8_t index, uint8_t pin);
    void setDryValue(uint8_t index, int value);
    void setWetValue(uint8_t index, int value);
    void resetToDefaults(uint8_t index);
    void removeSensor(uint8_t index);
    void enableSensor(uint8_t index);
    uint8_t getPin(uint8_t index) const;
    int getDryValue(uint8_t index) const;
    int getWetValue(uint8_t index) const;
    const char* getName(uint8_t index) const;
    const SensorConfig* getConfigs() const;

private:
    Preferences prefs;
    SensorConfig m_configs[MAX_SENSORS];
    uint8_t m_count;
};

#endif // CALIBRATION_H
