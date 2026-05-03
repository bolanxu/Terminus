#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "settings.h"

typedef void (*CmdFunction)(String args);

struct Command
{
  String name;
  CmdFunction function;
};

void runCommand(String cmd);

void handle_help(String args);
void handle_clear(String args);
void handle_echo(String args);
void handle_quote(String args);
void handle_wifi(String args);
void handle_sms(String args);
void handle_heap(String args);

extern Command commandTable[];
extern String codeMessages[];
extern int numOfCommands;

#endif
