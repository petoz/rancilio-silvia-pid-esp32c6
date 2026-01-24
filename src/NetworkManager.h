#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "Configuration.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

// Topics
extern const char *TOPIC_STATUS;
extern const char *TOPIC_SET_TEMP;
extern const char *TOPIC_SET_SWITCH;
extern const char *TOPIC_SET_KP;
extern const char *TOPIC_SET_KI;
extern const char *TOPIC_SET_KD;
extern const char *TOPIC_AVAILABILITY;

class Temperature;
class PID_Controller;

class SilviaNetworkManager {
public:
  SilviaNetworkManager(Configuration &config);
  void begin();
  void loop();
  void sendDiscoveryConfig();
  void setModules(Temperature &temp, PID_Controller &pid) {
    _temp = &temp;
    _pid = &pid;
  }
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

  WiFiClient _espClient;
  PubSubClient _mqttClient;
  unsigned long _lastMqttReconnectAttempt = 0;

  void saveParamsCallback();
  void reconnect();
  void onMqttCallback(char *topic, byte *payload, unsigned int length);
  void publishState();

  Temperature *_temp = nullptr;
  PID_Controller *_pid = nullptr;

  unsigned long _lastStatePublish = 0;
};

#endif
