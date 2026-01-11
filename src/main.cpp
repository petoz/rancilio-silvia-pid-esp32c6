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

  config.begin();
  networkManager.begin();
  temperature.begin();
  pid.begin();
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

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();

    uint8_t fault = temperature.getFault();

    Serial.print("Temp: ");
    Serial.print(currentTemp);
    Serial.print(" C | Target: ");
    Serial.print(targetTemp);
    Serial.print(" | Out: ");
    Serial.print(output);
    Serial.print("% | Mode: ");
    Serial.println(pid.isManualMode() ? "MANUAL" : "AUTO");

    if (temperature.hasFault()) {
      Serial.print("FAULT DETECTED! Code: 0x");
      Serial.println(fault, HEX);
      pid.setManualMode(true);
      pid.setManualPower(0); // Safely disable heater
    }
  }
}
