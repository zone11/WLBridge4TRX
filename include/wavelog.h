#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

class Wavelog {
  private:
    String url;
    String token;
    String cert;
    String version;

  public:    
    Wavelog() {
      this->url = "";
      this->token = "";
      this->cert = "";
      this->version = "";
    }

    bool init(String url, String token, String cert) {
      this->url = url;
      this->token = token;
      this->cert = cert;

      // Check if the version is 2.0 or higher
      this->getOnlineVersion();
      if (this->version.toInt() >= 2) {
        logging("WL","Init OK");
        return true;        
      } else {
        logging("WL","Init failed: Wavelog 2.0 or higher required");
        return false;
      } 
    }

    bool callAPI(String endpoint, String request) {
      logging("WL", "Free heap before API: " + String(ESP.getFreeHeap()));
      String RequestData = "{\"key\":\"" + this->token + "\"," + request + "}";
      this->url.toLowerCase();

      WiFiClientSecure client_secure;
      WiFiClient client; 
      HTTPClient wl_request;
      int clientOK;
      bool clientSuccess = false;

      logging("WL", "Data: "+RequestData);
      // HTTPS or not?
      if ((this->url.startsWith("https://")) && this->cert != "") {
        // HTTPS
        logging("WL","Using HTTPS");
        client_secure.setCACert(this->cert.c_str());
        clientOK = wl_request.begin(client_secure, this->url);
      } else if(this->url.startsWith("http://")) {
        // HTTP
        logging("WL","Using HTTP");
        clientOK = wl_request.begin(client, this->url);
      } else {
        logging("WL", "URL is missing the protocol (http/https): "+this->url);
      }      

      if (clientOK) {
        // Prepare header for JSON and add the payload for Wavelog
        wl_request.addHeader("Content-Type", "application/json");

        // Send request
        int httpCode = wl_request.POST(RequestData);

        // Error checking
        if (httpCode > 0) {
          logging("WL","POST... OK!");
          clientSuccess = true;
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
      if (callAPI("radio", RequestData)) {
        logging("WL", "API Call OK");
        return true;
      } else {
        logging("WL","API Call Error");
        return false;
      }
    }

    boolean getOnlineVersion() {
      if (callAPI("version", "") == false) {
        logging("WL","API Call Error");
        return false;
      } else {
        // Parse version from JSON - DUMMY
        this->version = "2.0.3"; // TODO: Parse version from JSON response
        logging("WL","Version: "+this->version);
        return true;
      }
    }
    
    String getVersion() {
      return this->version;
    }
};