#include "NetworkManager.h"

SilviaNetworkManager::SilviaNetworkManager(Configuration &config)
    : _config(config), _mqttClient(_espClient) {}

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
  // Lambda to callback class member
  _wm.setSaveParamsCallback([this]() { this->saveParamsCallback(); });

  // Dark mode nice-to-have
  _wm.setClass("invert");

  // 4. AutoConnect
  // Access Point Name: Silvia-PID-Config
  // No password for config AP for simplicity (or add one if desired)
  bool res = _wm.autoConnect("Silvia-PID-Config");

  if (!res) {
    Serial.println("Failed to connect");
    // ESP.restart(); // Loop will try again or we can continue offline
  } else {
    Serial.println("WiFi Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
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
  }
}

void SilviaNetworkManager::reconnect() {
  // Basic reconnect logic
  ConfigData &data = _config.data();
  if (strlen(data.mqtt_server) == 0)
    return;

  _mqttClient.setServer(data.mqtt_server, data.mqtt_port);

  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "SilviaESP32-";
  clientId += String(random(0xffff), HEX);

  if (_mqttClient.connect(clientId.c_str(), _config.data().mqtt_user,
                          _config.data().mqtt_pass)) {
    Serial.println("connected");
    // Once connected, publish an announcement...
    _mqttClient.publish("silvia/status", "online");
    // ... and resubscribe
    _mqttClient.subscribe("silvia/setpoint/set");

    // Auto-Discovery
    sendDiscoveryConfig();
  } else {
    Serial.print("failed, rc=");
    Serial.print(_mqttClient.state());
    Serial.println(" try again in 5 seconds");
  }
}

void SilviaNetworkManager::sendDiscoveryConfig() {
  // Common Device info
  String device =
      "\"device\":{\"identifiers\":[\"silvia_pid\"],\"name\":\"Rancilio Silvia "
      "PID\",\"manufacturer\":\"Rancilio\",\"model\":\"Silvia "
      "V3\",\"sw_version\":\"1.0.0\"}";

  // 1. Current Temperature (Sensor)
  String tempPayload =
      "{\"name\": \"Silvia Temperature\", \"unique_id\": \"silvia_temp\", "
      "\"state_topic\": \"silvia/status\", \"value_template\": \"{{ "
      "value_json.temp }}\", \"unit_of_measurement\": \"°C\", "
      "\"device_class\": \"temperature\", " +
      device + "}";
  _mqttClient.publish("homeassistant/sensor/silvia/temperature/config",
                      tempPayload.c_str(), true);

  // 2. Target Temperature (Sensor for now, eventually Number)
  String targetPayload =
      "{\"name\": \"Silvia Target\", \"unique_id\": \"silvia_target\", "
      "\"state_topic\": \"silvia/status\", \"value_template\": \"{{ "
      "value_json.target }}\", \"unit_of_measurement\": \"°C\", "
      "\"device_class\": \"temperature\", " +
      device + "}";
  _mqttClient.publish("homeassistant/sensor/silvia/target/config",
                      targetPayload.c_str(), true);

  // 3. Output (Sensor)
  String outputPayload =
      "{\"name\": \"Silvia Heater Output\", \"unique_id\": \"silvia_output\", "
      "\"state_topic\": \"silvia/status\", \"value_template\": \"{{ "
      "value_json.output }}\", \"unit_of_measurement\": \"%\", \"icon\": "
      "\"mdi:radiator\", " +
      device + "}";
  _mqttClient.publish("homeassistant/sensor/silvia/output/config",
                      outputPayload.c_str(), true);

  // 4. State (Sensor)
  String statePayload =
      "{\"name\": \"Silvia State\", \"unique_id\": \"silvia_state\", "
      "\"state_topic\": \"silvia/status\", \"value_template\": \"{{ "
      "value_json.state }}\",  \"icon\": \"mdi:coffee-maker\", " +
      device + "}";
  _mqttClient.publish("homeassistant/sensor/silvia/state/config",
                      statePayload.c_str(), true);
}
