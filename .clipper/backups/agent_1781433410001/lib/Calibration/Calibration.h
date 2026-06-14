#ifndef CALIBRATION_H
#define CALIBRATION_H

class Calibration {
public:
  Calibration();
  ~Calibration();
  void load();
  void save();
  float apply(float rawValue);

private:
  // Calibration parameters
};

#endif // CALIBRATION_H
