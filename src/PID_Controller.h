#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

class PID_Controller {
public:
  void begin();
  double compute(double input, double setpoint);
  void setTunings(double Kp, double Ki, double Kd);
};

#endif
