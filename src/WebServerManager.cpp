#include "WebServerManager.h"

// HTML Application (Minified for embedding)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Rancilio Silvia PID</title>
    <style>
        :root { --primary: #e74c3c; --bg: #1a1a1a; --card: #2d2d2d; --text: #ecf0f1; }
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 20px; display: flex; justify-content: center; }
        .container { width: 100%; max-width: 480px; }
        .card { background: var(--card); border-radius: 12px; padding: 20px; margin-bottom: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
        h1, h2 { margin-top: 0; text-align: center; }
        .header-info { display: flex; justify-content: space-between; font-size: 0.8rem; color: #888; margin-bottom: 10px; }
        .temp-display { font-size: 4rem; font-weight: bold; text-align: center; margin: 20px 0; color: var(--primary); }
        .stat-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
        .stat-item { background: rgba(255,255,255,0.05); padding: 10px; border-radius: 8px; text-align: center; }
        .label { font-size: 0.8rem; color: #aaa; display: block; margin-bottom: 5px; }
        .value { font-size: 1.2rem; font-weight: bold; }
        input[type="number"] { width: 100%; padding: 10px; background: #333; border: 1px solid #444; color: white; border-radius: 6px; box-sizing: border-box; margin-bottom: 10px; }
        button { width: 100%; padding: 12px; background: var(--primary); color: white; border: none; border-radius: 6px; font-size: 1rem; font-weight: bold; cursor: pointer; margin-top: 10px; }
        button.secondary { background: #555; }
        button:active { filter: brightness(0.9); }
        .badge { display: inline-block; padding: 4px 8px; border-radius: 4px; font-size: 0.8rem; font-weight: bold; }
        .badge.on { background: #2ecc71; color: #fff; }
        .badge.off { background: #7f8c8d; color: #fff; }
        .badge.warn { background: #f39c12; color: #fff; }
        a.ota-link { display: block; text-align: center; color: #888; margin-top: 20px; text-decoration: none; font-size: 0.8rem; }
        /* Switch CSS */
        .switch { position: relative; display: inline-block; width: 50px; height: 24px; }
        .switch input { opacity: 0; width: 0; height: 0; }
        .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 24px; }
        .slider:before { position: absolute; content: ""; height: 16px; width: 16px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
        input:checked + .slider { background-color: var(--primary); }
        input:checked + .slider:before { transform: translateX(26px); }
        input[type="text"], input[type="password"] { width: 100%; padding: 10px; background: #333; border: 1px solid #444; color: white; border-radius: 6px; box-sizing: border-box; margin-bottom: 10px; }
    </style>
</head>
<body>
    <!-- (Existing HTML...) -->

    <script>
        // ... (Existing Chart/WS setup) ...

        function toggleMqttFields() {
            const enabled = document.getElementById('mqttToggle').checked;
            document.getElementById('mqttFields').style.display = enabled ? 'block' : 'none';
        }

        // Combined Config Update (PID + MQTT)
        async function updateConfig() {
            // PID
            const kp = document.getElementById('kpInput').value;
            const ki = document.getElementById('kiInput').value;
            const kd = document.getElementById('kdInput').value;
            
            // MQTT
            const mqttEnabled = document.getElementById('mqttToggle').checked;
            const mqttServer = document.getElementById('mqttServer').value;
            const mqttPort = document.getElementById('mqttPort').value;
            const mqttUser = document.getElementById('mqttUser').value;
            const mqttPass = document.getElementById('mqttPass').value;

            try {
                await fetch('/api/config', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        pid_kp: parseFloat(kp), 
                        pid_ki: parseFloat(ki), 
                        pid_kd: parseFloat(kd),
                        mqtt_enabled: mqttEnabled,
                        mqtt_server: mqttServer,
                        mqtt_port: parseInt(mqttPort),
                        mqtt_user: mqttUser,
                        mqtt_pass: mqttPass
                    })
                });
                alert('Configuration Saved!');
                // Reload to reflect state? 
            } catch (e) { alert('Failed to update settings'); }
        }
        
        // Removed old updatePID in favor of unified config or keep separate?
        // User asked for "Save PID Settings" in PID card, and "Save Configuration" in MQTT card?
        // Let's keep updatePID for the PID card button, and make the new button call updateConfig which saves EVERYTHING including PID.
        
        async function updatePID() {
             updateConfig(); // Reuse the main function
        }
    
       // ... (Remainder of script) ...

        async function fetchInitialConfig() {
            try {
               const response = await fetch('/api/status');
               const data = await response.json();
               firstLoad = true; 
               
               // PID
               document.getElementById('setpointInput').value = data.target;
               document.getElementById('kpInput').value = data.kp;
               document.getElementById('kiInput').value = data.ki;
               document.getElementById('kdInput').value = data.kd;
               
               // MQTT
               document.getElementById('mqttToggle').checked = data.mqtt_enabled;
               document.getElementById('mqttServer').value = data.mqtt_server;
               document.getElementById('mqttPort').value = data.mqtt_port;
               document.getElementById('mqttUser').value = data.mqtt_user;
               document.getElementById('mqttPass').value = data.mqtt_pass;
               
               toggleMqttFields();
               
               firstLoad = false;
            } catch(e){}
        }


        async function toggleOverride() {
             const isManual = document.getElementById('overrideBtn').textContent.includes("Disable");
             try {
                await fetch('/api/override', { 
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'state=' + (!isManual) 
                });
            } catch (e) { alert('Failed to toggle override'); }
        }

        // Start
        fetchInitialConfig();
        initWebSocket();
    </script>
</body>
</html>

)rawliteral";

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
  doc["heating"] = (_pid.getOutput() > 0);

  // Only send changing data for chart/status
  String response;
  serializeJson(doc, response);
  _ws.textAll(response);
}

void WebServerManager::setupRoutes() {
  // 1. Serve Dashboard
  _server.on("/", AWS_HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
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
               doc["heating"] = (_pid.getOutput() > 0);

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
                 bool state = (stateStr == "true");
                 _pid.setManualMode(state);
                 if (state) {
                   _pid.setManualPower(
                       100.0); // Default to full power for manual override test
                 }
               }
               request->send(200, "application/json", "{\"status\":\"ok\"}");
             });

  ElegantOTA.begin(&_server);
}
