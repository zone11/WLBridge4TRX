#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

class Wavelog {
  private:
    String url;
    String token;
    String cert;
    String version;
    bool useHttps;

  public:    
    Wavelog() {
      this->url = "";
      this->token = "";
      this->cert = "";
      this->version = "";
      this->useHttps = false;
    }

    bool init(String url, String token, String cert) {
      this->url = url;
      this->token = token;
      this->cert = cert;
      this->useHttps = url.startsWith("https://");

      // Check if the version is 2.0 or higher
      this->getVersion();
      if (this->version.toInt() >= 2) {
        logging("WL","Init OK");
        return true;        
      } else {
        logging("WL","Init failed: Wavelog 2.0 or higher required");
        return false;
      } 
    }

    bool callAPI(String endpoint, String request, JsonDocument &responseDoc) {
      //logging("WL", "Free heap before API: " + String(ESP.getFreeHeap()));
      String RequestData;
      if (request.length() > 0) {
        RequestData = "{\"key\":\"" + this->token + "\"," + request + "}";
      } else {
        RequestData = "{\"key\":\"" + this->token + "\"}";
      }
      this->url.toLowerCase();

      WiFiClientSecure client_secure;
      WiFiClient client; 
      HTTPClient wl_request;
      int clientOK;
      bool clientSuccess = false;

      //logging("WL", "Data: "+RequestData);
      //logging("WL", "Endpoint: "+endpoint);


      // HTTPS or not?
      if (this->useHttps && this->cert != "") {
        logging("WL","Using HTTPS");
        client_secure.setCACert(this->cert.c_str());
        clientOK = wl_request.begin(client_secure, this->url+"/" + endpoint);
      } else {
        logging("WL","Using HTTP");
        clientOK = wl_request.begin(client, this->url+"/" + endpoint);
      }

      if (clientOK) {
        // Prepare header for JSON and add the payload for Wavelog
        wl_request.addHeader("Content-Type", "application/json");

        // Send request
        int httpCode = wl_request.POST(RequestData);

        // Error checking
        if (httpCode > 0) {
          logging("WL","POST... OK!");
          String payload = wl_request.getString();
          logging("WL","Response: " + payload);
          DeserializationError error = deserializeJson(responseDoc, payload);
          if (error) {
            logging("WL", "Failed to parse JSON response");
            clientSuccess = false;
          } else {
            clientSuccess = true;
          }
        } else {
          logging("WL","POST... failed, error: "+wl_request.errorToString(httpCode));
        }

        // End session
        wl_request.end();
      } else {
        logging("WL","Unable to connect!");
      }
      
      return clientSuccess;
    }

    boolean sendQRG(String radio, String mode, unsigned long qrg) {
      String RequestData = "\"radio\":\"" + radio + "\",\"frequency\":\"" + String(qrg) + "\",\"mode\":\"" + mode + "\"";
      DynamicJsonDocument doc(256);
      if (callAPI("radio", RequestData, doc)) {
        logging("WL", "API Call OK");
        return true;
      } else {
        logging("WL","API Call Error");
        return false;
      }
    }

    boolean getVersion() {
      DynamicJsonDocument doc(256);
      if (callAPI("version", "", doc) == false) {
        logging("WL","API Call Error");
        return false;
      } else {
        // Parse version from JSON response
        if (doc["version"].is<String>()) {
          this->version = doc["version"].as<String>();
          logging("WL","Version: " + this->version);
        }
        return true;
      }
    }
};