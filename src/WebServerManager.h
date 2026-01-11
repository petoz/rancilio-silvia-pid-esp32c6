#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include "Configuration.h"
#include "PID_Controller.h"
#include "Temperature.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

class WebServerManager {
public:
  WebServerManager(Configuration &config, Temperature &temp,
                   PID_Controller &pid);
  void begin();

private:
  AsyncWebServer _server;
  Configuration &_config;
  Temperature &_temp;
  PID_Controller &_pid;

  void setupRoutes();
  String getContentType(String filename);
};

#endif
