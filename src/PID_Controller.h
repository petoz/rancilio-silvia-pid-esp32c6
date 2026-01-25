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
  // ECM Style Tunings (Milder P, Stronger I for stability, Good D)
  double _Kp = 40.0;
  double _Ki = 2.0;
  double _Kd = 80.0;

  bool _manualMode = false;
  float _manualPower = 0.0; // 0-100

  // Espresso Logic
  bool _espressoLogicEnabled = true;

  // Zones
  double _warmupDelta = 20.0;  // > 20C: Zone 1 (Max Power)
  double _rampMidDelta = 5.0;  // 20C-5C: Zone 2A (Ramp 1)
  double _approachDelta = 1.0; // 5C-1C: Zone 2B (Ramp 2). <1C: Zone 3 (Stable)

  // Power Levels
  double _maxPower = 100.0;
  double _midPower = 45.0;
  double _minPower = 30.0;

  // Rate Limiter
  double _maxSlewRate =
      10.0;                 // Max % change per call (assuming ~1s sample time)
  double _lastOutput = 0.0; // For rate limiting

  PID _myPID;
};

#endif
