#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>
#include <QuickPID.h>

class PID_Controller {
public:
  PID_Controller();
  void begin();

  // Run this in main loop. Returns output 0-100.
  // We pass current temp and target temp here.
  float compute(float input, float setpoint);

  void setTunings(float Kp, float Ki, float Kd);

  // Manual override control
  void setManualMode(bool manual);
  bool isManualMode() const { return _manualMode; }
  void setManualPower(float power); // 0-100
  float getManualPower() const { return _manualPower; }

  float getOutput() const { return _output; }

private:
  float _input;
  float _output;
  float _setpoint;

  // Defaults
  float _Kp = 2.0;
  float _Ki = 0.5;
  float _Kd = 2.0;

  bool _manualMode = false;
  float _manualPower = 0.0; // 0-100

  QuickPID _myPID;
};

#endif
