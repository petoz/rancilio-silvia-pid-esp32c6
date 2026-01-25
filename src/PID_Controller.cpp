#include "PID_Controller.h"
#include <PID_v1.h>

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

  double outputTarget = 0.0;

  // ECM Logic
  double error = _setpoint - _input;

  if (error > _warmupDelta) {
    // Zone 1: Warmup (> 20C)
    _lastZone = 1;
    _lastLimit = 100.0;

    // Max power, no integral
    if (_myPID.GetMode() != MANUAL)
      _myPID.SetMode(MANUAL);
    outputTarget = _maxPower;
  } else if (error > _approachDelta) {
    // Zone 2: Approach (20C - 1C)
    _lastZone = 2;

    // Calculate Ramp Limit
    double maxLimit = 100.0;

    if (error > _rampMidDelta) {
      // Zone 2A: 20C -> 5C. Ramp 100% -> 45%
      double ratio = (error - _rampMidDelta) / (_warmupDelta - _rampMidDelta);
      if (ratio < 0)
        ratio = 0;
      if (ratio > 1)
        ratio = 1;
      maxLimit = _midPower + (ratio * (_maxPower - _midPower));
    } else {
      // Zone 2B: 5C -> 1C. Ramp 45% -> 30%
      double ratio =
          (error - _approachDelta) / (_rampMidDelta - _approachDelta);
      if (ratio < 0)
        ratio = 0;
      if (ratio > 1)
        ratio = 1;
      maxLimit = _minPower + (ratio * (_midPower - _minPower));
    }
    _lastLimit = maxLimit;

    // Transition Manual -> Auto cleanup
    if (_myPID.GetMode() != AUTOMATIC) {
      _output = 0;
      _myPID.SetMode(AUTOMATIC);
    }

    // We are in approach, so we kill Integral to prevent windup
    // But we use PID to calculate P-term mainly
    _myPID.SetTunings(_Kp, 0.0, _Kd);
    _myPID.SetOutputLimits(0, maxLimit);
    _myPID.Compute();

    // If PID output is too low in approach, force at least minPower
    // This prevents temperature from "stalling" or falling back
    if (_output < _minPower) {
      outputTarget = _minPower;
    } else {
      outputTarget = _output;
    }

    // Note: _output is updated by Compute(), but we might override it into
    // outputTarget
  } else {
    // Zone 3: Stable (< 1C)
    // Full PID

    // Check for Transition into Zone 3
    if (_lastZone != 3) {
      // Transition Clean-up
      // Ensure we start fresh in Auto mode
      if (_myPID.GetMode() != AUTOMATIC)
        _myPID.SetMode(AUTOMATIC);
    }

    _lastZone = 3;
    _lastLimit = 100.0;

    if (_myPID.GetMode() != AUTOMATIC)
      _myPID.SetMode(AUTOMATIC);

    _myPID.SetTunings(_Kp, _Ki, _Kd); // Restore I
    _myPID.SetOutputLimits(0, 100);
    _myPID.Compute();
    outputTarget = _output;
  }

  // Rate Limiter / Slew Rate Logic
  double delta = outputTarget - _lastOutput;
  if (delta > _maxSlewRate) {
    outputTarget = _lastOutput + _maxSlewRate;
  } else if (delta < -_maxSlewRate) {
    outputTarget = _lastOutput - _maxSlewRate;
  }

  // Final clamp
  if (outputTarget < 0)
    outputTarget = 0;
  if (outputTarget > 100)
    outputTarget = 100;

  _lastOutput = outputTarget;
  _output = outputTarget;

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
