#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Preferences.h>

class Calibration {
public:
    Calibration();
    void begin();
    uint16_t getDryValue();
    uint16_t getWetValue();
    void setDryValue(uint16_t value);
    void setWetValue(uint16_t value);
    void resetToDefaults();

private:
    Preferences prefs;
    static constexpr const char* KEY_DRY = "dry";
    static constexpr const char* KEY_WET = "wet";
};

#endif // CALIBRATION_H
