#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "settings.h"

class Communication
{
  private:
    WiFiClient _client;
    HTTPClient _http;
    String _serverHostname = SERVER_HOSTNAME;

  public:
    Communication();

    int postToWeb(const String& path, const String& message);
    String pollFromWeb(const String& path);
};

#endif
