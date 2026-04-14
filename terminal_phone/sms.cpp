#include "sms.h"
#include "globals.h"
#include "memory.h"

void SMS::updateContacts()
{
  _contacts = comm.pollFromWeb("/get_contacts");
  Serial.println(_contacts);
}

bool SMS::checkContact(const char* name)
{
  int lastPos = 0;
  int commaIndex = _contacts.indexOf('\n');
  String element;

  //Serial.println(name);
  
  while (commaIndex != -1) {
    element = _contacts.substring(lastPos, commaIndex);
    //Serial.println(element);
    if (element == String(name))
      return true;
    
    lastPos = commaIndex + 1;
    commaIndex = _contacts.indexOf('\n', lastPos);
  }
  
  element = _contacts.substring(lastPos);
  //Serial.println(element);
  if (element == String(name))
      return true;
  
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
