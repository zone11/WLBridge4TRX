# WLBridge4TRX
A WIFI bridge to connect your radio directly to Wavelog (Cloudlog if you think thats a good idea..) using the official API

Usage
- Create an API key in your Wavelog Account with write privilegs
- Compile and upload this project (PlattformIO) to an ESP32
- After the first boot, a SSID "WLBridge4TRX" is broadcasted, connect to it and open the website http://192.168.4.1 to configure your WIFI credentials
- Once the ESP32 is connected to your network/hotshop, open the configuration website http://WLBridge4TRX.local or using the IP reported over the serial console.
- Add your API endpoint, the token and a name for the radio to be reported in Wavelog.
- If your installation is SSL enabled, add the Root CA Certificat of the issuer of your SSL certificate. The one for Let's encrypt is configured by default.

Currently supporting only YAESU Radio with CAT
- Connect levelshifter to Serial2 pins of the ESP32 -> G16, G17
- Set the radio to 9600bps for CAT

Serial console
- Connect via USB at 15200,8,N,1
- You will see whats going on ;)

