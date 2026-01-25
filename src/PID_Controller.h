#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>
#include <PID_v1.h>

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

  // Espresso Logic Configuration
  void setEspressoLogic(bool enable);

private:
  double _input;
  double _output;
  double _setpoint;

  // Defaults
  // Aggressive tuning for Espresso Logic (High Kp/Kd because we clamp output)
  double _Kp = 60.0;
  double _Ki = 0.5;
  double _Kd = 120.0;

  bool _manualMode = false;
  float _manualPower = 0.0; // 0-100

  // Espresso Logic
  bool _espressoLogicEnabled = true;
  double _warmupDelta = 10.0;  // Above this delta: Max Power, No Integral
  double _approachDelta = 0.5; // Below this delta: Stable PID
                               // Between 10.0 and 0.5: Ramp Limit

  PID _myPID;
};

#endif
