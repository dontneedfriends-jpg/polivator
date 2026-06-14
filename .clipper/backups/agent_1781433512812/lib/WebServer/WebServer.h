#ifndef WEBSERVER_H
#define WEBSERVER_H

class WebServer {
public:
  WebServer();
  ~WebServer();
  void begin();
  void handleClient();
  void sendData(float moisture, float temp, float humidity);

private:
  // Web server members
};

#endif // WEBSERVER_H
