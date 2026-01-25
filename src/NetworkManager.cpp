#include "NetworkManager.h"
#include "PID_Controller.h"
#include "Temperature.h"
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <stdio.h>

#include <ArduinoJson.h>

// Topics
const char *TOPIC_STATUS = "silvia/status";
const char *TOPIC_SET_TEMP = "silvia/setpoint/set";
const char *TOPIC_SET_SWITCH = "silvia/switch/set";
const char *TOPIC_SET_KP = "silvia/pid/kp/set";
const char *TOPIC_SET_KI = "silvia/pid/ki/set";
const char *TOPIC_SET_KD = "silvia/pid/kd/set";
const char *TOPIC_AVAILABILITY = "silvia/status/availability";
const char *TOPIC_START_AUTOTUNE = "silvia/pid/autotune/start";
const char *TOPIC_STOP_AUTOTUNE = "silvia/pid/autotune/stop";

SilviaNetworkManager::SilviaNetworkManager(Configuration &config)
    : _config(config), _mqttClient(_espClient) {
  _mqttClient.setBufferSize(1024); // Increase buffer for discovery payloads
}

void SilviaNetworkManager::begin() {
  // 1. Create Params
  // Helper buffers for Int/Float conversion
  char port_buf[6];
  snprintf(port_buf, sizeof(port_buf), "%d", _config.data().mqtt_port);

  char kp_buf[10], ki_buf[10], kd_buf[10];
  snprintf(kp_buf, sizeof(kp_buf), "%.2f", _config.data().pid_kp);
  snprintf(ki_buf, sizeof(ki_buf), "%.2f", _config.data().pid_ki);
  snprintf(kd_buf, sizeof(kd_buf), "%.2f", _config.data().pid_kd);

  _p_mqtt_server = new WiFiManagerParameter("mqtt_server", "MQTT Server",
                                            _config.data().mqtt_server, 64);
  _p_mqtt_port =
      new WiFiManagerParameter("mqtt_port", "MQTT Port", port_buf, 6);
  _p_mqtt_user = new WiFiManagerParameter("mqtt_user", "MQTT User",
                                          _config.data().mqtt_user, 32);
  _p_mqtt_pass = new WiFiManagerParameter("mqtt_pass", "MQTT Password",
                                          _config.data().mqtt_pass, 64);

  _p_pid_kp = new WiFiManagerParameter("pid_kp", "PID Kp", kp_buf, 10);
  _p_pid_ki = new WiFiManagerParameter("pid_ki", "PID Ki", ki_buf, 10);
  _p_pid_kd = new WiFiManagerParameter("pid_kd", "PID Kd", kd_buf, 10);

  // 2. Add params to WM
  _wm.addParameter(_p_mqtt_server);
  _wm.addParameter(_p_mqtt_port);
  _wm.addParameter(_p_mqtt_user);
  _wm.addParameter(_p_mqtt_pass);
  _wm.addParameter(_p_pid_kp);
  _wm.addParameter(_p_pid_ki);
  _wm.addParameter(_p_pid_kd);

  // 3. Configure WM
  _wm.setSaveParamsCallback([this]() { this->saveParamsCallback(); });
  _wm.setClass("invert");

  // 4. AutoConnect
  bool res = _wm.autoConnect("Silvia-PID-Config");

  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("WiFi Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }

  // Setup MQTT Callback
  _mqttClient.setCallback(
      [this](char *topic, uint8_t *payload, unsigned int length) {
        this->onMqttCallback(topic, payload, length);
      });
}

void SilviaNetworkManager::loop() {
  if (!_config.data().mqtt_enabled) {
    if (_mqttClient.connected()) {
      _mqttClient.disconnect();
    }
    return;
  }

  if (!_mqttClient.connected()) {
    long now = millis();
    if (now - _lastMqttReconnectAttempt > 5000) {
      _lastMqttReconnectAttempt = now;
      reconnect();
    }
  } else {
    _mqttClient.loop();
    publishState();
  }
}

void SilviaNetworkManager::publishState() {
  unsigned long now = millis();
  if (now - _lastStatePublish < 2000) // 2s interval
    return;

  _lastStatePublish = now;

  if (!_temp || !_pid)
    return;

  JsonDocument doc;
  doc["temp"] = _temp->getTemperature();
  doc["target"] = _config.getTargetTemp();
  doc["output"] = _pid->getOutput();
  doc["is_autotuning"] = _pid->isAutotuning();

  // Only show zone/limit if not tuning, or show special code
  if (_pid->isAutotuning()) {
    doc["state"] = "AUTOTUNE";
  } else {
    doc["zone"] = _pid->getZone();
    doc["limit"] = _pid->getCurrentLimit();
    if (_pid->isManualMode()) {
      if (_config.data().heater_enabled) {
        doc["state"] = "MANUAL";
      } else {
        doc["state"] = "OFF";
      }
    } else {
      doc["state"] = (_pid->getOutput() > 0) ? "heating" : "idle";
    }
  }

  doc["heater_on"] = _config.data().heater_enabled;
  doc["kp"] = _config.data().pid_kp;
  doc["ki"] = _config.data().pid_ki;
  doc["kd"] = _config.data().pid_kd;

  String output;
  serializeJson(doc, output);
  _mqttClient.publish(TOPIC_STATUS, output.c_str());
}

void SilviaNetworkManager::reconnect() {
  ConfigData &data = _config.data();
  if (strlen(data.mqtt_server) == 0)
    return;

  _mqttClient.setServer(data.mqtt_server, data.mqtt_port);

  Serial.print("Attempting MQTT connection...");
  String clientId = "SilviaESP32-";
  clientId += String(random(0xffff), HEX);

  // Connect with Last Will and Testament (LWT)
  if (_mqttClient.connect(clientId.c_str(), data.mqtt_user, data.mqtt_pass,
                          TOPIC_AVAILABILITY, 0, true, "offline")) {
    Serial.println("connected");

    // Publish availability
    _mqttClient.publish(TOPIC_AVAILABILITY, "online", true);

    // Subscribe to topics
    _mqttClient.subscribe(TOPIC_SET_TEMP);
    _mqttClient.subscribe(TOPIC_SET_SWITCH);
    _mqttClient.subscribe(TOPIC_SET_KP);
    _mqttClient.subscribe(TOPIC_SET_KI);
    _mqttClient.subscribe(TOPIC_SET_KD);

    _mqttClient.subscribe(TOPIC_SET_KD);
    _mqttClient.subscribe(
        TOPIC_START_AUTOTUNE); // Subscribe to Autotune trigger
    _mqttClient.subscribe(TOPIC_STOP_AUTOTUNE);

    sendDiscoveryConfig();
  } else {
    Serial.print("failed, rc=");
    Serial.print(_mqttClient.state());
    Serial.println(" try again in 5 seconds");
  }
}

void SilviaNetworkManager::onMqttCallback(char *topic, byte *payload,
                                          unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.printf("MQTT Rx [%s]: %s\n", topic, msg.c_str());

  if (String(topic) == TOPIC_SET_TEMP) {
    float val = msg.toFloat();
    if (val >= 20.0 && val <= 140.0) {
      _config.data().pid_setpoint = val;
      _config.save();
    }
  } else if (String(topic) == TOPIC_SET_SWITCH) {
    bool on = (msg == "ON" || msg == "true" || msg == "1");
    _config.data().heater_enabled = on;
    _config.save();
    if (on) {
      _pid->setManualMode(false);
    } else {
      _pid->setManualMode(true);
      _pid->setManualPower(0);
    }
  } else if (String(topic) == TOPIC_SET_KP) {
    _config.data().pid_kp = msg.toFloat();
    _config.save();
    _pid->setTunings(_config.data().pid_kp, _config.data().pid_ki,
                     _config.data().pid_kd);
  } else if (String(topic) == TOPIC_SET_KI) {
    _config.data().pid_ki = msg.toFloat();
    _config.save();
    _pid->setTunings(_config.data().pid_kp, _config.data().pid_ki,
                     _config.data().pid_kd);
  } else if (String(topic) == TOPIC_SET_KD) {
    _config.data().pid_kd = msg.toFloat();
    _config.save();
    _pid->setTunings(_config.data().pid_kp, _config.data().pid_ki,
                     _config.data().pid_kd);
  } else if (String(topic) == TOPIC_START_AUTOTUNE) {
    Serial.println("MQTT: Starting Autotune");
    _pid->startAutotune();
  } else if (String(topic) == TOPIC_STOP_AUTOTUNE) {
    Serial.println("MQTT: Stopping Autotune");
    _pid->stopAutotune();
  }

  // Force immediate update
  publishState();
}

void SilviaNetworkManager::sendDiscoveryConfig() {
  String device =
      "\"device\":{\"identifiers\":[\"silvia_pid\"],\"name\":\"Silvia "
      "PID\",\"manufacturer\":\"Rancilio\",\"model\":\"Silvia "
      "V3\",\"sw_version\":\"1.0.0\"}";

  String availability = "\"availability_topic\": \"" +
                        String(TOPIC_AVAILABILITY) +
                        "\", "
                        "\"payload_available\": \"online\", "
                        "\"payload_not_available\": \"offline\", ";

  // 1. Climate Entity (Target + Current)
  // Using MQTT Climate integration
  String climatePayload =
      "{\"name\": \"Silvia Thermotstat\", \"unique_id\": \"silvia_climate\", "
      "\"action_topic\": \"silvia/status\", \"action_template\": \"{{ "
      "value_json.state }}\", "
      "\"current_temperature_topic\": \"silvia/status\", "
      "\"current_temperature_template\": \"{{ value_json.temp }}\", "
      "\"temperature_command_topic\": \"silvia/setpoint/set\", "
      "\"temperature_state_topic\": \"silvia/status\", "
      "\"temperature_state_template\": \"{{ value_json.target }}\", "
      "\"mode_command_topic\": \"silvia/switch/set\", "
      "\"mode_state_topic\": \"silvia/status\", \"mode_state_template\": \"{% "
      "if value_json.heater_on %}heat{% else %}off{% endif %}\", "
      "\"modes\": [\"off\", \"heat\"], "
      "\"min_temp\": 20, \"max_temp\": 110, \"precision\": 0.1, \"temp_step\": "
      "0.1, " +
      availability + device + "}";
  _mqttClient.publish("homeassistant/climate/silvia/config",
                      climatePayload.c_str(), true);

  // 2. Heater Output (Sensor)
  String outPayload =
      "{\"name\": \"Silvia Heater Output\", \"unique_id\": \"silvia_output\", "
      "\"state_topic\": \"silvia/status\", \"value_template\": \"{{ "
      "value_json.output }}\", "
      "\"unit_of_measurement\": \"%\", \"icon\": \"mdi:radiator\", " +
      availability + device + "}";
  _mqttClient.publish("homeassistant/sensor/silvia/output/config",
                      outPayload.c_str(), true);

  // 3. PID Kp (Number)
  String kpPayload =
      "{\"name\": \"Silvia PID Kp\", \"unique_id\": \"silvia_kp\", "
      "\"command_topic\": \"silvia/pid/kp/set\", "
      "\"state_topic\": \"silvia/status\", \"value_template\": \"{{ "
      "value_json.kp }}\", "
      "\"min\": 0, \"max\": 200, \"step\": 0.1, \"icon\": "
      "\"mdi:chart-bell-curve\", " +
      availability + device + "}";
  _mqttClient.publish("homeassistant/number/silvia/kp/config",
                      kpPayload.c_str(), true);
}

void SilviaNetworkManager::saveParamsCallback() {
  Serial.println("Saving custom parameters...");
  ConfigData &data = _config.data();
  strlcpy(data.mqtt_server, _p_mqtt_server->getValue(),
          sizeof(data.mqtt_server));
  data.mqtt_port = atoi(_p_mqtt_port->getValue());
  strlcpy(data.mqtt_user, _p_mqtt_user->getValue(), sizeof(data.mqtt_user));
  strlcpy(data.mqtt_pass, _p_mqtt_pass->getValue(), sizeof(data.mqtt_pass));
  data.pid_kp = atof(_p_pid_kp->getValue());
  data.pid_ki = atof(_p_pid_ki->getValue());
  data.pid_kd = atof(_p_pid_kd->getValue());
  _config.save();
}

void SilviaNetworkManager::resetSettings() { _wm.resetSettings(); }
