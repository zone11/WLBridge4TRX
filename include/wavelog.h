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

    void init(String url, String token) {
      this->url = url;
      this->token = token;
      this->cert = "";
      this->version = "";
    }

    void init(String url, String token, String cert) {
      this->url = url;
      this->token = token;
      this->cert = cert;
      this->version = "";
    }

    boolean sendQRG(String radio, String mode, unsigned long qrg) {
      String RequestData = "\"radio\":\"" + radio + "\",\"frequency\":\"" + String(qrg) + "\",\"mode\":\"" + mode + "\"}";
      if (callAPI(RequestData)) {
        logging("WL", "API Call OK");
        return true;
      } else {
        logging("WL","API Call Error");
        return false;
      }
    }

    bool callAPI(String request) {
      String RequestData = "{\"key\":\"" + this->token + "\"," + request + "}";
      this->url.toLowerCase();

      WiFiClientSecure *client_secure = new WiFiClientSecure;
      WiFiClient *client = new WiFiClient(); 
      HTTPClient wl_request;
      bool clientOK;
      bool clientSuccess = false;

      // HTTPS or not?
      if ((this->url.startsWith("https://")) && this->cert != "") {
        // HTTPS
        logging("WL","Using HTTPS");
        client_secure -> setCACert(this->cert.c_str());
        clientOK = wl_request.begin(*client_secure, this->url);
      } else if(this-url.startsWith("http://")) {
        // HTTP
        logging("WL","Using HTTP");
        clientOK = wl_request.begin(*client, this->url);
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
      
      // Cleanup Memory
      delete client;
      delete client_secure;

      return clientSuccess;
    }
    
    void getVersion() {

    }
};