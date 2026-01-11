#include "PID_Controller.h"

PID_Controller::PID_Controller()
    : _input(0), _output(0), _setpoint(0),
      _myPID(&_input, &_output, &_setpoint) {}

void PID_Controller::begin() {
  // Apply default tunings or load from config (TODO: Load from config)
  _myPID.SetTunings(_Kp, _Ki, _Kd);
  _myPID.SetMode(QuickPID::Control::automatic);
  _myPID.SetOutputLimits(0, 100);
  _myPID.SetSampleTimeUs(100000); // 100ms
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

  // Compute PID
  _myPID.Compute();

  // _output is updated by Compute()
  return _output;
}

void PID_Controller::setManualMode(bool manual) {
  _manualMode = manual;
  // If we stay in automatic mode but just ignore output, integral builds up.
  // Better to switch QuickPID mode?
  if (_manualMode) {
    _myPID.SetMode(QuickPID::Control::manual);
    _output = _manualPower; // Initialize output to manual power
  } else {
    _myPID.SetMode(QuickPID::Control::automatic);
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
