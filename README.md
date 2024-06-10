# WLBridge4TRX
A WIFI bridge to connect your radio directly to Wavelog using the official API.
Cloudlog should also work, but is not tested. Consider switching to Wavelog ;)

Serial console
- Connect via USB at 15200,8,N,1
- You will see whats going on.

Currently supporting only YAESU Radio with CAT
- Connect a levelshifter to Serial2 pins of the ESP32 -> G16, G17 to interface with CAT of your radio, more about this very soon on this stage.
- Set the radio serial speed for CAT to 9600bps.

Usage
- Create an API key in your Wavelog Account with write privilegs.
- Compile and upload this project (PlattformIO) to an ESP32.
- After the first boot, a SSID "WLBridge4TRX" is broadcasted. Connect to it and open the website http://192.168.4.1 to configure your WIFI credentials
- Once the ESP32 is connected to your network/hotshop, open the configuration website http://WLBridge4TRX.local or using the IP reported over the serial console.
- Add your API endpoint, the token and a name for the radio to be reported in Wavelog.
- If your installation is SSL enabled, add the Root CA Certificate from the issuer of your SSL certificate. The one for Let's encrypt is configured by default.
