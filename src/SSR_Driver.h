#ifndef SSR_DRIVER_H
#define SSR_DRIVER_H

#include <Arduino.h>

class SSR_Driver {
public:
  void begin();
  void setPower(float dutyCycle); // 0.0 - 100.0 or 0.0 - 1.0
  void loop(); // Call in main loop for time-proportioned control
};

#endif
