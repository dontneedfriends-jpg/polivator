#include "Calibration.h"

Calibration::Calibration() {
}

void Calibration::begin() {
    prefs.begin("calib", false);
    if (!prefs.isKey(KEY_DRY) || !prefs.isKey(KEY_WET)) {
        resetToDefaults();
    }
}

uint16_t Calibration::getDryValue() {
    return prefs.getUShort(KEY_DRY, 3000);
}

uint16_t Calibration::getWetValue() {
    return prefs.getUShort(KEY_WET, 1200);
}

void Calibration::setDryValue(uint16_t value) {
    prefs.putUShort(KEY_DRY, value);
}

void Calibration::setWetValue(uint16_t value) {
    prefs.putUShort(KEY_WET, value);
}

void Calibration::resetToDefaults() {
    prefs.putUShort(KEY_DRY, 3000);
    prefs.putUShort(KEY_WET, 1200);
}
