#include "communication.h"

Communication::Communication()
{
}

int Communication::postToWeb(const String& path, const String& message)
{
  if (WiFi.status() != WL_CONNECTED) return 0;

  _http.begin(_client, "http://" + _serverHostname + path);
  _http.addHeader("Content-Type", "text/plain");

  int httpCode = _http.POST(message);
  
  // drain response without storing it
  WiFiClient* stream = _http.getStreamPtr();
  if (stream) while (stream->available()) stream->read();
  
  _http.end();
  return (httpCode > 0) ? 1 : -1;
}

String Communication::pollFromWeb(const String& path)
{
  if (WiFi.status() != WL_CONNECTED) return "ERROR";

  HTTPClient http;
  http.begin(_client, "http://" + _serverHostname + path);
  int httpCode = http.GET();

  if (httpCode <= 0) { http.end(); return "ERROR"; }

  String msg = http.getString();
  http.end();
  return msg;
}
