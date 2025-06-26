#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

void sendToWavelog(unsigned long qrg, String mode, String radio, String url, String token, String caCert) {
  String RequestData = "{\"key\":\"" + token + "\",\"radio\":\"" + radio + "\",\"frequency\":\"" + String(qrg) + "\",\"mode\":\"" + mode + "\"}";
  url.toLowerCase();

  WiFiClientSecure clientSecure;
  WiFiClient clientPlain;
  WiFiClient *client = nullptr;
  HTTPClient wl_request;
  int clientOK = 0;

  // HTTPS or not?
  if (url.startsWith("https://")) {
    // HTTPS
    logging("WL","Using HTTPS");
    clientSecure.setCACert(caCert.c_str());
    client = &clientSecure;
    clientOK = wl_request.begin(*client, url);
  } else if(url.startsWith("http://")) {
    // HTTP
    logging("WL","Using HTTP");
    client = &clientPlain;
    clientOK = wl_request.begin(*client, url);
  } else {
    logging("WL", "URL is missing the protocol (http/https): "+url);
    return;
  }

  if (clientOK) {
    // Prepare header for JSON and add the payload for Wavelog
    wl_request.addHeader("Content-Type", "application/json");

    int httpCode = wl_request.POST(RequestData);
    if (httpCode > 0) {
      logging("WL","POST... OK!");
    } else {
      logging("WL","POST... failed, error: "+wl_request.errorToString(httpCode));
    }

    // End session
    wl_request.end();
  } else {
    logging("WL","Unable to connect!");
  }

}
