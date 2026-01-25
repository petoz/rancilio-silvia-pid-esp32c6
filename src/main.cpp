#include "Configuration.h"
#include "NetworkManager.h"
#include "PID_Controller.h"
#include "SSR_Driver.h"
#include "Temperature.h"
#include "WebServerManager.h"
#include "config.h"
#include <Arduino.h>

// Global Modules
Configuration config;
SilviaNetworkManager networkManager(config);
Temperature temperature;
PID_Controller pid;
SSR_Driver ssr(PIN_SSR);
WebServerManager webServer(config, temperature, pid);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Rancilio Silvia PID Starting...");
  Serial.println("ECM-Style Logic: ENABLED");
  Serial.println(
      "Zones: Warmup >20C | Approach 20-1C (Ramp 45-30%) | Stable <1C");

  config.begin();
  networkManager.setModules(temperature, pid);
  networkManager.begin();
  temperature.begin();
  temperature.setCorrection(config.getTempCorrection());
  temperature.setRref(config.data().rref);
  // 4. Initialize PID
  if (config.data().heater_enabled) {
    pid.begin(); // Auto mode
  } else {
    pid.setTunings(
        30.0, 2.0,
        80.0); // ECM Style Default Tunings (Lower P for less overshoot)
    pid.begin();
    pid.setManualMode(true);
    pid.setManualPower(0);
  }
  ssr.begin();

  webServer.begin();
}

void loop() {
  temperature.update();

  // PID Loop
  double currentTemp = temperature.getTemperature();
  double targetTemp = config.getTargetTemp();
  double output = pid.compute(currentTemp, targetTemp);

  ssr.setPower(output);
  ssr.loop();

#ifdef SIMULATION_MODE
  temperature.updateSimulation(output);
#endif

  // WS Cleanup
  webServer.loop();

  // MQTT Cleanup
  networkManager.loop();

  // Safety / Self-Healing
  // If config says Heater ON, but PID is in Manual Mode, force it back to Auto.
  // This prevents getting stuck in Manual 0% if a glitch occurs.
  if (config.data().heater_enabled && pid.isManualMode()) {
    Serial.println("Safety: Detected stuck Manual Mode. Forcing AUTO.");
    pid.setManualMode(false);
  }

  unsigned long now = millis();
  static unsigned long lastPrint = 0;
  if (now - lastPrint >= 500) { // 2Hz Update Rate for Chart
    lastPrint = now;

    uint8_t fault = temperature.getFault();

    // Serial Debug
    Serial.print("Target: ");
    Serial.print(targetTemp);
    Serial.print(" C, Temp: ");
    Serial.print(currentTemp);
    Serial.print(" C, Output: ");
    Serial.print(output);
    Serial.print("% (Diff: ");
    Serial.print(targetTemp - currentTemp);
    Serial.println(" C)");

    // Broadcast WebSocket
    webServer.broadcastStatus();

    if (temperature.hasFault()) {
      Serial.print("FAULT DETECTED! Code: 0x");
      Serial.println(fault, HEX);
      pid.setManualMode(true);
      pid.setManualPower(0); // Safely disable heater
    }
  }
}
