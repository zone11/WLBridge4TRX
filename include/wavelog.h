#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

void sendToWavelog(unsigned long qrg, String mode, String radio, String url, String token, String caCert) {
  String RequestData = "{\"key\":\"" + token + "\",\"radio\":\"" + radio + "\",\"frequency\":\"" + String(qrg) + "\",\"mode\":\"" + mode + "\"}";

  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client -> setCACert(caCert.c_str());

    HTTPClient https;
  
    logging("HTTPS", "Start...\n");
    if (https.begin(*client, url)) {

      // Prepare header for JSON and add the payload for Wavelog
      https.addHeader("Content-Type", "application/json");

      int httpCode = https.POST(RequestData);
      if (httpCode > 0) {
        logging("[HTTPS]","POST... OK!");
      } else {
        logging("[HTTPS]","POST... failed, error: "+https.errorToString(httpCode));
      }

        // End session
      https.end();
    } else {
      logging("[HTTPS]","Unable to connect!");
    }
      
    delete client;
  } else {
    logging("HTTPS","Unable to create WifiClientSecure!");
  }
}