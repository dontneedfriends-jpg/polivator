#include "Sensor.h"
#include <Arduino.h>

Sensor::Sensor(int pin) : m_pin(pin), m_dryValue(3000), m_wetValue(1200) {
}

void Sensor::begin() {
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

int Sensor::readRaw() {
  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(m_pin);
  }
  return sum / SAMPLES;
}

int Sensor::readPercent() {
  int raw = readRaw();
  if (raw <= m_wetValue) return 100;
  if (raw >= m_dryValue) return 0;
  return (m_dryValue - raw) * 100 / (m_dryValue - m_wetValue);
}

float Sensor::readVoltage() {
  int raw = readRaw();
  return raw * (3.3f / 4095.0f);
}

void Sensor::setCalibration(int dry, int wet) {
  m_dryValue = dry;
  m_wetValue = wet;
}
