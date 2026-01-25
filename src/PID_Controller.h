#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include "PID_AutoTune_v0.h"
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

  // Diagnostics
  int getZone() const { return _lastZone; }
  double getCurrentLimit() const { return _lastLimit; }
  bool isAutotuning() const { return _isAutotuning; }

  // Autotune Control
  void startAutotune();
  void stopAutotune();

private:
  double _input;
  double _output;
  double _setpoint;

  // ESPressIoT Defaults (Silvia V3)
  // Normal PID (Error < 1.5C)
  double _Kp = 91.0;
  double _Ki = 0.26;
  double _Kd = 7950.0;

  // Aggressive PID (Error > 1.5C)
  // Max power
  double _aggKp = 100.0;
  double _aggKi = 0.0;
  double _aggKd = 0.0;

  double _adaptiveThreshold = 1.5;

  bool _manualMode = false;
  float _manualPower = 0.0; // 0-100

  // Espresso Logic
  bool _espressoLogicEnabled = true; // Use ESPressIoT Logic if true

  // Zones - REVERTED to stricter values
  double _warmupDelta = 20.0;  // > 20C: Zone 1 (Max Power)
  double _rampMidDelta = 5.0;  // 20C-5C: Zone 2A (Ramp 1)
  double _approachDelta = 1.0; // 5C-1C: Zone 2B (Ramp 2). <1C: Zone 3 (Stable)

  // Power Levels
  double _maxPower = 100.0;
  double _midPower = 20.0; // Drastically reduced from 45.0
  double _minPower = 0.0;  // Allowed to drop to 0.0

  // Rate Limiter
  double _maxSlewRate =
      10.0;                 // Max % change per call (assuming ~1s sample time)
  double _lastOutput = 0.0; // For rate limiting

  // State
  int _lastZone = 0;
  double _lastLimit = 100.0;

  // Autotune
  PID_ATune *_autotune = nullptr;
  bool _isAutotuning = false;

  PID _myPID;
};

#endif
