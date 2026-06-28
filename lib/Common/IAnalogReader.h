#ifndef IANALOG_READER_H
#define IANALOG_READER_H

#include <Arduino.h>

/// Abstract interface for ADC reading (built-in ESP32 ADC or external ADC like ADS1115).
class IAnalogReader {
public:
    virtual ~IAnalogReader() = default;

    /// Read raw ADC value. `pin` semantics depend on implementation:
    /// - Built-in ADC: GPIO number
    /// - ADS1115: channel index (0-3)
    virtual int readRaw(uint8_t pin) = 0;

    /// Scale factor to convert raw → voltage (V_per_LSB).
    /// Default: ESP32 built-in 12-bit ADC with 3.3 V reference.
    virtual float voltageScale() const { return 3.3f / 4095.0f; }
};

#endif // IANALOG_READER_H
