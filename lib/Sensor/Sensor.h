#ifndef SENSOR_H
#define SENSOR_H

class Sensor {
public:
  Sensor(int pin);
  void begin();
  int readRaw();
  int readPercent();
  float readVoltage();
  void setCalibration(int dry, int wet);

private:
  int m_pin;
  int m_dryValue;
  int m_wetValue;
  static const int SAMPLES = 10;
};

#endif // SENSOR_H
