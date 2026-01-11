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
        .temp-display { font-size: 4rem; font-weight: bold; text-align: center; margin: 20px 0; color: var(--primary); }
        .stat-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
        .stat-item { background: rgba(255,255,255,0.05); padding: 10px; border-radius: 8px; text-align: center; }
        .label { font-size: 0.8rem; color: #aaa; display: block; margin-bottom: 5px; }
        .value { font-size: 1.2rem; font-weight: bold; }
        input[type="number"] { width: 100%; padding: 10px; background: #333; border: 1px solid #444; color: white; border-radius: 6px; box-sizing: border-box; }
        button { width: 100%; padding: 12px; background: var(--primary); color: white; border: none; border-radius: 6px; font-size: 1rem; font-weight: bold; cursor: pointer; margin-top: 10px; }
        button:active { filter: brightness(0.9); }
        .badge { display: inline-block; padding: 4px 8px; border-radius: 4px; font-size: 0.8rem; font-weight: bold; }
        .badge.on { background: #2ecc71; color: #fff; }
        .badge.off { background: #7f8c8d; color: #fff; }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
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
            <h2>Generic Settings</h2>
             <div class="stat-grid" style="margin-bottom: 10px;">
                <div class="stat-item">
                     <span class="label">Kp</span>
                     <span class="value" id="dispKp">--</span>
                </div>
                 <div class="stat-item">
                     <span class="label">Ki</span>
                     <span class="value" id="dispKi">--</span>
                </div>
                 <div class="stat-item">
                     <span class="label">Kd</span>
                     <span class="value" id="dispKd">--</span>
                </div>
            </div>
            <label class="label">Set Target Temperature (°C)</label>
            <input type="number" id="setpointInput" step="0.1" value="95.0">
            <button onclick="updateSetpoint()">Update Target</button>
        </div>
    </div>

    <script>
        function updateUI(data) {
            document.getElementById('currentTemp').textContent = data.temp.toFixed(1) + '°C';
            document.getElementById('targetTemp').textContent = data.setpoint.toFixed(1) + '°C';
            const heater = document.getElementById('heaterState');
            heater.textContent = data.heating ? 'ON' : 'OFF';
            heater.className = 'badge ' + (data.heating ? 'on' : 'off');
            
            document.getElementById('dispKp').textContent = data.kp.toFixed(2);
            document.getElementById('dispKi').textContent = data.ki.toFixed(2);
            document.getElementById('dispKd').textContent = data.kd.toFixed(2);
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

        // Poll every 1 second
        setInterval(fetchStatus, 1000);
        fetchStatus();
    </script>
</body>
</html>
)rawliteral";

WebServerManager::WebServerManager(Configuration &config, Temperature &temp)
    : _server(80), _config(config), _temp(temp) {}

void WebServerManager::begin() {
  setupRoutes();
  _server.begin();
}

void WebServerManager::setupRoutes() {
  // 1. Serve Dashboard
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // 2. API Status Endpoint
  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["temp"] = _temp.getTemperature();
    doc["setpoint"] = 95.0; // Placeholder until PID integrated
    doc["heating"] = false; // Placeholder
    doc["kp"] = _config.data().pid_kp;
    doc["ki"] = _config.data().pid_ki;
    doc["kd"] = _config.data().pid_kd;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // 3. API Setpoint Endpoint
  // Use AsyncWebHandler for body parsing is tricky in lambda, simplified
  // approach:
  _server.on(
      "/api/setpoint", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [](AsyncWebServerRequest *request, uint8_t *data, size_t len,
         size_t index, size_t total) {
        // Simple body parser
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if (!error) {
          float newSetpoint = doc["setpoint"];
          // potentially save to config or update PID
          // For now just log
          Serial.printf("New Setpoint: %.1f\n", newSetpoint);
          request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
          request->send(400, "application/json",
                        "{\"error\":\"Invalid JSON\"}");
        }
      });
}
