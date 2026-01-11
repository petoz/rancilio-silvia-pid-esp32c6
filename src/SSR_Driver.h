#ifndef SSR_DRIVER_H
#define SSR_DRIVER_H

#include <Arduino.h>

class SSR_Driver {
public:
  SSR_Driver(uint8_t pin);
  void begin();
  // Set power in percentage (0.0 - 100.0)
  void setPower(double dutyCycle);
  void loop();

private:
  uint8_t _pin;
  double _dutyCycle; // 0-100
  unsigned long _windowStartTime;
  unsigned long _windowSize; // Time window in ms (e.g. 1000ms or 2000ms)
  bool _isOn;
};

#endif
