#include <Arduino.h>
#include <HardwareSerial.h>
#include <functional>

class Radio {
  private:
    /** Reference to the serial interface used for CAT communication. */
    HardwareSerial& serial;
    /** Baud rate used for CAT communication. */
    uint32_t baudRate;

    /** CAT command to request frequency. */
    String catFreqCommand;
    /** CAT command to request mode. */
    String catModeCommand;
    /** Start position of the frequency value in the CAT response. */
    int catFreqPosition;
    /** Start position of the mode value in the CAT response. */
    int catModePosition;
    /** Length of the frequency value in the CAT response. */
    int catFreqLength;
    /** Length of the mode value in the CAT response. */
    int catModeLength;

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

            // Check for frequency response
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

            // Check for mode response
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

        // Notify if frequency or mode changed
        if (changed) {
            currentFrequency = newFrequency;
            currentMode = newMode;
            if (onStateChanged) {
                onStateChanged(currentFrequency, currentMode);
            }
        }
    }

    // Poll CAT interface and update internal state
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
