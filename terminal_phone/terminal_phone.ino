#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <TFT_eSPI.h>

#include "settings.h"
#include "globals.h"
#include "terminal.h"
#include "commands.h"
#include "communication.h"
#include "sms.h"
#include "memory.h"
#include "chat.h"

TFT_eSPI tft = TFT_eSPI();

Terminal terminal(0, 0, SCREEN_W, SCREEN_H);
Terminal termInfo(0, 0, SCREEN_W, CHAR_H);
Terminal termMessages(0, CHAR_H, SCREEN_W, SCREEN_H - 3 * CHAR_H);
Terminal termSendBar(0, SCREEN_H - 2 * CHAR_H, SCREEN_W, 2 * CHAR_H);

Communication comm;
SMS sms;
Memory data;
Chat* currentChat = nullptr;

String inputLine = "";
int mainState = 1;
bool readChar = false;
unsigned long lastPollTime = 0;

void setup()
{
  Serial.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  tft.init();
  tft.setRotation(1);
  tft.writecommand(0x36);
  tft.writedata(0xA8);
  tft.writecommand(0x20);
  tft.fillScreen(TFT_BLACK);

  terminal.println("Terminal Phone v1.0");
  terminal.println("-------------------");

  if (!LittleFS.begin())
  {
    terminal.println("LittleFS init error", TFT_RED);
    terminal.println("Please reboot and diagnose!", TFT_YELLOW);
    while (1) yield();
  }

  runCommand("wifi connect benji benji2021");

  sms.updateContacts();

  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());

  terminal.print(PROMPT);
}

void loop()
{
  char c = 0;

  if (Serial.available())
  {
    c = Serial.read();
    readChar = true;
  }

  if (mainState == 1)
  {
    if (readChar)
    {
      if (c == '\n' || c == '\r')
      {
        terminal.println();
        runCommand(inputLine);
        if (mainState == 1)
          terminal.print(PROMPT);
        inputLine = "";
      }
      else if (c == '\b' || c == 127)
      {
        if (inputLine.length() > 0)
        {
          inputLine.remove(inputLine.length() - 1);
          terminal.print('\b');
        }
      }
      else
      {
        inputLine += c;
        terminal.print(c);
      }

      readChar = false;
    }
    //if (millis() - lastPollTime >= 5000)
    //{
    //  sms.readSMS();
    //}
  }
  else if (mainState == 2 && currentChat != nullptr)
  {
    if (readChar)
    {
      currentChat->updateKey(c);
      readChar = false;
    }

    if (millis() - lastPollTime >= 5000)
    {
      lastPollTime = millis();
      currentChat->updateMsgs();
    }
  }

  yield();
}
