#include "SSR_Driver.h"

SSR_Driver::SSR_Driver(uint8_t pin)
    : _pin(pin), _dutyCycle(0.0), _windowSize(1000), _isOn(false) {}

void SSR_Driver::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  _windowStartTime = millis();
}

void SSR_Driver::setPower(double dutyCycle) {
  // Clamp to 0-100
  if (dutyCycle < 0.0)
    dutyCycle = 0.0;
  if (dutyCycle > 100.0)
    dutyCycle = 100.0;
  _dutyCycle = dutyCycle;
}

void SSR_Driver::loop() {
  unsigned long now = millis();

  if (now - _windowStartTime > _windowSize) {
    _windowStartTime += _windowSize;
  }

  // Time within window
  unsigned long timeInWindow = now - _windowStartTime;

  // Calculate ON time based on duty cycle
  unsigned long onTime = (_dutyCycle / 100.0) * _windowSize;

  if (onTime > timeInWindow) {
    if (!_isOn) {
      digitalWrite(_pin, HIGH);
      _isOn = true;
    }
  } else {
    if (_isOn) {
      digitalWrite(_pin, LOW);
      _isOn = false;
    }
  }
}
