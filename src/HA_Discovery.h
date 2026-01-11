#ifndef HA_DISCOVERY_H
#define HA_DISCOVERY_H

#include <Arduino.h>

class HA_Discovery {
public:
  void begin();
  void loop();
  void publishDiscovery();
};

#endif
