#include "WebServerManager.h"

#include "index_html.h"

WebServerManager::WebServerManager(Configuration &config, Temperature &temp,
                                   PID_Controller &pid)
    : _server(80), _ws("/ws"), _config(config), _temp(temp), _pid(pid) {}

void WebServerManager::begin() {
  setupRoutes();
  _ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                     AwsEventType type, void *arg, uint8_t *data, size_t len) {
    this->onEvent(server, client, type, arg, data, len);
  });
  _server.addHandler(&_ws);
  _server.begin();
}

void WebServerManager::loop() { _ws.cleanupClients(); }

void WebServerManager::onEvent(AsyncWebSocket *server,
                               AsyncWebSocketClient *client, AwsEventType type,
                               void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    // Could send initial status here immediately
  } else if (type == WS_EVT_DISCONNECT) {
    // Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  }
}

void WebServerManager::broadcastStatus() {
  if (_ws.count() == 0)
    return;

  JsonDocument doc;
  doc["temp"] = _temp.getTemperature();
  doc["target"] = _config.getTargetTemp();
  doc["output"] = _pid.getOutput();
  doc["state"] = _pid.isManualMode() ? "MANUAL" : "AUTO";
  if (_pid.getOutput() > 0 && !_pid.isManualMode())
    doc["state"] = "HEATING";

  doc["rssi"] = WiFi.RSSI();
  doc["uptime"] = millis();
  doc["manual"] = _pid.isManualMode();
  doc["rssi"] = WiFi.RSSI();
  doc["uptime"] = millis();
  doc["manual"] = _pid.isManualMode();
  doc["heating"] = (_pid.getOutput() > 0);
  doc["heater_enabled"] = _config.data().heater_enabled;

  // Only send changing data for chart/status
  String response;
  serializeJson(doc, response);
  _ws.textAll(response);
}

void WebServerManager::setupRoutes() {
  // 1. Serve Dashboard
  _server.on("/", AWS_HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // 2. API Status Endpoint
  _server.on("/api/status", AWS_HTTP_GET,
             [this](AsyncWebServerRequest *request) {
               JsonDocument doc;
               doc["temp"] = _temp.getTemperature();
               doc["target"] = _config.getTargetTemp();
               doc["output"] = _pid.getOutput();
               doc["state"] = _pid.isManualMode() ? "MANUAL" : "AUTO";
               if (_pid.getOutput() > 0 && !_pid.isManualMode())
                 doc["state"] = "HEATING";

               doc["kp"] = _config.data().pid_kp;
               doc["ki"] = _config.data().pid_ki;
               doc["kd"] = _config.data().pid_kd;

               // MQTT Params for UI
               doc["mqtt_enabled"] = _config.data().mqtt_enabled;
               doc["mqtt_server"] = _config.data().mqtt_server;
               doc["mqtt_port"] = _config.data().mqtt_port;
               doc["mqtt_user"] = _config.data().mqtt_user;
               // Do not send password for security, or send mask if needed.
               // User asked for fields to be shown, usually implies editing.
               // We can send it since it's local network admin dashboard.
               doc["mqtt_pass"] = _config.data().mqtt_pass;

               doc["rssi"] = WiFi.RSSI();
               doc["uptime"] = millis();
               doc["manual"] = _pid.isManualMode();
               doc["rssi"] = WiFi.RSSI();
               doc["uptime"] = millis();
               doc["manual"] = _pid.isManualMode();
               doc["heating"] = (_pid.getOutput() > 0);
               doc["heater_enabled"] = _config.data().heater_enabled;

               String response;
               serializeJson(doc, response);
               request->send(200, "application/json", response);
             });

  // 3. API Setpoint Endpoint
  _server.on(
      "/api/setpoint", AWS_HTTP_POST, [](AsyncWebServerRequest *request) {},
      NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if (!error) {
          float newSetpoint = doc["setpoint"];
          _config.data().pid_setpoint = newSetpoint;
          _config.save();
          request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
          request->send(400, "application/json",
                        "{\"error\":\"Invalid JSON\"}");
        }
      });

  // 4. API Config Endpoint (PID)
  _server.on(
      "/api/config", AWS_HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if (!error) {
          // PID Config
          if (doc["pid_kp"].is<float>())
            _config.data().pid_kp = doc["pid_kp"];
          if (doc["pid_ki"].is<float>())
            _config.data().pid_ki = doc["pid_ki"];
          if (doc["pid_kd"].is<float>())
            _config.data().pid_kd = doc["pid_kd"];

          // MQTT Config
          if (doc["mqtt_enabled"].is<bool>())
            _config.data().mqtt_enabled = doc["mqtt_enabled"];
          if (doc["mqtt_server"].is<const char *>())
            strlcpy(_config.data().mqtt_server, doc["mqtt_server"],
                    sizeof(_config.data().mqtt_server));
          if (doc["mqtt_port"].is<int>())
            _config.data().mqtt_port = doc["mqtt_port"];
          if (doc["mqtt_user"].is<const char *>())
            strlcpy(_config.data().mqtt_user, doc["mqtt_user"],
                    sizeof(_config.data().mqtt_user));
          if (doc["mqtt_pass"].is<const char *>())
            strlcpy(_config.data().mqtt_pass, doc["mqtt_pass"],
                    sizeof(_config.data().mqtt_pass));

          _config.save();

          // Apply new tunings immediately
          _pid.setTunings(_config.data().pid_kp, _config.data().pid_ki,
                          _config.data().pid_kd);
          request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
          request->send(400, "application/json",
                        "{\"error\":\"Invalid JSON\"}");
        }
      });

  // 5. API Manual Override Endpoint
  _server.on("/api/override", AWS_HTTP_POST,
             [this](AsyncWebServerRequest *request) {
               if (request->hasParam("state", true)) {
                 String stateStr = request->getParam("state", true)->value();
                 bool turnOn = (stateStr == "true");

                 _config.data().heater_enabled = turnOn;
                 _config.save(); // Persist

                 if (turnOn) {
                   // Resume Auto PID
                   _pid.setManualMode(false);
                 } else {
                   // Force Off (Manual 0%)
                   _pid.setManualMode(true);
                   _pid.setManualPower(0.0);
                 }
               }
               request->send(200, "application/json", "{\"status\":\"ok\"}");
             });

  ElegantOTA.begin(&_server);
}
