#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <Preferences.h>

class Sensor;
class Calibration;
class Display;

class WebServer {
public:
  WebServer(Sensor* sensor, Calibration* calibration, Display* display);
  ~WebServer();
  void begin();
  void handleClient();
  void stop();

private:
  void setupAP();
  bool connectToSavedWiFi();
  void setupRoutes();

  AsyncWebServer server;
  AsyncEventSource events{"/events"};
  DNSServer dnsServer;
  Sensor* m_sensor;
  Calibration* m_calibration;
  Display* m_display;
  Preferences preferences;
};

#endif // WEBSERVER_H
