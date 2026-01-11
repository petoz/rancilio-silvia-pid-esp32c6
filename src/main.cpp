#include "Configuration.h"
#include "NetworkManager.h"
#include "Temperature.h"
#include "WebServerManager.h"
#include "config.h"
#include <Arduino.h>

// Global Modules
Configuration config;
SilviaNetworkManager networkManager(config);
Temperature temperature;
WebServerManager webServer(config, temperature);

void setup() {
  Serial.begin(115200);
  // while (!Serial) { delay(10); } // Removing blocking wait
  delay(1000); // Give some time for USB to enumerate
  Serial.println("Rancilio Silvia PID Starting...");

  // 1. Load Configuration
  config.begin();

  // 2. Start Network/WiFi
  // This might block if it goes to AP mode, which is fine for startup
  networkManager.begin();

  // 3. Init Hardware
  temperature.begin();

  // 4. Start Web Server
  webServer.begin();
}

void loop() {
  // Update temperature state machine
  temperature.update();

  // Non-blocking print every 1 second
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 1000) {
    lastPrint = millis();

    float currentTemp = temperature.getTemperature();
    uint8_t fault = temperature.getFault();

    Serial.print("Temp: ");
    Serial.print(currentTemp);
    Serial.print(" C | Fault Code: 0x");
    Serial.println(fault, HEX);

    if (temperature.hasFault()) {
      Serial.print("FAULT DETECTED! Code: 0x");
      Serial.println(fault, HEX);
      // Here we would safely disable SSR
    }
  }
}
