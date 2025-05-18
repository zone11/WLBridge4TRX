#include <Arduino.h>

class Radio {
  private:
    String catQrg;  // Command for frequency
    String catMd;   // Command for mode
    int catQrgPos;  // Position in response for frequency
    int catMdPos;   // Position in reponse for mode
    int catQrgLen;  // Length of response for frequency
    int catMdLen;   // Length of response for mode

  public:    
    Radio() {
    }

};
