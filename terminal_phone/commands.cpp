#include "commands.h"
#include "globals.h"
#include "chat.h"
#include "terminal.h"
#include "sms.h"

Command commandTable[] =
{
  {"help",  handle_help },
  {"clear", handle_clear},
  {"echo",  handle_echo },
  {"quote", handle_quote},
  {"wifi",  handle_wifi},
  {"sms",   handle_sms},
};

String codeMessages[] =
{
  "Error: Contact not found",
  "Error: Server could not be reached",
  "WiFi not connected",
  "Success!"
};

int numOfCommands = sizeof(commandTable) / sizeof(Command);

void runCommand(String cmd)
{
  cmd.trim();
  if (cmd.length() == 0) return;

  int firstSpace = cmd.indexOf(' ');
  String commandName = (firstSpace == -1) ? cmd : cmd.substring(0, firstSpace);
  String arguments   = (firstSpace == -1) ? ""  : cmd.substring(firstSpace + 1);

  for (int i = 0; i < numOfCommands; i++)
  {
    if (commandName == commandTable[i].name)
    {
      commandTable[i].function(arguments);
      return;
    }
  }

  terminal.println("Unknown command: " + commandName, TFT_RED);
}

void handle_help(String args)
{
  terminal.println("Commands:");
  terminal.println("  help  clear  echo  quote");
  terminal.println("  wifi  sms");
}

void handle_clear(String args)
{
  terminal.clear();
}

void handle_echo(String args)
{
  terminal.println(args);
}

void handle_quote(String args)
{
  WiFiClient client;
  const char* quote_host = QUOTE_HOST;
  const uint16_t quote_port = 17;

  terminal.println("Connecting to " + String(quote_host), TFT_YELLOW);
  if (!client.connect(quote_host, quote_port))
  {
    terminal.println("Connection FAILED!", TFT_RED);
    delay(1000);
    return;
  }
  terminal.println("Connected!", TFT_GREEN);

  client.println("hello from ESP8266");

  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      terminal.println("Timeout. Disconnecting.", TFT_RED);
      client.stop();
      return;
    }
  }

  while (client.available())
    terminal.print((char)client.read());

  terminal.println();
  terminal.println("Connection closed.", TFT_YELLOW);
  client.stop();
}

void handle_wifi(String args)
{
  if (args == "scan")
  {
    terminal.println("Scanning...", TFT_YELLOW);
    WiFi.disconnect();
    delay(100);

    int numFound = WiFi.scanNetworks();
    terminal.print("Scan done. Found ");
    if (numFound > 0)
    {
      terminal.println(String(numFound));
      for (int i = 0; i < numFound; i++)
      {
        uint16_t signalColor = (WiFi.RSSI(i) > -60) ? TFT_GREEN :
                               (WiFi.RSSI(i) > -80) ? TFT_ORANGE : TFT_RED;

        terminal.print(String(i));
        terminal.print(": ");
        terminal.print(WiFi.SSID(i), signalColor);
        terminal.print(" (", signalColor);
        terminal.print(String(WiFi.RSSI(i)), signalColor);
        terminal.println("dBm)", signalColor);
        delay(10);
      }
    }
    else
    {
      terminal.println("none.");
    }
  }
  else if (args.startsWith("connect"))
  {
    String param = args.substring(8);
    int spaceIndex = param.indexOf(' ');
    if (spaceIndex < 0)
    {
      terminal.println("Usage: wifi connect <ssid> <password>", TFT_RED);
      return;
    }

    String ssid = param.substring(0, spaceIndex);
    String password = param.substring(spaceIndex + 1);

    terminal.print("Connecting to ", TFT_YELLOW);
    terminal.println(ssid, TFT_YELLOW);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long timeout = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      terminal.print('.');
      if (millis() - timeout > 8000)
      {
        terminal.println();
        terminal.println("Timeout: Could not connect!", TFT_RED);
        WiFi.disconnect();
        return;
      }
    }

    terminal.println();
    terminal.println("WiFi connected!", TFT_GREEN);
    terminal.print("IP: ", TFT_YELLOW);
    terminal.println(WiFi.localIP().toString(), TFT_YELLOW);
  }
  else if (args == "status")
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      terminal.print("SSID: ", TFT_YELLOW);
      terminal.println(WiFi.SSID());
      terminal.print("IP: ", TFT_YELLOW);
      terminal.println(WiFi.localIP().toString());

      int rssi = WiFi.RSSI();
      terminal.print("Strength: ", TFT_YELLOW);
      terminal.print(String(rssi));
      terminal.println(" dBm");

      int quality = map(rssi, -100, -50, 0, 100);
      quality = constrain(quality, 0, 100);
      terminal.print("Quality: ", TFT_YELLOW);
      terminal.print(String(quality));
      terminal.println("%");
    }
    else
    {
      terminal.println("WiFi not connected", TFT_YELLOW);
    }
  }
  else
  {
    terminal.println("Unknown Argument: '" + args + "'", TFT_RED);
  }
}

void handle_sms(String args)
{
  if (args.startsWith("send"))
  {
    String param = args.substring(5);
    int spaceIndex = param.indexOf(' ');

    if (spaceIndex < 0)
    {
      terminal.println("Usage: sms send <contact> <message>", TFT_RED);
      return;
    }

    String contact = param.substring(0, spaceIndex);
    String msg = param.substring(spaceIndex + 1);

    int code = sms.sendSMS(contact.c_str(), msg);

    uint16_t color = TFT_YELLOW;
    if (code < 0) color = TFT_RED;
    else if (code > 0) color = TFT_GREEN;

    terminal.println(codeMessages[code + 2], color);
  }
  else if (args.startsWith("read"))
  {
    String msg = comm.pollFromWeb("/get_for_arduino");

    if (msg == "ERROR") terminal.println("Error when reading", TFT_RED);
    else if (msg == "NONE") terminal.println("No new messages", TFT_YELLOW);
    else
    {
      terminal.println("Msgs received:", TFT_YELLOW);
      for (;;)
      {
        String contact_name = msg.substring(1, msg.indexOf(']'));
        String currentMsg = msg.substring(msg.indexOf(' ') + 1, msg.indexOf('[', 1) - 1);

        terminal.print("From: ", TFT_YELLOW);
        terminal.println(contact_name, TFT_YELLOW);
        terminal.println(currentMsg);

        if (msg.indexOf('\n') == -1) break;
        msg = msg.substring(msg.indexOf('\n') + 1);
      }
    }
  }
  else if (args.startsWith("chat"))
  {
    String contact = args.substring(5);
    contact.trim();
    contact.toUpperCase();

    if (contact.length() == 0)
    {
      terminal.println("Usage: sms chat <contact>", TFT_RED);
      return;
    }
    if (!sms.checkContact(contact.c_str()))
    {
      terminal.println("Error: Contact not found", TFT_RED);
      return;
    }

    mainState = 2;

    termInfo.reinit(0, 0, SCREEN_W, CHAR_H);
    termMessages.reinit(0, CHAR_H, SCREEN_W, SCREEN_H - 3 * CHAR_H);
    termSendBar.reinit(0, SCREEN_H - 2 * CHAR_H, SCREEN_W, 2 * CHAR_H);

    delete currentChat;
    currentChat = new Chat(contact.c_str(), &termInfo, &termMessages, &termSendBar);
  }
  else
  {
    terminal.println("Usage: sms [send|read|chat] ...", TFT_RED);
  }
}
