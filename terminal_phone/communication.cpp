#include "communication.h"

Communication::Communication()
{
}

int Communication::postToWeb(const String& path, const String& message)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return 0;
  }

  _http.begin(_client, "http://" + _serverHostname + path);
  _http.addHeader("Content-Type", "text/plain");

  int httpCode = _http.POST(message);
  int returnCode = (httpCode > 0) ? 1 : -1;

  _http.end();
  return returnCode;
}

String Communication::pollFromWeb(const String& path)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return "ERROR";
  }

  _http.begin(_client, "http://" + _serverHostname + path);
  int httpCode = _http.GET();

  String msg;
  if (httpCode > 0)
  {
    msg = _http.getString();
  }
  else
  {
    msg = "ERROR";
  }

  _http.end();
  return msg;
}
