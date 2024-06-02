#include <Arduino.h>
#include <config.h>

void logging(String service, String message) {
    #if LOGGING_SERIAL == 1
      Serial.println("["+service+"] "+message);
    #endif
}