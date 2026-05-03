#include "sms.h"
#include "globals.h"
#include "memory.h"

String SMS::getElement(int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = _contacts.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (_contacts.charAt(i) == '\n' || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? _contacts.substring(strIndex[0], strIndex[1]) : "";
}


void SMS::updateContacts() {
  _contacts = comm.pollFromWeb("/get_contacts");
  _numOfContacts = 0;
  if (_contacts.length() == 0)
    return;

  // Simple count of newlines
  for (int i = 0; i < _contacts.length(); i++) {
    if (_contacts.charAt(i) == '\n')
      _numOfContacts++;
  }
  _numOfContacts++; // Add 1 for the last element
}


bool SMS::checkContact(const char* name) {
  for (int i = 0; i < _numOfContacts; i++) {
    if (getElement(i) == String(name)) {
      return true;
    }
  }
  return false;
}


int SMS::sendSMS(const char* contact, const String& msg)
{
  String contactStr = String(contact);
  contactStr.toLowerCase();
  
  int postResult = comm.postToWeb("/" + contactStr + "/post_from_arduino", msg);
  if (postResult <= 0)
    return postResult;

  addMessageOut(contact, msg);
  return 1;
}

String SMS::readSMS()
{
  String msg = comm.pollFromWeb("/get_for_arduino");

  if (msg != "ERROR" && msg.length() > 0 && msg != "NO_MSG")
  {
    for (;;)
    {
      String contact = msg.substring(1, msg.indexOf(']'));
      String currentMsg = msg.substring(msg.indexOf(' ') + 1, msg.indexOf('[', 1) - 1);

      addMessageIn(contact.c_str(), currentMsg);

      if (msg.indexOf('\n') == -1) break;
      msg = msg.substring(msg.indexOf('\n') + 1);
    }
  }

  return msg;
}

int SMS::addMessageOut(const char* contact, String msg)
{
  msg.trim();
  return data.addToFile(String(contact) + ".txt", "ME:" + msg + "\n");
}

int SMS::addMessageIn(const char* contact, String msg)
{
  msg.trim();
  return data.addToFile(String(contact) + ".txt", String(contact) + ":" + msg + "\n");
}
