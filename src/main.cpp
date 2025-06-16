#include <Arduino.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Effortless_SPIFFS.h>
#include <ESPmDNS.h>
#include <display.h>
#include <logging.h>
#include <wavelog.h>
#include <radio.h>
#include <globals.h>

String netIP = "";
bool netOnline = false;

// Objects
eSPIFFS fileSystem;
WebServer server(80);
Wavelog wl;
Radio trx(Serial2, 9600, "FA", "MD", 2, 3, 11, 1);


// CAT Modes
String catModes[] = {"ERR", "LSB", "USB", "CW-U","FM","AM","DATA","CW-REV","DATA-REV"}; // ELECRAFT


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
    netOnline = true;
    netIP = WiFi.localIP().toString();
    logging("WIFI","Connected! IP: "+netIP);
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

      gWavelogURL = read_wl_url;
      gWavelogToken = read_wl_token;
      gWavelogRadio = read_wl_radio;
      gWavelogCertificate = read_wl_rootCACertificate;
      
      String wl_rootCACertificateDisplay = read_wl_rootCACertificate;
      wl_rootCACertificateDisplay.replace("\n","\n\r");

      logging("Prefs", "URL: "+gWavelogURL);
      logging("Prefs", "Token: "+gWavelogToken);
      logging("Prefs", "Radio: "+gWavelogRadio);
      logging("Prefs", "RootCA Cert: "+gWavelogCertificate);
      
      return true;
    } else {
      logging("eSPIFFS","Deserialize JSON failed");
      return false;
    }
  } else {
    logging("eSPIFFS", "no configuration found, lets create one with defaults and reboot");
    if (savePreferences(default_wl_url, default_wl_token, default_wl_radio, default_wl_rootCACertificate) == true) {
      delay(1000);
      ESP.restart();
      return true;
    } else {
      logging("eSPIFFS", "Error creating a default configuration - Check SPIFFS parameters");
      return false;
    }
  }
}

void webSiteHome() {
  String html = "<!DOCTYPE html>\n";
  html += "<html>\n";
  html += "<head>\n";
  html += "<meta charset=\"UTF-8\">\n";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  html += "<title>WLBridge4TRX</title>\n";
  html += "<style>\n";
  html += "body { font-family: Arial, sans-serif; }\n";
  html += ".main { max-width: 450px; margin: 0 auto; }\n";
  html += ".version { font-size: 10px; margin-top: 20px; }\n";
  html += "input[type='text'] { width: 440px; padding: 5px; margin: 5px 0; margin-bottom:15px; border: 1px solid gray; }\n";
  html += "input[type='submit'] { width: 452px; padding: 10px; margin-top: 10px; border: 1px solid gray; }\n";
  html += "label {font-weight: bold; }\n";
  html += "textarea { width: 446px; height: 500px; font-family:Monaco; border: 1px solid gray; margin: 5px 0 }\n";
  html += "</style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "<div class=\"main\">\n";
  html += "<h1>WLBridge4TRX - Setup</h1>\n";
  html += "<form action='/update' method='post'>\n";
  html += "<label for='wl_URL'>Wavelog URL (with full path to radio API)</label><br>\n";
  html += "<input type='text' id='wl_URL' name='wl_URL' value='"+gWavelogURL+"'><br>\n";
  html += "<label for='wl_Token'>Wavelog Token</label><br>\n";
  html += "<input type='text' id='wl_Token' name='wl_Token' value='"+gWavelogToken+"'><br>\n";
  html += "<label for='wl_Radio'>Wavelog Radio Name</label><br>\n";
  html += "<input type='text' id='wl_Radio' name='wl_Radio' value='"+gWavelogRadio+"'><br>\n";
  html += "<label for='wl_rootCACertificate'>Wavelog Root CA Certificate (Only for HTTPS)</label><br>\n";
  html += "<textarea id='wl_rootCACertificate' name='wl_rootCACertificate'>"+gWavelogCertificate+"</textarea><br>\n";
  html += "<input type='submit' value='Update'>\n";
  html += "</form>\n";
  html += "<div class='version'>Version: "+String(VERSION_MAJOR)+"."+String(VERSION_MINOR)+" - <a href='http://github.com/zone11/WLBridge4TRX' target='blank'/>github.com/zone11/WLBridge4TRX</a></div>\n";
  html += "</div>\n";
  html += "</body>\n";
  html += "</html>\n";

  server.send(200, "text/html", html);
}

void webSiteUpdate() {
  String html = "<!DOCTYPE html>\n";
  html += "<html>\n";
  html += "<head>\n";
  html += "<meta charset=\"UTF-8\">\n";
  html += "<meta http-equiv=\"refresh\" content=\"3; URL=/\" />\n";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  html += "<title>WLBridge4TRX</title>\n";
  html += "<style>\n";
  html += "body { font-family: Arial, sans-serif; }\n";
  html += ".main { max-width: 450px; margin: 0 auto; }\n";
  html += "</style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "<div class=\"main\">\n";
  html += "<h1>WLBridge4TRX - Setup</h1>\n";
  html += "Settings saved, rebooting...\n";
  html += "</div>\n";
  html += "</body>\n";
  html += "</html>\n";

  gWavelogURL = server.arg("wl_URL");
  gWavelogToken = server.arg("wl_Token");
  gWavelogRadio = server.arg("wl_Radio");
  gWavelogCertificate = server.arg("wl_rootCACertificate");

  if (savePreferences(gWavelogURL, gWavelogToken, gWavelogRadio, gWavelogCertificate)) {
    server.send(200, "text/html", html);
    logging("HTTP","Saving config sucessfull, rebooting");
    delay(2000);
    ESP.restart();
  } else {
    logging("HTTP", "Saving config failed!");
  }
}


void TaskRadioUpdate(void *pvParameters) {
  while (true) {
    trx.pollCatInterface();
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void onRadioStateChanged(int freq, int mode) {
  logging("Radio", "Callback - Freq: " + String(freq) + ", Mode: " + String(mode));
  gRadioFrequency = freq;
  gRadioMode = mode;
  wl.sendQRG(gWavelogRadio, catModes[mode], freq);
}

void setup() {
  // Init Serial (Debug)
  Serial.begin(115200);

  // Init OLED and show splash
  if (!displayInit()){
    logging("OLED","initialization failed!");
  };

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

  // Prepare Wavelog
  wl.init(gWavelogURL, gWavelogToken, gWavelogCertificate);

  // Show acutal Infos on Display
  //displayInfos("booting",netIP,"Online "+wl.getVersion(),"USB",10, 14380);

  // Start local web server
  server.on("/", webSiteHome);
  server.on("/update", webSiteUpdate);
  server.begin();
  logging("HTTP","Service started");

  // Announce HTTP of this device using mDNS
  #if ENABLE_MDNS == 1
  if (!MDNS.begin("WLBridge4TRX")) {
    logging("MDNS","Service failed!");
  } else {
    logging("MDNS","Service started");
  }
  MDNS.addService("http", "tcp", 80);
  #endif

  // Radio CAT Interface
  trx.begin();
  trx.setOnStateChanged(onRadioStateChanged);
  logging("Radio","CAT Interface started");

  // FreeRTOS-Tasks
  logging("RTOS","Running Tasks");
  xTaskCreate(TaskRadioUpdate, "RadioTask", 8192, NULL, 1, NULL);
}

void loop() {
  // Ensure the WebServer for configurations
  server.handleClient();
}
