#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "config.h"
#include <Arduino.h>
#include <Preferences.h>

struct ConfigData {
  char mqtt_server[64];
  int mqtt_port;
  char mqtt_user[32];
  char mqtt_pass[64]; // Increased size for security tokens
  float pid_kp;
  float pid_ki;
  float pid_kd;
  float pid_setpoint;
  bool mqtt_enabled; // Conditional MQTT
};

class Configuration {
public:
  Configuration();
  void begin();

  // Load config from preferences
  void load();

  // Save config to preferences
  void save();

  // Accessor
  ConfigData &data() { return _data; }
  float getTargetTemp() const { return _data.pid_setpoint; }

private:
  Preferences _prefs;
  ConfigData _data;
  bool _initialized;

  // Defaults
  void setDefaults();
};

#endif
