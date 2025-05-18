#include <Arduino.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Effortless_SPIFFS.h>
#include <ESPmDNS.h>
#include <display.h>
#include <logging.h>
#include <wavelog.h>
#include <globals.h>

#define LED_BUILTIN 2

// Variables for daily use
Wavelog wl;

String cat_buffer;
long cat_qrg = 0;
long cat_qrg_last = 0;
unsigned int cat_mode = 0;
unsigned int cat_mode_last = 0;

long wl_qrg = 0;
String wl_mode = "SSB";
unsigned long last_millis_cat = 0;
unsigned long last_millis_upload = 0;

String netIP = "";
bool netOnline = false;

// Instances for eSPIFFS (Settings) and the webserver
eSPIFFS fileSystem;
WebServer server(80);

// CAT Modes
//String catModes[] = {"ERR", "LSB", "USB", "CW","FM","AM"}; // YAESU
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

      g_wl_url = read_wl_url;
      g_wl_token = read_wl_token;
      g_wl_radio = read_wl_radio;
      g_wl_rootCACertificate = read_wl_rootCACertificate;
      
      String wl_rootCACertificateDisplay = read_wl_rootCACertificate;
      wl_rootCACertificateDisplay.replace("\n","\n\r");

      logging("Prefs", "URL: "+g_wl_url);
      logging("Prefs", "Token: "+g_wl_token);
      logging("Prefs", "Radio: "+g_wl_radio);
      logging("Prefs", "RootCA Cert: "+wl_rootCACertificateDisplay);
      
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
  html += "<input type='text' id='wl_URL' name='wl_URL' value='"+g_wl_url+"'><br>\n";
  html += "<label for='wl_Token'>Wavelog Token</label><br>\n";
  html += "<input type='text' id='wl_Token' name='wl_Token' value='"+g_wl_token+"'><br>\n";
  html += "<label for='wl_Radio'>Wavelog Radio Name</label><br>\n";
  html += "<input type='text' id='wl_Radio' name='wl_Radio' value='"+g_wl_radio+"'><br>\n";
  html += "<label for='wl_rootCACertificate'>Wavelog Root CA Certificate (Only for HTTPS)</label><br>\n";
  html += "<textarea id='wl_rootCACertificate' name='wl_rootCACertificate'>"+g_wl_rootCACertificate+"</textarea><br>\n";
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

  g_wl_url = server.arg("wl_URL");
  g_wl_token = server.arg("wl_Token");
  g_wl_radio = server.arg("wl_Radio");
  g_wl_rootCACertificate = server.arg("wl_rootCACertificate");

  if (savePreferences(g_wl_url, g_wl_token, g_wl_radio, g_wl_rootCACertificate)) {
    server.send(200, "text/html", html);
    logging("HTTP","Saving config sucessfull, rebooting");
    delay(2000);
    ESP.restart();
  } else {
    logging("HTTP", "Saving config failed!");
  }
}

// Simple task for testing
void TaskBlink(void *pvParameters) {
  pinMode(LED_BUILTIN, OUTPUT);
  while (true) {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // Init Serial (Debug) and Serial2 (CAT)
  Serial.begin(115200);
  Serial2.begin(9600);

   // FreeRTOS-Tasks
  xTaskCreate(TaskBlink,"BlinkTask",1024, NULL,1,NULL); // Testing

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
  wl.init(g_wl_url, g_wl_token, g_wl_rootCACertificate);

  // Show acutal Infos on Display
  displayInfos("booting",netIP,"Online "+wl.getVersion(),"USB",10, 14380);

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
}

void catSendRequest() {
  // YAESU: FA; for QRG, MD0 for the mode
  // ELECRAFT: FA; for QRG, MD for the mode
  Serial2.print("FA;MD;");  
}

bool catParseBuffer() {
  int posQRG = 0;
  int posMode= 0;

  if (cat_buffer.length() > 17 ) {
    // Example response YAESU: FA007045100;MD01;
    // Example response ELECRAFT: FA00014253700;MD2;
    // always catch the last entry in the buffer!
    posQRG = cat_buffer.lastIndexOf("FA");
    //posMode = cat_buffer.lastIndexOf("MD0"); // YAESU
    posMode = cat_buffer.lastIndexOf("MD"); // ELECRAFT

    //cat_qrg = cat_buffer.substring(posQRG+2,posQRG+11).toInt(); // YAESU
    //cat_mode = cat_buffer.substring(posMode+3,posMode+4).toInt(); // YAESU
    
    cat_qrg = cat_buffer.substring(posQRG+2,posQRG+13).toInt(); // ELECRAFT
    cat_mode = cat_buffer.substring(posMode+2,posMode+3).toInt(); // ELECRAFT

    cat_buffer = "";

    return true;
  }
  return false;
}
void loop() {
  // Request CAT data every second
  if (millis() > (last_millis_cat+500)) {
    catSendRequest();
    last_millis_cat = millis();
  }

  // Parse CAT response and send to Wavelog
  if (catParseBuffer()) {
    if ((cat_qrg != cat_qrg_last) || (cat_mode != cat_mode_last)) {
      logging("CAT","The data has changed!");
      wl_qrg = cat_qrg;
      wl_mode = catModes[cat_mode];

      logging("CAT", "QRG: "+String(wl_qrg));
      logging("CAT", "Mode: "+wl_mode);

      wl.sendQRG(g_wl_radio, wl_mode, wl_qrg);

      cat_qrg_last = cat_qrg;
      cat_mode_last = cat_mode;
      
    }  
   }

  // Read CAT data from Serial2
  if (Serial2.available()) {
    cat_buffer += Serial2.readString();
  }

  // Ensure the WebServer for configurations
  server.handleClient();
}
