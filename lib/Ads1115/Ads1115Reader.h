#ifndef ADS1115_READER_H
#define ADS1115_READER_H

#include "IAnalogReader.h"
#include <Wire.h>

/// IAnalogReader implementation for ADS1115 external ADC (I2C).
/// Maps pin 0-3 to AIN0-AIN3 in single-ended mode.
/// Defaults to ±4.096V range (1 mV per LSB), 128 SPS, single-shot.
class Ads1115Reader : public IAnalogReader {
public:
    /// @param address  I2C address (default 0x48, ADDR = GND)
    /// @param wire     Wire instance to use
    Ads1115Reader(uint8_t address = 0x48, TwoWire& wire = Wire);

    /// Initialise I2C and verify the chip is present.
    bool begin();

    /// Read one channel (0-3) in single-ended mode.
    /// Returns raw 16-bit ADC value (0-32767 for single-ended).
    int readRaw(uint8_t channel) override;

    /// Scale: ±4.096V / 32767 (1 mV per LSB in single-ended mode).
    float voltageScale() const override;

    /// Set the I2C address. Call before begin().
    void setAddress(uint8_t address);

    /// Set PGA gain. Call before begin().
    /// gain: 0=6.144V, 1=4.096V, 2=2.048V, 3=1.024V, 4=0.512V, 5=0.256V
    void setGain(uint8_t gain);

    /// Set samples per second. Call before begin().
    /// sps: 0=8, 1=16, 2=32, 3=64, 4=128, 5=250, 6=475, 7=860
    void setSps(uint8_t sps);

private:
    TwoWire& m_wire;
    uint8_t m_address;
    uint8_t m_gain;       // 0-5
    uint8_t m_sps;        // 0-7

    bool readRegister(uint8_t reg, uint16_t& value);
    bool writeRegister(uint8_t reg, uint16_t value);
    uint16_t makeConfig(uint8_t channel, bool singleShot);
};

#endif // ADS1115_READER_H
