#include <Arduino.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Effortless_SPIFFS.h>
#include <ESPmDNS.h>
#include <logging.h>
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

String wl_url = "https://wavelog.hb9hjq.ch/api/radio";
String wl_token = "wl65d74436bed67";
String wl_radio = "WLBridge4TRX";
String wl_rootCACertificate = default_wl_rootCACertificate;

unsigned long last_millis = 0;

eSPIFFS fileSystem;
WebServer server(80);


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

void initWiFi() {
  // Init WifiManager
  WiFiManager wm;
  wm.setDebugOutput(false);

  // Start AP if last WLAN is unavilable, restart if no valid configuration is provided.
  if(!wm.autoConnect("WLBridget4TRX")) {
      logging("WIFI","Failed to connect, reboot!");
      ESP.restart();
  } 
  else {
      logging("WIFI","Connected! Let's talk to Wavelog :)");
  }
}

void webSiteHome() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>WLBridget4TRX</title>";
  // html += "<style>";
  // html += "body { font-family: Arial, sans-serif; }";
  // html += ".container { max-width: 400px; margin: 0 auto; padding: 20px; }";
  // html += "input[type='text'] { width: 100%; padding: 10px; margin: 5px 0; }";
  // html += "input[type='submit'] { width: 100%; padding: 10px; margin-top: 10px; background-color: #4CAF50; color: white; border: none; }";
  // html += ".btn-reboot { width: 100%; padding: 10px; margin-top: 10px; background-color: #FF0000; color: white; border: none; }";
  // html += "input[type='submit']:hover { background-color: #45a049; }";
  // html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"container\">";
  html += "<h1>WLBridget4TRX-Configuration</h1>";
  html += "<form action='/update' method='post'>";
  html += "<label for='wl_URL'>Wavelog URL:</label><br>";
  html += "<input type='text' id='wl_URL' name='wl_URL' value='"+wl_url+"'><br>";
  html += "<label for='wl_Token'>Wavelog Token:</label><br>";
  html += "<input type='text' id='wl_Token' name='wl_Token' value='"+wl_token+"'><br>";
  html += "<label for='wl_Radio'>Wavelog Radio Name:</label><br>";
  html += "<input type='text' id='wl_Radio' name='wl_Radio' value='"+wl_radio+"'><br>";
  html += "<label for='wl_rootCACertificate'>Wavelog CA Certificate</label><br>";
  html += "<input type='textarea' id='wl_rootCACertificate' name='wl_rootCACertificate' value='"+wl_rootCACertificate+"'><br>";
  html += "<input type='submit' value='Update'>";
  html += "</form>";
  html += "</div>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

void webSiteUpdate() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>WLBridget4TRX</title>";
  // html += "<style>";
  // html += "body { font-family: Arial, sans-serif; }";
  // html += ".container { max-width: 400px; margin: 0 auto; padding: 20px; }";
  // html += "input[type='text'] { width: 100%; padding: 10px; margin: 5px 0; }";
  // html += "input[type='submit'] { width: 100%; padding: 10px; margin-top: 10px; background-color: #4CAF50; color: white; border: none; }";
  // html += ".btn-reboot { width: 100%; padding: 10px; margin-top: 10px; background-color: #FF0000; color: white; border: none; }";
  // html += "input[type='submit']:hover { background-color: #45a049; }";
  // html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"container\">";
  html += "<h1>WLBridget4TRX-Configuration</h1>";
  html += "Settings saved, rebooting...";
  html += "</div>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
  ESP.restart();
}

boolean savePreferences(String url, String token, String radio, String caCert) {
  String theSettingsRAW = "";
  JsonDocument theSettings;

  theSettings["url"] = url;
  theSettings["token"] = token;
  theSettings["radio"] = radio;
  theSettings["caCert"] = caCert;
  serializeJsonPretty(theSettings,Serial);
  return true;
}

boolean readPreferences() {
  String theSettingsRAW = "";
  JsonDocument theSettings;

  if (fileSystem.openFromFile("/WLBridget4TRX.cfg", theSettingsRAW)) {
    logging("eSPIFFS", "Configfile found");
    deserializeJson(theSettings, theSettingsRAW);

    return true;
  } else {
    logging("eSPIFFS", "no configuration found, lets create one with defaults");
    if (savePreferences(default_wl_url, default_wl_token, default_wl_radio, default_wl_rootCACertificate) == true) {
      return true;
    } else {
      logging("eSPIFFS", "Error creating a default configuration - Check SPIFFS parameters");
      return false;
    }
  }
}

void setup() {
  // Init Serial (Debug) and Serial2 (CAT)
  Serial.begin(115200);
  Serial2.begin(9600);

  // Lets go!
  delay(2000);
  Serial.println();
  logging("Main","Lets start!");
  
  // Check SPIFFS
  if (!SPIFFS.begin(true)) {
    logging("SPIFFS","initialization failed!");
    return;
  }

  // Read Wavelog settings from SPIFFS 
  if (!readPreferences()) {
    logging("eSPIFFS","reading settings failed, check details above");
  } else {
      logging("SPIFFS","reading settings went fine");
  }

  // Lets start WIFI and the manager if required
  initWiFi();

  // Spit out some Network informations


  // Start local web server
  server.on("/", webSiteHome);
  server.on("/update", webSiteUpdate);
  server.begin();
  logging("HTTP","Service started");

  // Announce HTTP of this device using mDNS
  if (!MDNS.begin("WLBridget4TRX")) {
    logging("MDNS","Service failed!");
  } else {
    logging("MDNS","Service started");
  }
  MDNS.addService("http", "tcp", 80);
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

    // Parse CAT response and send to Wavelog
    if (catParseBuffer()) {
      if ((cat_qrg != cat_qrg_last) || (cat_mode != cat_mode_last)) {
        wl_qrg = cat_qrg;
        wl_mode = yaesuMode[cat_mode];

        logging("CAT", "QRG: "+wl_qrg);
        logging("CAT", "Mode: "+wl_mode);
        sendToWavelog(wl_qrg, wl_mode, wl_radio, wl_token, wl_url, wl_rootCACertificate);

        cat_qrg_last = cat_qrg;
        cat_mode_last = cat_mode;
      } 
   }
    last_millis = millis();
  }

  if (Serial2.available()) {
    cat_buffer += Serial2.readString();
  }

  server.handleClient();
}
