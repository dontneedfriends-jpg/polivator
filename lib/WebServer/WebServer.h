#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <Preferences.h>
#include "../Sensor/Sensor.h"
#include "../Common/Types.h"
#include "../Settings/Settings.h"

class Calibration;
class Display;

class WebServer {
public:
  WebServer(SensorManager* sensorManager, Calibration* calibration, Display* display, Settings* settings);
  ~WebServer();
  bool begin();
  void handleClient();
  void stop();
  void sendStatusEvent(SensorManager* sensorManager);
  void handleUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

private:
  void setupAP();
  bool connectToSavedWiFi();
  void setupRoutes();
  static int getIndexFromParam(AsyncWebServerRequest *request);

  AsyncWebServer server;
  AsyncEventSource events{"/events"};
  DNSServer dnsServer;
  SensorManager* m_sensorManager;
  Calibration* m_calibration;
  Display* m_display;
  Settings* m_settings;
  Preferences preferences;
  bool updateInProgress = false;
  uint32_t uploadStartTime = 0;
};

#endif // WEBSERVER_H
