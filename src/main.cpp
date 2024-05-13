#include <Arduino.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <wavelog.h>
#include <cat.h>

String cat_buffer = ";";
bool cat_get = false;
unsigned long cat_qrg = 0;
unsigned int cat_mode = 0;
unsigned long cat_qrg_last = 0;
unsigned int cat_mode_last = 0;


unsigned long wl_qrg = 0;
String wl_mode = "SSB";

unsigned long last_millis = 0;



bool catParseBuffer() {
  if (cat_buffer.length() > 1 ) {
    int semiFirst = cat_buffer.indexOf(";");
    int semiLast = cat_buffer.lastIndexOf(";");

    // MODE
    if((semiLast-semiFirst == 5)) {
      cat_mode = cat_buffer.substring(4,5).toInt();
      cat_buffer = ";";
      return true;
    }

    // QRG
    if((semiLast-semiFirst == 12)) {
      cat_qrg = cat_buffer.substring(3,12).toInt();
      cat_buffer = ";";
      return true;
    }
  }
  return false;
}

void sendToWavelog(boolean useSSL) {
 WiFiClientSecure *client = new WiFiClientSecure;
    if(client) {
      client -> setCACert(wl_rootCACertificate);

      HTTPClient https;
  
      Serial.print("[HTTPS] Start...\n");
      if (https.begin(*client, wl_url)) {

        // Prepare header for JSON and add the payload for Wavelog
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

void initWiFi() {
  // Init WifiManager
  WiFiManager wm;
  wm.setDebugOutput(false);

  // Start AP if last WLAN is unavilable, restart if no valid configuration is provided.
  if(!wm.autoConnect("Yaesu2Wavelog")) {
      Serial.println("[WIFI] Failed to connect, reboot!");
      ESP.restart();
      delay(1000);
  } 
  else {
      Serial.println("[WIFI] Connected! Let's talk to Wavelog :)");
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println();

  Serial2.begin(9600);
  initWiFi();
}

void loop() {
  if (millis() > (last_millis+250)) {
    // Send CAT command
    if(cat_get == false) {
      Serial2.print("FA;");
      cat_get = true;
    } else {
      Serial2.print("MD0;");
      cat_get = false;
    }

    // Parse CAT response 
    if (catParseBuffer()) {
      if ((cat_qrg != cat_qrg_last) || (cat_mode != cat_mode_last)) {
        Serial.print("[CAT] QRG: ");
        Serial.println(cat_qrg);

        Serial.print("[CAT] Mode: ");
        Serial.println(yaesuMode[cat_mode]);

        wl_qrg = cat_qrg;
        wl_mode = yaesuMode[cat_mode];

        sendToWavelog(true);

        cat_qrg_last = cat_qrg;
        cat_mode_last = cat_mode;
      } 
   }
    last_millis = millis();
  }

  if (Serial2.available()) {
    cat_buffer += Serial2.readString();
  }

 

}
