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
        input[type="number"] { width: 100%; padding: 10px; background: #333; border: 1px solid #444; color: white; border-radius: 6px; box-sizing: border-box; }
        button { width: 100%; padding: 12px; background: var(--primary); color: white; border: none; border-radius: 6px; font-size: 1rem; font-weight: bold; cursor: pointer; margin-top: 10px; }
        button.secondary { background: #555; }
        button:active { filter: brightness(0.9); }
        .badge { display: inline-block; padding: 4px 8px; border-radius: 4px; font-size: 0.8rem; font-weight: bold; }
        .badge.on { background: #2ecc71; color: #fff; }
        .badge.off { background: #7f8c8d; color: #fff; }
        .badge.warn { background: #f39c12; color: #fff; }
        a.ota-link { display: block; text-align: center; color: #888; margin-top: 20px; text-decoration: none; font-size: 0.8rem; }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <div class="header-info">
                <span id="rssi">WiFi: -- dBm</span>
                <span id="uptime">Up: --:--:--</span>
            </div>
            <h1>Silvia PID</h1>
            <div class="temp-display" id="currentTemp">--.-°C</div>
            <div class="stat-grid">
                <div class="stat-item">
                    <span class="label">Target</span>
                    <span class="value" id="targetTemp">--.-°C</span>
                </div>
                <div class="stat-item">
                    <span class="label">Heater</span>
                    <span class="badge off" id="heaterState">OFF</span>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>Controls</h2>
            <label class="label">Set Target Temperature (°C)</label>
            <input type="number" id="setpointInput" step="0.1" value="95.0">
            <button onclick="updateSetpoint()">Update Target</button>
            <button class="secondary" onclick="toggleOverride()" id="overrideBtn" style="margin-top: 20px;">Enable Manual Heater Override</button>
        </div>

        <a href="/update" class="ota-link">Update Firmware (OTA)</a>
    </div>

    <script>
        function formatTime(ms) {
            let seconds = Math.floor(ms / 1000);
            let minutes = Math.floor(seconds / 60);
            let hours = Math.floor(minutes / 60);
            seconds = seconds % 60;
            minutes = minutes % 60;
            return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
        }

        function updateUI(data) {
            document.getElementById('currentTemp').textContent = data.temp.toFixed(1) + '°C';
            document.getElementById('targetTemp').textContent = data.setpoint.toFixed(1) + '°C';
            const heater = document.getElementById('heaterState');
            heater.textContent = data.heating ? 'ON' : 'OFF';
            heater.className = 'badge ' + (data.heating ? 'on' : 'off');
            
            document.getElementById('rssi').textContent = `WiFi: ${data.rssi} dBm`;
            document.getElementById('uptime').textContent = `Up: ${formatTime(data.uptime)}`;

            const overrideBtn = document.getElementById('overrideBtn');
            if (data.manual) {
                overrideBtn.textContent = "Disable Manual Override";
                overrideBtn.style.background = "#f39c12";
            } else {
                overrideBtn.textContent = "Enable Manual Heater Override";
                overrideBtn.style.background = "#555";
            }
        }

        async function fetchStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                updateUI(data);
            } catch (e) { console.error(e); }
        }

        async function updateSetpoint() {
            const val = document.getElementById('setpointInput').value;
            try {
                await fetch('/api/setpoint', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({setpoint: parseFloat(val)})
                });
                fetchStatus();
            } catch (e) { alert('Failed to update setpoint'); }
        }

        async function toggleOverride() {
             try {
                await fetch('/api/override', { method: 'POST' });
                fetchStatus();
            } catch (e) { alert('Failed to toggle override'); }
        }

        setInterval(fetchStatus, 1000);
        fetchStatus();
    </script>
</body>
</html>
)rawliteral";

WebServerManager::WebServerManager(Configuration &config, Temperature &temp,
                                   PID_Controller &pid)
    : _server(80), _config(config), _temp(temp), _pid(pid) {}

void WebServerManager::begin() {
  setupRoutes();
  _server.begin();
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
               doc["state"] =
                   _pid.isManualMode() ? "MANUAL" : "IDLE"; // Simplified state
               if (_pid.getOutput() > 0)
                 doc["state"] = "HEATING";
               doc["setpoint"] = 95.0; // Placeholder until PID integrated
               doc["heating"] = false; // Placeholder
               doc["kp"] = _config.data().pid_kp;
               doc["ki"] = _config.data().pid_ki;
               doc["kd"] = _config.data().pid_kd;

               // New Fields
               doc["rssi"] = WiFi.RSSI();
               doc["uptime"] = millis();
               doc["manual"] = false; // Placeholder for manual mode state

               String response;
               serializeJson(doc, response);
               request->send(200, "application/json", response);
             });

  // 3. API Setpoint Endpoint
  _server.on(
      "/api/setpoint", AWS_HTTP_POST, [](AsyncWebServerRequest *request) {},
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if (!error) {
          float newSetpoint = doc["setpoint"];
          Serial.printf("New Setpoint: %.1f\n", newSetpoint);
          request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
          request->send(400, "application/json",
                        "{\"error\":\"Invalid JSON\"}");
        }
      });

  // 4. API Manual Override Endpoint (Stub)
  _server.on(
      "/api/override", AWS_HTTP_POST, [](AsyncWebServerRequest *request) {},
      NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        // Stub for manual override
        request->send(200, "application/json", "{\"status\":\"ok\"}");
      });

  // Start ElegantOTA
  ElegantOTA.begin(&_server);
}
