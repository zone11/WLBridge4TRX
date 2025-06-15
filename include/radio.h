#include <Arduino.h>
#include <HardwareSerial.h>
#include <functional>

class Radio {
  private:
    HardwareSerial& serial; // Reference to the HardwareSerial object used for CAT communication
    uint32_t baudRate; // Baud rate used for CAT communication

    String catFreqCommand;  // Command string to request frequency
    String catModeCommand;   // Command string to request mode
    int catFreqPosition;  // Position in the CAT response where frequency starts
    int catModePosition;   // Position in the CAT response where mode starts
    int catFreqLength;  // Length of the frequency value in the CAT response
    int catModeLength;   // Length of the mode value in the CAT response

    int currentFrequency = 0;
    int currentMode = 0;

    std::function<void(int, int)> onStateChanged = nullptr;

  public:    
    Radio(HardwareSerial& serial, uint32_t baudRate, 
          String catFreqCommand, String catModeCommand, int catFreqPosition, int catModePosition, 
          int catFreqLength, int catModeLength) 
      : serial(serial), baudRate(baudRate), 
        catFreqCommand(catFreqCommand), catModeCommand(catModeCommand), 
        catFreqPosition(catFreqPosition), catModePosition(catModePosition), 
        catFreqLength(catFreqLength), catModeLength(catModeLength) {
    }

    void begin() {
        serial.begin(baudRate);
    }

    void requestCatStatus() {
        // logging("Radio", "Sending: " + catFreqCommand + ";" + catModeCommand + ";");
        serial.print(catFreqCommand + ";" + catModeCommand + ";");
    }

    void processCatResponse() {
        static char buffer[256];
        static size_t bufferLen = 0;

        // Read all available characters into the buffer
        while (serial.available() && bufferLen < sizeof(buffer) - 1) {
            char c = serial.read();
            buffer[bufferLen++] = c;
        }
        buffer[bufferLen] = '\0'; // Null terminator for safety

        int newFrequency = currentFrequency;
        int newMode = currentMode;
        bool changed = false;

        // Process all complete CAT commands in the buffer
        bool foundSomething;
        do {
            foundSomething = false;

            // Look for frequency command
            String freqCmdSearch = ";" + catFreqCommand;
            char* qrgPos = strstr(buffer, freqCmdSearch.c_str());
            if (qrgPos && (size_t)(qrgPos - buffer + 1 + catFreqCommand.length() + catFreqLength) <= bufferLen) {
                int start = (qrgPos - buffer) + 1 + catFreqCommand.length();
                char temp[16] = {0};
                strncpy(temp, &buffer[start], catFreqLength);
                int parsedFreq = atoi(temp);
                if (parsedFreq != currentFrequency) {
                    newFrequency = parsedFreq;
                    changed = true;
                }

                size_t consumed = start + catFreqLength;
                memmove(buffer, buffer + consumed, bufferLen - consumed);
                bufferLen -= consumed;
                buffer[bufferLen] = '\0';
                foundSomething = true;
                continue;
            }

            // Look for mode command
            String modeCmdSearch = ";" + catModeCommand;
            char* mdPos = strstr(buffer, modeCmdSearch.c_str());
            if (mdPos && (size_t)(mdPos - buffer + 1 + catModeCommand.length() + catModeLength) <= bufferLen) {
                int start = (mdPos - buffer) + 1 + catModeCommand.length();
                char temp[8] = {0};
                strncpy(temp, &buffer[start], catModeLength);
                int parsedMode = atoi(temp);
                if (parsedMode != currentMode) {
                    newMode = parsedMode;
                    changed = true;
                }

                size_t consumed = start + catModeLength;
                memmove(buffer, buffer + consumed, bufferLen - consumed);
                bufferLen -= consumed;
                buffer[bufferLen] = '\0';
                foundSomething = true;
                continue;
            }

        } while (foundSomething);

        if (changed) {
            currentFrequency = newFrequency;
            currentMode = newMode;
            if (onStateChanged) {
                onStateChanged(currentFrequency, currentMode);
            }
        }
    }

    void pollCatInterface() {
        static bool toggle = false;
        processCatResponse();
        if (toggle) {
            requestCatStatus();
        }
        toggle = !toggle;
    }

    int getCurrentFrequency() const {
        return currentFrequency;
    }

    int getCurrentMode() const {
        return currentMode;
    }

    void setOnStateChanged(std::function<void(int, int)> callback) {
        onStateChanged = callback;
    }
};
