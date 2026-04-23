#pragma once
#include <LiquidCrystal_I2C.h>
#include "config.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/display.h — LCD 16×2 I2C  (addr 0x27)                  ║
// ║  SDA=GPIO21  SCL=GPIO22                                     ║
// ╚══════════════════════════════════════════════════════════════╝

class LCDDisplay {
public:
    LiquidCrystal_I2C lcd;

    LCDDisplay() : lcd(LCD_I2C_ADDR, 16, 2) {}

    void begin() {
        lcd.init();
        lcd.backlight();
        show("EzRover Pro", "Starting...");
        delay(1000);
    }

    void show(const char* l1, const char* l2 = "                ") {
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print(l1);
        lcd.setCursor(0, 1); lcd.print(l2);
    }

    // Dòng 1: "AVOID  192.168.1.5"  (mode + IP)
    // Dòng 2: "S:145cm  Y:32°"
    void updateMain(const char* mode, float speedCms,
                    float yawDeg, const char* ip = "") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(mode);
        if (strlen(ip) > 0) {
            // Căn phải IP
            int col = 16 - (int)strlen(ip);
            if (col > 0) { lcd.setCursor(col, 0); lcd.print(ip); }
        }
        lcd.setCursor(0, 1);
        lcd.print("S:");
        lcd.print((int)speedCms);
        lcd.print("cm Y:");
        lcd.print((int)yawDeg);
        lcd.write((uint8_t)223); // ký tự °
    }

    void showIP(const String& ip) {
        show("IP:", ip.c_str());
    }
};
