#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include "Configuration.h"
#include "Temperature.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

class WebServerManager {
public:
  WebServerManager(Configuration &config, Temperature &temp);
  void begin();

private:
  AsyncWebServer _server;
  Configuration &_config;
  Temperature &_temp;

  void setupRoutes();
  String getContentType(String filename);
};

#endif
