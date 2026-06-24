#include "Sensor.h"
#include <Arduino.h>
#include "Calibration.h"
#include "Settings.h"

SensorManager::SensorManager() : cal(nullptr) {
}

bool SensorManager::begin(Calibration* calibration, Settings* settings) {
    if (!calibration) {
        Serial.println("SensorManager::begin FAILED - null calibration");
        return false;
    }
    cal = calibration;
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    int samples = settings ? settings->getAdcSamples() : 10;
    if (samples < 1) samples = 1;
    if (samples > 100) samples = 100;
    m_samples = samples;
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        sensors[i].pin = cal->getPin(i);
        sensors[i].dry = cal->getDryValue(i);
        sensors[i].wet = cal->getWetValue(i);
        sensors[i].raw = 0;
        sensors[i].percent = 0;
        sensors[i].voltage = 0.0f;
        // Do NOT call pinMode on ADC pins — it can detach the ADC channel.
        // analogRead will configure the pin correctly internally.
    }
    Serial.println("SensorManager::begin OK");
    return true;
}

void SensorManager::readAll() {
    const SensorConfig* configs = cal->getConfigs();
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        if (!configs[i].enabled) {
            sensors[i].raw = 0;
            sensors[i].percent = 0;
            sensors[i].voltage = 0.0f;
            continue;
        }
        long sum = 0;
        for (int j = 0; j < m_samples; j++) {
            sum += analogRead(sensors[i].pin);
        }
        sensors[i].raw = sum / m_samples;
        int raw = sensors[i].raw;
        if (raw <= sensors[i].wet) {
            sensors[i].percent = 100;
        } else if (raw >= sensors[i].dry) {
            sensors[i].percent = 0;
        } else {
            int range = sensors[i].dry - sensors[i].wet;
            if (range == 0) sensors[i].percent = 0;
            else sensors[i].percent = (sensors[i].dry - raw) * 100 / range;
        }
        sensors[i].voltage = raw * (3.3f / 4095.0f);
    }
}

int SensorManager::getRaw(uint8_t index) {
    if (index >= MAX_SENSORS) return 0;
    return sensors[index].raw;
}

int SensorManager::getPercent(uint8_t index) {
    if (index >= MAX_SENSORS) return 0;
    return sensors[index].percent;
}

float SensorManager::getVoltage(uint8_t index) {
    if (index >= MAX_SENSORS) return 0.0f;
    return sensors[index].voltage;
}

uint8_t SensorManager::getCount() {
    return MAX_SENSORS;
}

uint8_t SensorManager::getEnabledCount() {
    if (!cal) return 0;
    const SensorConfig* configs = cal->getConfigs();
    uint8_t n = 0;
    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        if (configs[i].enabled) n++;
    }
    return n;
}

void SensorManager::setCalibration(uint8_t index, uint16_t dry, uint16_t wet) {
    if (index >= MAX_SENSORS) return;
    sensors[index].pin = cal->getPin(index);
    sensors[index].dry = dry;
    sensors[index].wet = wet;
    if (cal) {
        cal->setDryRaw(index, dry);
        cal->setWetRaw(index, wet);
    }
}
