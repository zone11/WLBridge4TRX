#include <Arduino.h>

#define LOGGING

void logging(String service, String message) {
    #ifdef LOGGING
      Serial.println("["+service+"] "+message);
    #endif
}