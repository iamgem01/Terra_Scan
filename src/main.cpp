#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>

#include "config.h"
#include "wifi_credentials.h"
#include "motor.h"
#include "webui.h"
#include "app_context.h"

void setup() {
    Serial.begin(115200);
    Serial.println("\n[EzRover Pro] Booting...");

    Wire.begin(I2C_SDA, I2C_SCL);
    // *** FIX 1: giảm delay I2C từ 50ms → 20ms
    //     Wire.begin() chỉ cần ~10ms để ổn định bus
    delay(20);

    lcd.begin();

    motorsBegin();
    Serial.println("[Motor] OK");

    enc.begin();
    Serial.println("[Encoder] GPIO34+35 attached");

    sonar.begin();
    Serial.printf("[Sonar] HC-SR04 + Servo OK (Trig=%d Echo=%d Servo=%d)\n",
                  SONAR_TRIG_PIN, SONAR_ECHO_PIN, SERVO_PIN);

    lcd.show("WiFi", "AP+STA...");

    // *** FIX 2: Khởi động AP trước, sau đó bắt đầu STA connect không blocking
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    String apIp = WiFi.softAPIP().toString();
    Serial.println("[WiFi] AP ready: " + apIp + " (SSID: " + String(AP_SSID) + ")");

    // *** FIX 2: Bắt đầu STA connect KHÔNG blocking
    //     HTTP/WS server khởi động ngay lập tức với AP IP
    //     STA connection hoàn tất trong background, được kiểm tra trong loop()
    Serial.printf("[WiFi] STA connecting to: %s (background)\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Dùng AP IP để hiển thị ngay, sẽ update sau khi STA kết nối
    String ip = apIp;

    // *** FIX 3: Bỏ delay(500) trước đây (hoặc giảm rất ngắn)
    //     LCD hiển thị AP IP ngay lập tức
    lcd.showIP(ip);

    http.on("/", []() {
        http.send_P(200, "text/html", HTML_PAGE);
    });
    http.on("/status", handleHttpStatus);
    http.begin();
    Serial.println("[HTTP] port 80 OK");

    wss.begin();
    wss.onEvent(onWsEvent);
    Serial.println("[WS] port 81 OK");

    Serial.printf("[Pins] Motor ENA=%d IN1=%d IN2=%d | ENB=%d IN3=%d IN4=%d\n",
                  MOTOR_A_ENA, MOTOR_A_IN1, MOTOR_A_IN2,
                  MOTOR_B_ENB, MOTOR_B_IN3, MOTOR_B_IN4);
    Serial.printf("[Pins] Encoder L=%d R=%d | I2C SDA=%d SCL=%d\n",
                  ENC_LEFT_PIN, ENC_RIGHT_PIN, I2C_SDA, I2C_SCL);

    lcd.show("EzRover Pro", ("AP:" + apIp.substring(apIp.lastIndexOf('.') + 1)).c_str());
    Serial.println("[EzRover Pro] Ready! (STA connecting in background...)");
}

void loop() {
    http.handleClient();
    wss.loop();

    const uint32_t now = millis();

    // *** FIX 2: Kiểm tra STA connection trong loop() — không block setup()
    //     Khi kết nối xong, update LCD và log
    static bool staConnected = false;
    static uint32_t tStaCheck = 0;
    if (!staConnected && (now - tStaCheck >= 500)) {
        tStaCheck = now;
        if (WiFi.status() == WL_CONNECTED) {
            staConnected = true;
            String staIp = WiFi.localIP().toString();
            Serial.println("[WiFi] STA connected: " + staIp);
            lcd.showIP(staIp);
            // Broadcast IP mới qua WebSocket cho client đang kết nối
            broadcastTelemetry("wifi_sta_connected");
        }
    }

    if (now - tCtrl >= LOOP_CONTROL_MS) {
        tCtrl = now;

        enc.update();
        int dL = 0;
        int dR = 0;
        enc.getAndResetDelta(dL, dR);

        pose.update(dL, dR);

        switch (mode) {
            case MANUAL:
                driveRaw(cmdL, cmdR);
                break;
            case AVOID:
                doAvoid();
                break;
            case GOTO:
                if (nav.tick(pose.pose)) {
                    mode = MANUAL;
                    broadcastTelemetry("arrived");
                }
                break;
            case SCAN:
                break;
        }
    }

    if ((mode == AVOID || mode == GOTO) && (now - tScan >= LOOP_SCAN_MS)) {
        tScan = now;
        stopMotors();
        sonar.fullScan();
        broadcastScan();
    }

    if (now - tTele >= LOOP_TELEMETRY_MS) {
        tTele = now;
        broadcastTelemetry("update");
    }

    if (now - tDiag >= 1000) {
        tDiag = now;
        logRuntimeDiag();
    }

    if (now - tDisp >= LOOP_DISPLAY_MS) {
        tDisp = now;
        const float avgSpd = (enc.speedL_cms + enc.speedR_cms) * 0.5f;
        const char* mstr = modeStr();
        String ip = (WiFi.status() == WL_CONNECTED)
                        ? WiFi.localIP().toString()
                        : WiFi.softAPIP().toString();
        lcd.updateMain(mstr, avgSpd, currentYawDeg(), ip.c_str());
    }
}