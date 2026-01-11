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
  void loop();
  void broadcastStatus();

private:
  AsyncWebServer _server;
  AsyncWebSocket _ws;
  Configuration &_config;
  Temperature &_temp;
  PID_Controller &_pid;

  void setupRoutes();
  String getContentType(String filename);
  void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len);
};

#endif
