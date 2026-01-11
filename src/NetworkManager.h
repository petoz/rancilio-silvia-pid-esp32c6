#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "Configuration.h"
#include <Arduino.h>
#include <WiFiManager.h>

class SilviaNetworkManager {
public:
  SilviaNetworkManager(Configuration &config);
  void begin();
  void resetSettings(); // Helper to clear wifi settings

private:
  Configuration &_config;
  WiFiManager _wm;

  // Custom parameters
  WiFiManagerParameter *_p_mqtt_server;
  WiFiManagerParameter *_p_mqtt_port;
  WiFiManagerParameter *_p_mqtt_user;
  WiFiManagerParameter *_p_mqtt_pass;

  // PID Params could be set here too, though usually better via MQTT
  // Valid to expose them for initial setup
  WiFiManagerParameter *_p_pid_kp;
  WiFiManagerParameter *_p_pid_ki;
  WiFiManagerParameter *_p_pid_kd;

  void saveParamsCallback();
};

#endif
