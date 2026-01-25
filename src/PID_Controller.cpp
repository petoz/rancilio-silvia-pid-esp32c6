#include "PID_Controller.h"
#include <PID_v1.h>

PID_Controller::PID_Controller()
    : _input(0), _output(0), _setpoint(0),
      _myPID(&_input, &_output, &_setpoint, _Kp, _Ki, _Kd, DIRECT) {
  _autotune = new PID_ATune(&_input, &_output, &_setpoint);
}

void PID_Controller::startAutotune() {
  _isAutotuning = true;
  _autotune->SetNoiseBand(0.5);
  _autotune->SetOutputStep(100); // Toggle between 0 and 100 for Relay
  _autotune->SetLookbackSec(10); // 10s lookback
  _manualMode = true;            // Take over control
  _myPID.SetMode(MANUAL);
}

void PID_Controller::stopAutotune() {
  _isAutotuning = false;
  _autotune->Cancel();
  _manualMode = false;
  _myPID.SetMode(AUTOMATIC);

  // Apply new tunings? Or let user do it manually?
  // ESPressIoT style: usually just prints them or saves them.
  // We will expose them via getter or log them.
  Serial.print("Autotune Finished. Kp: ");
  Serial.print(_autotune->GetKp());
  Serial.print(" Ki: ");
  Serial.print(_autotune->GetKi());
  Serial.print(" Kd: ");
  Serial.println(_autotune->GetKd());

  setTunings(_autotune->GetKp(), _autotune->GetKi(), _autotune->GetKd());
}
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
  _input = input;
  _setpoint = setpoint;

  if (_isAutotuning) {
    int val = _autotune->Runtime(); // Returns 1 when done
    if (val != 0) {
      stopAutotune();
    }
    return (float)_output;
  }

  if (_manualMode) {
    return _manualPower;
  }

  if (!_espressoLogicEnabled) {
    // Standard behavior
    _myPID.SetTunings(_Kp, _Ki, _Kd);
    _myPID.Compute();
    return (float)_output;
  }

  // Adaptive PID Logic (ESPressIoT)
  double error = abs(_setpoint - _input);

  if (error > _adaptiveThreshold) {
    // Aggressive Zone
    _lastZone = 1;
    _myPID.SetTunings(_aggKp, _aggKi, _aggKd);
  } else {
    // Normal Zone
    _lastZone = 2;
    _myPID.SetTunings(_Kp, _Ki, _Kd);
  }

  // Ensure Auto
  if (_myPID.GetMode() != AUTOMATIC)
    _myPID.SetMode(AUTOMATIC);

  _myPID.SetOutputLimits(0, 100);
  _myPID.Compute();

  _lastLimit = 100.0;
  _output = _output; // PID lib updates _output pointer
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
