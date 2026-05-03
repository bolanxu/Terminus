#ifndef SMS_H
#define SMS_H

#include <Arduino.h>
#include "settings.h"
#include "communication.h"

/*
struct Contact
{
  const char* name;
  const char* phoneNum;
};
*/

class SMS
{
  private:
    //const Contact _contactTable[2] =
    //{
    //  {"bohan", "9999999999"},
    //  {"dad",      "2222222222"},
    //};
    int _numOfContacts = 0;

    String _contacts;

  public:
    SMS() {}

    String getElement(int index);
    int getContactLen() { return _numOfContacts; }
    void updateContacts();
    bool checkContact(const char* name);
    String getContacts() { return _contacts; }

    int sendSMS(const char* contact, const String& msg);
    String readSMS();

    int addMessageOut(const char* phoneNum, String msg);
    int addMessageIn(const char* contact, String msg);
};

#endif
