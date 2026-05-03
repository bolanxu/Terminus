#ifndef CHAT_H
#define CHAT_H

#include <Arduino.h>
#include "settings.h"
#include <TFT_eSPI.h>

class Terminal;

class Chat
{
  private:
    Terminal* _info;
    Terminal* _messages;
    Terminal* _sendBar;

    String _inputText;
    String _currentContact;

    bool _pendingQuit = false;

    void updateInfo(const String& status, uint16_t color = TFT_CYAN);

  public:
    Chat(const char* contact, Terminal* info, Terminal* messages, Terminal* sendBar);
    ~Chat();

    void displayHistory(const char* contact);
    void updateKey(char c);
    void updateMsgs();

    bool pendingQuit() { return _pendingQuit; }
};

#endif
