#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

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
                <div class="stat-item">
                     <span class="label">Output</span>
                     <span class="value" id="pidOutput">0%</span>
                </div>
                 <div class="stat-item">
                     <span class="label">Mode</span>
                     <span class="value" id="sysMode">--</span>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>Controls</h2>
            <label class="label">Set Target Temperature (°C)</label>
            <input type="number" id="setpointInput" step="0.1" value="95.0">
            <button onclick="updateSetpoint()">Update Target</button>
            <button class="secondary" onclick="toggleOverride()" id="overrideBtn" style="margin-top: 20px;">Heater Power: CHECKING...</button>
        </div>

        <div class="card">
            <h2>PID Tuning</h2>
            <label class="label">Proportional (Kp)</label>
            <input type="number" id="kpInput" step="0.1">
            <label class="label">Integral (Ki)</label>
            <input type="number" id="kiInput" step="0.01">
            <label class="label">Derivative (Kd)</label>
            <input type="number" id="kdInput" step="0.1">
            <button onclick="updatePID()">Save PID Settings</button>
        </div>

        <div class="card">
            <h2>MQTT / Home Assistant</h2>
            <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 20px;">
                <label class="label" style="margin:0;">Enable Integration</label>
                <label class="switch">
                <input type="checkbox" id="mqttToggle" onchange="toggleMqttFields()">
                <span class="slider round"></span>
                </label>
            </div>
            
            <div id="mqttFields" style="display:none;">
                <label class="label">Broker Address</label>
                <input type="text" id="mqttServer" placeholder="192.168.1.x">
                
                <label class="label">Port</label>
                <input type="number" id="mqttPort" placeholder="1883">
                
                <label class="label">Username</label>
                <input type="text" id="mqttUser">
                
                <label class="label">Password</label>
                <input type="password" id="mqttPass">
            </div>
            <button onclick="updateConfig()">Save Configuration</button>
        </div>

        <div class="card">
            <h2>Temperature History</h2>
            <canvas id="tempChart" width="400" height="200"></canvas>
        </div>
        
        <a href="/update" class="ota-link">Firmware Update (OTA)</a>
    </div> 

    <!-- Chart.js CDN -->
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-streaming@2.0.0"></script> 
    
    <script>
        // Chart Initialization
        const ctx = document.getElementById('tempChart').getContext('2d');
        const maxDataPoints = 60; // 30 seconds at 2Hz
        const tempChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: Array(maxDataPoints).fill(''),
                datasets: [
                    {
                        label: 'Temperature (°C)',
                        data: Array(maxDataPoints).fill(null),
                        borderColor: '#e74c3c',
                        backgroundColor: 'rgba(231, 76, 60, 0.2)',
                        tension: 0.4,
                        fill: true,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Target (°C)',
                        data: Array(maxDataPoints).fill(null),
                        borderColor: '#2ecc71',
                        borderDash: [5, 5],
                        fill: false,
                        yAxisID: 'y'
                    },
                    {
                        label: 'Output (%)',
                        data: Array(maxDataPoints).fill(null),
                        borderColor: '#f1c40f',
                        borderWidth: 1,
                        pointRadius: 0,
                        fill: true,
                        backgroundColor: 'rgba(241, 196, 15, 0.1)',
                        yAxisID: 'y1'
                    }
                ]
            },
            options: {
                responsive: true,
                animation: false, // Performance
                interaction: { mode: 'index', intersect: false },
                scales: {
                    x: { display: false },
                    y: { 
                        type: 'linear', display: true, position: 'left',
                        suggestedMin: 20, suggestedMax: 110,
                        grid: { color: '#444' }, title: {display: true, text: 'Temp (°C)'}
                    },
                    y1: {
                        type: 'linear', display: true, position: 'right',
                        min: 0, max: 105,
                        grid: { drawOnChartArea: false }, title: {display : true, text: 'Output %'}
                    }
                },
                plugins: { legend: { labels: { color: '#ccc' } } }
            }
        });

        function addData(chart, temp, target, output) {
            chart.data.datasets[0].data.push(temp);
            chart.data.datasets[1].data.push(target);
            chart.data.datasets[2].data.push(output);
            
            if (chart.data.datasets[0].data.length > maxDataPoints) {
                chart.data.datasets[0].data.shift();
                chart.data.datasets[1].data.shift();
                chart.data.datasets[2].data.shift();
            }
            chart.update();
        }

        // WebSockets
        let socket;
        function initWebSocket() {
            socket = new WebSocket('ws://' + window.location.hostname + '/ws');
            socket.onopen = function(e) { console.log("WS Connected"); };
            socket.onclose = function(e) { console.log("WS Disconnected"); setTimeout(initWebSocket, 2000); };
            socket.onmessage = function(event) {
                const data = JSON.parse(event.data);
                updateUI(data); 
                addData(tempChart, data.temp, data.target, data.output);
            };
        }

        function formatTime(ms) {
            let seconds = Math.floor(ms / 1000);
            let minutes = Math.floor(seconds / 60);
            let hours = Math.floor(minutes / 60);
            seconds = seconds % 60;
            minutes = minutes % 60;
            return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
        }

        let firstLoad = true;
        function updateUI(data) {
            document.getElementById('currentTemp').textContent = data.temp.toFixed(1) + '°C';
            document.getElementById('targetTemp').textContent = data.target.toFixed(1) + '°C';
            const heater = document.getElementById('heaterState');
            heater.textContent = data.heating ? 'ON' : 'OFF';
            heater.className = 'badge ' + (data.heating ? 'on' : 'off');
            
            document.getElementById('pidOutput').textContent = data.output.toFixed(0) + '%';
            document.getElementById('sysMode').textContent = data.state;

            document.getElementById('rssi').textContent = `WiFi: ${data.rssi} dBm`;
            document.getElementById('uptime').textContent = `Up: ${formatTime(data.uptime)}`;

            const overrideBtn = document.getElementById('overrideBtn');
            if (data.heater_enabled) {
                overrideBtn.textContent = "Turn Heater OFF";
                overrideBtn.style.background = "#e74c3c"; // Red to stop
            } else {
                overrideBtn.textContent = "Turn Heater ON";
                overrideBtn.style.background = "#2ecc71"; // Green to start
            }
        }
        
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
               if(data.mqtt_enabled !== undefined) {
                   document.getElementById('mqttToggle').checked = data.mqtt_enabled;
                   document.getElementById('mqttServer').value = data.mqtt_server || '';
                   document.getElementById('mqttPort').value = data.mqtt_port || '';
                   document.getElementById('mqttUser').value = data.mqtt_user || '';
                   document.getElementById('mqttPass').value = data.mqtt_pass || '';
                   toggleMqttFields();
               }

               firstLoad = false;
            } catch(e){ console.error(e); }
        }

        function toggleMqttFields() {
            const enabled = document.getElementById('mqttToggle').checked;
            document.getElementById('mqttFields').style.display = enabled ? 'block' : 'none';
        }

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
            } catch (e) { alert('Failed to update settings'); }
        }
        
        async function updatePID() {
             updateConfig(); 
        }

        async function updateSetpoint() {
            const val = document.getElementById('setpointInput').value;
            try {
                await fetch('/api/setpoint', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({setpoint: parseFloat(val)})
                });
            } catch (e) { alert('Failed to update setpoint'); }
        }

        async function toggleOverride() {
             const btnText = document.getElementById('overrideBtn').textContent;
             const turningOn = btnText.includes("ON"); // If it says "Turn Heater ON", we are turning it on
             
             try {
                await fetch('/api/override', { 
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'state=' + (turningOn) 
                });
                // Optimistic update
                if(turningOn) {
                    document.getElementById('overrideBtn').textContent = "Turn Heater OFF";
                    document.getElementById('overrideBtn').style.background = "#e74c3c";
                } else {
                    document.getElementById('overrideBtn').textContent = "Turn Heater ON";
                    document.getElementById('overrideBtn').style.background = "#2ecc71";
                }
            } catch (e) { alert('Failed to toggle heater'); }
        }

        // Start
        fetchInitialConfig();
        initWebSocket();
    </script>
</body>
</html>
)rawliteral";

#endif
