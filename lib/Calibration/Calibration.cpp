#include "Calibration.h"
#include <string.h>

Calibration::Calibration() : m_count(0) {
}

bool Calibration::begin(const uint8_t* defaultPins, uint8_t count) {
    prefs.begin("calib", false);
    m_count = count;
    loadAllConfigs(m_configs, defaultPins, count);
    Serial.println("Calibration::begin OK");
    return true;
}

void Calibration::loadAllConfigs(SensorConfig* configs, const uint8_t* defaultPins, uint8_t count) {
    String jsonStr = prefs.getString("config", "");
    if (jsonStr.length() == 0) {
        for (uint8_t i = 0; i < count; i++) {
            configs[i].pin = defaultPins[i];
            configs[i].dryRaw = 3000;
            configs[i].wetRaw = 1200;
            configs[i].enabled = true;
            snprintf(configs[i].name, 32, "Sensor %d", i+1);
        }
    } else {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonStr);
        if (error) {
            for (uint8_t i = 0; i < count; i++) {
                configs[i].pin = defaultPins[i];
                configs[i].dryRaw = 3000;
                configs[i].wetRaw = 1200;
                configs[i].enabled = true;
                snprintf(configs[i].name, 32, "Sensor %d", i+1);
            }
        } else {
            JsonArray arr = doc.as<JsonArray>();
            if (!arr) {
                for (uint8_t i = 0; i < count; i++) {
                    configs[i].pin = defaultPins[i];
                    configs[i].dryRaw = 3000;
                    configs[i].wetRaw = 1200;
                    configs[i].enabled = true;
                    snprintf(configs[i].name, 32, "Sensor %d", i+1);
                }
            } else {
                for (uint8_t i = 0; i < count; i++) {
                    configs[i].pin = defaultPins[i];
                    if (i < arr.size()) {
                        JsonObject obj = arr[i];
                        configs[i].pin = obj["pin"] | defaultPins[i];
                        configs[i].dryRaw = obj["dryRaw"] | 3000;
                        configs[i].wetRaw = obj["wetRaw"] | 1200;
                        const char* name = obj["name"] | "";
                        if (strlen(name) > 0) {
                            strncpy(configs[i].name, name, 31);
                            configs[i].name[31] = '\0';
                        } else {
                            snprintf(configs[i].name, 32, "Sensor %d", i+1);
                        }
                        configs[i].enabled = obj["enabled"] | true;
                    } else {
                        configs[i].dryRaw = 3000;
                        configs[i].wetRaw = 1200;
                        configs[i].enabled = true;
                        snprintf(configs[i].name, 32, "Sensor %d", i+1);
                    }
                }
            }
        }
    }
}

void Calibration::saveAllConfigs(const SensorConfig* configs, uint8_t count) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (uint8_t i = 0; i < count; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = configs[i].name;
        obj["pin"] = configs[i].pin;
        obj["dryRaw"] = configs[i].dryRaw;
        obj["wetRaw"] = configs[i].wetRaw;
        obj["enabled"] = configs[i].enabled;
    }
    String jsonStr;
    serializeJson(doc, jsonStr);
    prefs.putString("config", jsonStr);
}

void Calibration::writeCalibration(const SensorConfig* configs, uint8_t count) {
    SensorConfig adjustedConfigs[MAX_SENSORS];
    for (uint8_t i = 0; i < count; i++) {
        adjustedConfigs[i] = configs[i];
        if (adjustedConfigs[i].wetRaw == adjustedConfigs[i].dryRaw) {
            if (adjustedConfigs[i].dryRaw > 0) {
                adjustedConfigs[i].wetRaw = adjustedConfigs[i].dryRaw - 1;
            } else {
                adjustedConfigs[i].dryRaw = adjustedConfigs[i].wetRaw + 1;
            }
        }
    }
    saveAllConfigs(adjustedConfigs, count);
}

void Calibration::setDryRaw(uint8_t index, int value) {
    if (index < m_count) {
        m_configs[index].dryRaw = value;
        writeCalibration(m_configs, m_count);
    }
}

void Calibration::setWetRaw(uint8_t index, int value) {
    if (index < m_count) {
        m_configs[index].wetRaw = value;
        writeCalibration(m_configs, m_count);
    }
}

void Calibration::setName(uint8_t index, const char* name) {
    if (index < m_count) {
        strncpy(m_configs[index].name, name, 31);
        m_configs[index].name[31] = '\0';
        writeCalibration(m_configs, m_count);
    }
}

void Calibration::setPin(uint8_t index, uint8_t pin) {
    if (index < m_count) {
        m_configs[index].pin = pin;
        writeCalibration(m_configs, m_count);
    }
}

void Calibration::setDryValue(uint8_t index, int value) {
    setDryRaw(index, value);
}

void Calibration::setWetValue(uint8_t index, int value) {
    setWetRaw(index, value);
}

void Calibration::resetToDefaults(uint8_t index) {
    if (index < m_count) {
        m_configs[index].pin = index + 1;
        m_configs[index].dryRaw = 3000;
        m_configs[index].wetRaw = 1200;
        m_configs[index].enabled = true;
        writeCalibration(m_configs, m_count);
    }
}

void Calibration::removeSensor(uint8_t index) {
    if (index < m_count) {
        m_configs[index].enabled = false;
        strncpy(m_configs[index].name, "---", 31);
        m_configs[index].name[31] = '\0';
        writeCalibration(m_configs, m_count);
    }
}

void Calibration::enableSensor(uint8_t index) {
    if (index < m_count) {
        m_configs[index].enabled = true;
        saveAllConfigs(m_configs, m_count);
    }
}

uint8_t Calibration::getPin(uint8_t index) const {
    if (index < m_count) {
        return m_configs[index].pin;
    }
    return 0;
}

int Calibration::getDryValue(uint8_t index) const {
    if (index < m_count) {
        return m_configs[index].dryRaw;
    }
    return 0;
}

int Calibration::getWetValue(uint8_t index) const {
    if (index < m_count) {
        return m_configs[index].wetRaw;
    }
    return 0;
}

const char* Calibration::getName(uint8_t index) const {
    if (index < m_count) {
        return m_configs[index].name;
    }
    return "";
}

const SensorConfig* Calibration::getConfigs() const {
    return m_configs;
}
