# WLBridge4TRX
A WIFI bridge to connect your radio directly to Wavelog using the official radio API.\
Cloudlog should also work, but is not tested -> have a look at Wavelog ;)

### Serial console
- Connect via USB at 15200,8,N,1
- You will see whats going on.

### Radios
Currently tested with YAESU (991A) and Elecraft (KX2) Radios using CAT over RS232
- Connect a levelshifter (https://www.sparkfun.com/products/11189) to Serial2 pins of the ESP32 -> G16(RX), G17(TX) to interface with CAT/RS232 port of your radio.
- Set the radio serial speed for CAT to 9600bps.

### Usage
- Create an API key in your Wavelog Account with read & write permission.
- Compile and upload this project (PlattformIO) to an ESP32.
- After the first boot, a SSID "WLBridge4TRX" is broadcasted. Connect to it and open the website http://192.168.4.1 to configure your WIFI credentials
- Once the ESP32 is connected to your network/hotspot, open the configuration website http://WLBridge4TRX.local or using the IP reported in the serial console.
- Add your API endpoint (full URL like https://mywavelog.url/api/radio, the API key (token) and a name for the radio to be shown in Wavelog.
- If your installation is SSL enabled, add the Root CA Certificate from the issuer of your SSL certificate. The one for Let's encrypt is configured by default.

### Example (Elecraft KX2)
Using a Sparkfun ESP32 Thing with a LiPo Battery for portable operation. The radio is connected using a MAX2323 levelshifter to the radios RS232 Port (Port ACC).
![Elecraft KX2 Example](https://github.com/zone11/WLBridge4TRX/blob/main/misc/ESP32Thing-KX2.jpg)

### ICOM Support
I don't have an ICOM radio but Kim (DG9VH) build an interface for CI-V. Have a look at his project here: https://github.com/dg9vh/Wavelog_CI_V

### Questions/Suggestions
Feel free to open an issue or start a discussion.

73 de HB9HJQ, Christian
