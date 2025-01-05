#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0,22,21,U8X8_PIN_NONE);

bool displayInit() {
    if (u8g2.begin()) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_7x13_mf);
        u8g2.drawStr(0, 12, "WL4TRX");
        u8g2.drawStr(128-u8g2.getStrWidth("v0.2"), 12, "v0.2");

        u8g2.setFont(u8g2_font_6x12_me);
        u8g2.drawStr(0, 28, "Starting up!");
        u8g2.sendBuffer();
        return true;
    } else {
        return false;
    }
}

void displayInfos(String status, String ip, String trx, String mode, int pwr, long qrg) {
    String myqrg = String(qrg) + " / "+ mode;
    String mypwr = String(pwr);
    u8g2.setFont(u8g2_font_6x12_me);
    u8g2.drawStr(0, 28, "IP:       ");
    u8g2.drawStr(128-u8g2.getStrWidth(ip.c_str()), 28, ip.c_str());
    u8g2.drawStr(0, 40, "WL:       ");
    u8g2.drawStr(128-u8g2.getStrWidth(trx.c_str()), 40, trx.c_str());
    u8g2.drawStr(0, 52, "QRG:        ");
    u8g2.drawStr(128-u8g2.getStrWidth(myqrg.c_str()), 52, myqrg.c_str());
    /*u8g2.drawStr(0, 64, "PWR:       ");
    u8g2.drawStr(128-u8g2.getStrWidth(mypwr.c_str()), 64, mypwr.c_str());    */   
    u8g2.sendBuffer();
}