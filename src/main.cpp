#include <Arduino.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Effortless_SPIFFS.h>
#include <ESPmDNS.h>
#include <logging.h>
#include <wavelog.h>
#include <cat.h>

// Configuration values 
String wl_url ="";
String wl_token = "";
String wl_radio = "";
String wl_rootCACertificate = "";

// Variables for daily use
String cat_buffer = ";";
bool cat_get = false;
unsigned long cat_qrg = 0;
unsigned int cat_mode = 0;
unsigned long cat_qrg_last = 0;
unsigned int cat_mode_last = 0;

unsigned long wl_qrg = 0;
String wl_mode = "SSB";
unsigned long last_millis = 0;

// Instances for eSPIFFS (Settings) and the webserver
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
  if(!wm.autoConnect("WLBridge4TRX")) {
      logging("WIFI","Failed to connect, reboot!");
      ESP.restart();
  } 
  else {
      logging("WIFI","Connected! Let's talk to Wavelog :)");
  }
}

boolean savePreferences(String url, String token, String radio, String caCert) {
  String theSettingsRAW;
  JsonDocument theSettings;

  theSettings["url"] = url;
  theSettings["token"] = token;
  theSettings["radio"] = radio;
  theSettings["caCert"] = caCert;
  serializeJson(theSettings,theSettingsRAW);

  if (fileSystem.saveFile("/WLBridge4TRX.cfg", theSettingsRAW.c_str())) {
    logging("eSPIFFS","File written!");
    return true;
  } else {
    logging("eSPIFFS","Error writing file!");
    return false;
  }
}

boolean readPreferences() {
  String theSettingsRAW;
  JsonDocument theSettings;

  if (fileSystem.openFromFile("/WLBridge4TRX.cfg", theSettingsRAW)) {
    logging("eSPIFFS", "Configfile found");
    if (!deserializeJson(theSettings, theSettingsRAW)) {
      String read_wl_url = theSettings["url"];
      String read_wl_token = theSettings["token"];
      String read_wl_radio = theSettings["radio"];
      String read_wl_rootCACertificate = theSettings["caCert"];

      wl_url = read_wl_url;
      wl_token = read_wl_token;
      wl_radio = read_wl_radio;
      wl_rootCACertificate = read_wl_rootCACertificate;
      
      String wl_rootCACertificateDisplay = read_wl_rootCACertificate;
      wl_rootCACertificateDisplay.replace("\n","\n\r");

      logging("Prefs", "URL: "+wl_url);
      logging("Prefs", "Token: "+wl_token);
      logging("Prefs", "Radio: "+wl_radio);
      logging("Prefs", "RootCA Cert: "+wl_rootCACertificateDisplay);
      
      return true;
    } else {
      logging("eSPIFFS","Deserialize JSON failed");
      return false;
    }
  } else {
    logging("eSPIFFS", "no configuration found, lets create one with defaults and reboot");
    if (savePreferences(default_wl_url, default_wl_token, default_wl_radio, default_wl_rootCACertificate) == true) {
      delay(2000);
      ESP.restart();
      return true;
    } else {
      logging("eSPIFFS", "Error creating a default configuration - Check SPIFFS parameters");
      return false;
    }
  }
}

void webSiteHome() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>WLBridge4TRX</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; }";
  html += ".main { max-width: 500px; margin: 0 auto; padding: 20px; }";
  html += "input[type='text'] { width: 100%; padding: 10px; margin: 5px 0; }";
  html += "input[type='submit'] { width: 100%; padding: 10px; margin-top: 10px; background-color: #4CAF50; color: white; border: none; }";
  html += "input[type='submit']:hover { background-color: #45a049; }";
  html += "textare { width: 100%; padding: 10px; margin: 5px 0; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"main\">";
  html += "<h1>WLBridge4TRX-Configuration</h1>";
  html += "<form action='/update' method='post'>";
  html += "<label for='wl_URL'>Wavelog URL:</label><br>";
  html += "<input type='text' id='wl_URL' name='wl_URL' value='"+wl_url+"'><br>";
  html += "<label for='wl_Token'>Wavelog Token:</label><br>";
  html += "<input type='text' id='wl_Token' name='wl_Token' value='"+wl_token+"'><br>";
  html += "<label for='wl_Radio'>Wavelog Radio Name:</label><br>";
  html += "<input type='text' id='wl_Radio' name='wl_Radio' value='"+wl_radio+"'><br>";
  html += "<label for='wl_rootCACertificate'>Wavelog CA Certificate</label><br>";
  html += "<textarea id='wl_rootCACertificate' name='wl_rootCACertificate' rows='40' cols='80' >"+wl_rootCACertificate+"'</textarea><br>";
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
  html += "<title>WLBridge4TRX</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; }";
  html += ".main { max-width: 400px; margin: 0 auto; padding: 20px; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"main\">";
  html += "<h1>WLBridge4TRX-Configuration</h1>";
  html += "Settings saved, rebooting...";
  html += "</div>";
  html += "</body>";
  html += "</html>";

  wl_url = server.arg("wl_URL");
  wl_token = server.arg("wl_Token");
  wl_radio = server.arg("wl_Radio");
  wl_rootCACertificate = server.arg("wl_rootCACertificate");

  if (savePreferences(wl_url, wl_token, wl_radio, wl_rootCACertificate)) {
    server.send(200, "text/html", html);
    logging("HTTP","Saving config sucessfull, rebooting");
    ESP.restart();
  } else {
    logging("HTTP", "Saving config failed!");
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
    logging("eSPIFFS","reading configuration failed, check details above");
  } else {
    logging("eSPIFFS","reading settings went fine");
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
  if (!MDNS.begin("WLBridge4TRX")) {
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
