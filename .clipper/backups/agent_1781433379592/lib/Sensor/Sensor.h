#ifndef SENSOR_H
#define SENSOR_H

class Sensor {
public:
  Sensor();
  ~Sensor();
  void begin();
  float readMoisture();
  float readTemperature();
  float readHumidity();

private:
  // Sensor-specific members
};

#endif // SENSOR_H
