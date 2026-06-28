#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "../Common/Types.h"
#include "../Common/IAnalogReader.h"
#include "../Settings/Settings.h"

class Calibration; // forward declaration

class SensorManager {
public:
    SensorManager();
    bool begin(Calibration* calibration, Settings* settings = nullptr);

    /// Wire an optional IAnalogReader (e.g. ADS1115).
    /// When set, readAll() uses it instead of the built-in ADC.
    /// Also updates the voltage scale factor from the reader.
    void setReader(IAnalogReader* reader);

    void readAll();
    int getRaw(uint8_t index);
    int getPercent(uint8_t index);
    float getVoltage(uint8_t index);
    uint8_t getCount();
    uint8_t getEnabledCount();
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
    IAnalogReader* m_reader;
    int m_samples = 10;
    float m_voltScale;   // V_per_LSB from the reader or built-in ADC
};

#endif // SENSOR_MANAGER_H
