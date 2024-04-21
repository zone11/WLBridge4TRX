#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <wavelog.h>

unsigned long wl_qrg = 0;
String wl_mode = "SSB";

unsigned long last_millis = 0;

void send() {
 WiFiClientSecure *client = new WiFiClientSecure;
    if(client) {
      client -> setCACert(wl_rootCACertificate);

      HTTPClient https;
  
      Serial.print("[HTTPS] Start...\n");
      if (https.begin(*client, wl_url)) {

        // Header for JSON and payload for wavelog
        https.addHeader("Content-Type", "application/json");
        String RequestData = "{\"key\":\"" + wl_token + "\",\"radio\":\"" + wl_radio + "\",\"frequency\":\"" + String(wl_qrg) + "\",\"mode\":\"" + wl_mode + "\"}";

        int httpCode = https.POST(RequestData);
        if (httpCode > 0) {
          Serial.printf("[HTTPS] POST... OK! Code: %d\n", httpCode);
        } else {
          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }

        // End session
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
      
      delete client;
    } else {
      Serial.println("Unable to create WifiClientSecure..");
    }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFiManager wm;
  
  bool res;
  res = wm.autoConnect("Yaesu2Wavelog");

  if(!res) {
      Serial.println("[WIFI] Failed to connect");
      ESP.restart();
  } 
  else {
      Serial.println("[WIFI] Connected! Let's talk to Wavelog :)");
  }
}

void loop() {
  if (millis() > (last_millis+5000)) {
    wl_qrg = random(1600000,433000000);
    Serial.print("[CAT] QRG: ");
    Serial.println(wl_qrg);

    Serial.print("[CAT] Mode: ");
    Serial.println(wl_mode);

    send();
   
    last_millis = millis();
  }
}
