#include "Ads1115Reader.h"

// ─── Register map ─────────────────────────────────────────────────────────
#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01

// ─── Config register helpers ──────────────────────────────────────────────

// Bit masks
#define ADS_CFG_OS_MASK         (1 << 15)    // Operational status / start
#define ADS_CFG_MUX_MASK        0x7000       // Input multiplexer
#define ADS_CFG_PGA_MASK        0x0E00       // Gain
#define ADS_CFG_MODE_MASK       0x0100       // Mode (0=continuous, 1=single-shot)
#define ADS_CFG_DR_MASK         0x00E0       // Data rate
#define ADS_CFG_COMP_MODE_MASK  0x0010       // Comparator mode
#define ADS_CFG_COMP_POL_MASK   0x0008       // Comparator polarity
#define ADS_CFG_COMP_LAT_MASK   0x0004       // Latching comparator
#define ADS_CFG_COMP_QUE_MASK   0x0003       // Comparator queue / disable

// MUX values for single-ended channels
static const uint16_t mux_single[] = {
    0x4000,  // AIN0
    0x5000,  // AIN1
    0x6000,  // AIN2
    0x7000,  // AIN3
};

// PGA gain → voltage scale and config bits
// index: gain arg (0-5)
static const uint16_t pga_bits[] = {
    0x0000,  // ±6.144 V
    0x0200,  // ±4.096 V
    0x0400,  // ±2.048 V (default)
    0x0600,  // ±1.024 V
    0x0800,  // ±0.512 V
    0x0A00,  // ±0.256 V
};

// Data rate → config bits
static const uint16_t dr_bits[] = {
    0x0000,  // 8 SPS
    0x0020,  // 16
    0x0040,  // 32
    0x0060,  // 64
    0x0080,  // 128 (default)
    0x00A0,  // 250
    0x00C0,  // 475
    0x00E0,  // 860
};

// ─── Constructor / Setup ──────────────────────────────────────────────────

Ads1115Reader::Ads1115Reader(uint8_t address, TwoWire& wire)
    : m_wire(wire),
      m_address(address),
      m_gain(1),          // ±4.096 V
      m_sps(4) {}         // 128 SPS

bool Ads1115Reader::begin() {
    m_wire.begin();
    // Probe the chip by reading the config register
    uint16_t cfg;
    if (!readRegister(ADS1115_REG_CONFIG, cfg)) {
        return false;
    }
    return true;
}

void Ads1115Reader::setAddress(uint8_t address) { m_address = address; }
void Ads1115Reader::setGain(uint8_t gain) {
    if (gain > 5) gain = 5;
    m_gain = gain;
}
void Ads1115Reader::setSps(uint8_t sps) {
    if (sps > 7) sps = 7;
    m_sps = sps;
}

// ─── I2C low-level ────────────────────────────────────────────────────────

bool Ads1115Reader::readRegister(uint8_t reg, uint16_t& value) {
    m_wire.beginTransmission(m_address);
    m_wire.write(reg);
    if (m_wire.endTransmission(false) != 0) return false;

    if (m_wire.requestFrom(m_address, (uint8_t)2) < 2) return false;
    value = ((uint16_t)m_wire.read() << 8) | m_wire.read();
    return true;
}

bool Ads1115Reader::writeRegister(uint8_t reg, uint16_t value) {
    m_wire.beginTransmission(m_address);
    m_wire.write(reg);
    m_wire.write((uint8_t)(value >> 8));
    m_wire.write((uint8_t)(value & 0xFF));
    return m_wire.endTransmission(true) == 0;
}

// ─── Config builder ───────────────────────────────────────────────────────

uint16_t Ads1115Reader::makeConfig(uint8_t channel, bool singleShot) {
    uint16_t cfg = 0;
    cfg |= ADS_CFG_OS_MASK;                   // Start conversion (when single-shot)
    if (channel < 4) cfg |= mux_single[channel];
    cfg |= pga_bits[m_gain];                  // Gain
    if (singleShot) cfg |= ADS_CFG_MODE_MASK; // Single-shot mode
    cfg |= dr_bits[m_sps];                    // Data rate
    // Comparator disabled
    cfg |= 0x0003; // Disable comparator (bits 0-1 = 11)
    return cfg;
}

// ─── Read ─────────────────────────────────────────────────────────────────

int Ads1115Reader::readRaw(uint8_t channel) {
    if (channel > 3) channel = 3;

    // Start single-shot conversion on the requested channel
    uint16_t cfg = makeConfig(channel, true);
    if (!writeRegister(ADS1115_REG_CONFIG, cfg)) {
        return -1;
    }

    // Wait for conversion to complete (at 128 SPS max ~8 ms)
    // Poll the OS bit (bit 15) which is 0 while converting, 1 when done
    // Fallback: fixed delay to avoid infinite loop on i2c error
    for (int tries = 0; tries < 50; tries++) {
        delay(1);
        uint16_t status;
        if (!readRegister(ADS1115_REG_CONFIG, status)) {
            // I2C error — fall through to a fixed wait
            delay(5);
            break;
        }
        if (status & ADS_CFG_OS_MASK) break; // Conversion complete
    }

    // Read the conversion result
    uint16_t raw;
    if (!readRegister(ADS1115_REG_CONVERSION, raw)) {
        return -1;
    }

    // ADS1115 returns signed 16-bit. For single-ended, 0 = GND, 32767 = Vref.
    // If raw > 32767 it means the input exceeded the range; cap at 32767.
    if (raw > 32767) raw = 32767;
    return (int)raw;
}

float Ads1115Reader::voltageScale() const {
    // ±4.096V PGA, single-ended → 0 to 4.096V maps to 0-32767
    return 4.096f / 32767.0f;
}
