#include "PID_Controller.h"

PID_Controller::PID_Controller()
    : _input(0), _output(0), _setpoint(0),
      _myPID(&_input, &_output, &_setpoint, _Kp, _Ki, _Kd, DIRECT) {}
void PID_Controller::setEspressoLogic(bool enable) {
  _espressoLogicEnabled = enable;
}

void PID_Controller::begin() {
  // Apply default tunings or load from config (TODO: Load from config)
  _myPID.SetTunings(_Kp, _Ki, _Kd);
  _myPID.SetMode(AUTOMATIC);
  _myPID.SetOutputLimits(0, 100);
  _myPID.SetSampleTime(1000); // 1000ms
}

void PID_Controller::setTunings(float Kp, float Ki, float Kd) {
  _Kp = Kp;
  _Ki = Ki;
  _Kd = Kd;
  _myPID.SetTunings(_Kp, _Ki, _Kd);
}

float PID_Controller::compute(float input, float setpoint) {
  if (_manualMode) {
    return _manualPower;
  }

  _input = input;
  _setpoint = setpoint;

  if (!_espressoLogicEnabled) {
    // Standard behavior
    if (_myPID.GetMode() != AUTOMATIC)
      _myPID.SetMode(AUTOMATIC);
    _myPID.SetOutputLimits(0, 100);
    _myPID.SetTunings(_Kp, _Ki, _Kd);
    _myPID.Compute();
    return (float)_output;
  }

  // Espresso Logic Implementation
  double error = _setpoint - _input;

  if (error > _warmupDelta) {
    // Zone 1: Warmup (Far from setpoint)
    // Strategy: Max Power, Disable Integral (Anti-windup)
    if (_myPID.GetMode() != MANUAL)
      _myPID.SetMode(MANUAL);
    _output = 100.0;
  } else if (error > _approachDelta) {
    // Zone 2: Approach (Ramping down)
    // Strategy: Linear Limit Ramp + PD Control (No Integral)

    // Calculate Ramp Limit (20% to 100%)
    double rampRatio =
        (error - _approachDelta) / (_warmupDelta - _approachDelta);
    if (rampRatio < 0)
      rampRatio = 0;
    if (rampRatio > 1)
      rampRatio = 1;

    double maxPower = 20.0 + (rampRatio * 80.0);

    // Ensure we are in AUTOMATIC for PID calculation
    if (_myPID.GetMode() != AUTOMATIC) {
      // Transition Manual -> Auto:
      // Reset output to 0 internally so ITerm initializes to 0.
      // This is CRITICAL for Anti-Windup when coming from 100% manual power.
      _output = 0;
      _myPID.SetMode(AUTOMATIC);
    }

    // Disable Integral during approach to prevent overshoot
    _myPID.SetTunings(_Kp, 0.0, _Kd);
    _myPID.SetOutputLimits(0, maxPower);
    _myPID.Compute();
  } else {
    // Zone 3: Stable (Close to setpoint)
    // Strategy: Full PID with Integral

    if (_myPID.GetMode() != AUTOMATIC)
      _myPID.SetMode(AUTOMATIC);

    _myPID.SetTunings(_Kp, _Ki, _Kd); // Restore I-term
    _myPID.SetOutputLimits(0, 100);
    _myPID.Compute();
  }

  return (float)_output;
}

void PID_Controller::setManualMode(bool manual) {
  _manualMode = manual;

  if (_manualMode) {
    _myPID.SetMode(MANUAL);
    _output = _manualPower; // Initialize output to manual power
  } else {
    _myPID.SetMode(AUTOMATIC);
  }
}

void PID_Controller::setManualPower(float power) {
  if (power < 0)
    power = 0;
  if (power > 100)
    power = 100;
  _manualPower = power;
  if (_manualMode) {
    _output = _manualPower;
  }
}
