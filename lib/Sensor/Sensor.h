#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "../Common/Types.h"
#include "../Settings/Settings.h"

class Calibration; // forward declaration

class SensorManager {
public:
    SensorManager();
    bool begin(Calibration* calibration, Settings* settings = nullptr);
    void readAll();
    int getRaw(uint8_t index);
    int getPercent(uint8_t index);
    float getVoltage(uint8_t index);
    uint8_t getCount();
    void setCalibration(uint8_t index, uint16_t dry, uint16_t wet);

private:
    struct SensorState {
        int pin;
        uint16_t dry;
        uint16_t wet;
        int raw;
        int percent;
        float voltage;
    };
    SensorState sensors[MAX_SENSORS];
    Calibration* cal;
    int m_samples = 10;
};

#endif // SENSOR_MANAGER_H
