#include "chat.h"
#include "terminal.h"
#include "globals.h"
#include "sms.h"
#include "memory.h"

Chat::Chat(const char* contact, Terminal* info, Terminal* messages, Terminal* sendBar)
{
  _info = info;
  _messages = messages;
  _sendBar = sendBar;

  _currentContact = String(contact);

  _info->clear();
  _messages->clear();
  _sendBar->clear();

  _info->print(_currentContact, TFT_GREEN);
  _sendBar->print(PROMPT);

  displayHistory(_currentContact.c_str());
}

Chat::~Chat()
{
}

void Chat::updateInfo(const String& status, uint16_t color)
{
  
  _info->clear();
  _info->print(_currentContact, TFT_CYAN);

  Serial.print(_currentContact);

  int usedCols = _currentContact.length();
  int statusStart = MAX_COLS - status.length();
  for (int i = usedCols; i < statusStart; i++)
  {
    _info->print(' ');
    Serial.print(' ');
  }

  _info->print(status, color);
  Serial.println(status);
}

void Chat::displayHistory(const char* contact)
{
  if (data.openFile(String(contact) + ".txt", "r") == -1) return;

  _messages->setAutoRedraw(false);

  while (1)
  {
    String currLine = data.readNextLine();
    if (currLine.length() == 0) break;

    int sep = currLine.indexOf(':');
    if (sep == -1)
    {
      _messages->println(currLine);
      continue;
    }
    if (currLine.substring(0, sep + 1) == "ME:")
      _messages->print(currLine.substring(0, sep + 1));
    else
      _messages->print(currLine.substring(0, sep + 1), TFT_YELLOW);
    _messages->print(" ", TFT_YELLOW);
    _messages->println(currLine.substring(sep + 1), TFT_YELLOW);
  }

  _messages->setAutoRedraw(true);
  _messages->redraw();

  data.closeFile();
}

void Chat::updateKey(char c)
{
  if (c == '\n' || c == '\r')
  {
    if (_inputText.length() > 0)
    {
      if (_inputText == "quit")
      {
        _inputText = "";
        _pendingQuit = true;
        return;
      }

      String textToSend = _inputText;
      _inputText = "";
      _sendBar->clear();
      _sendBar->print(PROMPT);

      bool sent = false;
      for (int attempt = 1; attempt <= 3; attempt++)
      {
        updateInfo("try " + String(attempt) + "/3", TFT_YELLOW);

        int result = sms.sendSMS(_currentContact.c_str(), textToSend);
        if (result > 0)
        {
          updateInfo("sent", TFT_GREEN);
          _messages->print("ME: ");
          _messages->println(textToSend);
          sent = true;
          break;
        }

        if (attempt < 3)
          delay(1000);
      }

      if (!sent)
        updateInfo("failed", TFT_RED);
    }

    //_inputText = "";
    //_sendBar->clear();
    //_sendBar->print(PROMPT);
  }
  else if (c == '\b' || c == 127)
  {
    if (_inputText.length() > 0)
    {
      _inputText.remove(_inputText.length() - 1);
      _sendBar->print('\b');
    }
  }
  else
  {
    _inputText += c;
    _sendBar->print(c);
  }
}

void Chat::updateMsgs()
{
  String msg = sms.readSMS();

  if (msg != "ERROR" && msg.length() > 0 && msg != "NO_MSG")
  {
    for (;;)
    {
      String contact = msg.substring(1, msg.indexOf(']'));
      String currentMsg = msg.substring(msg.indexOf(' ') + 1, msg.indexOf('[', 1) - 1);

      _messages->print(contact + ":", TFT_YELLOW);
      _messages->print(" ");
      _messages->println(currentMsg, TFT_YELLOW);

      if (msg.indexOf('\n') == -1) break;
      msg = msg.substring(msg.indexOf('\n') + 1);
    }
  }
}
