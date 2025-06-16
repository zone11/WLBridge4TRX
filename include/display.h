#include <U8g2lib.h>

class Display {
public:
    Display(uint8_t sclPin, uint8_t sdaPin);
    bool begin();
    void showInfos(String status, String ip, String trx, String mode, int pwr, long qrg);
private:
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2;
    bool initialized;
};

Display::Display(uint8_t sclPin, uint8_t sdaPin)
    : u8g2(U8G2_R0, sclPin, sdaPin, U8X8_PIN_NONE), initialized(false) {}

bool Display::begin() {
    if (u8g2.begin()) {
        initialized = true;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_7x13_mf);
        u8g2.drawStr(0, 12, "WL4TRX");
        u8g2.drawStr(128 - u8g2.getStrWidth(cVersion.c_str()), 12, cVersion.c_str());

        u8g2.setFont(u8g2_font_6x12_me);
        u8g2.drawStr(0, 28, "Starting up!");
        u8g2.sendBuffer();
        return true;
    } else {
        initialized = false;
        return false;
    }
}

void Display::showInfos(String status, String ip, String trx, String mode, int pwr, long qrg) {
    if (!initialized) return;
    String myqrg = String(qrg) + " / " + mode;
    String mypwr = String(pwr);
    u8g2.setFont(u8g2_font_6x12_me);
    u8g2.drawStr(0, 28, "IP:       ");
    u8g2.drawStr(128 - u8g2.getStrWidth(ip.c_str()), 28, ip.c_str());
    u8g2.drawStr(0, 40, "WL:       ");
    u8g2.drawStr(128 - u8g2.getStrWidth(trx.c_str()), 40, trx.c_str());
    u8g2.drawStr(0, 52, "QRG:        ");
    u8g2.drawStr(128 - u8g2.getStrWidth(myqrg.c_str()), 52, myqrg.c_str());
    // u8g2.drawStr(0, 64, "PWR:       ");
    // u8g2.drawStr(128 - u8g2.getStrWidth(mypwr.c_str()), 64, mypwr.c_str());
    u8g2.sendBuffer();
}