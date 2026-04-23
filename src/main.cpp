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
    checkServoAutoDetach();

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

    // ── Control loop 50Hz: đọc encoder → cập nhật pose → dispatch mode ──
    if (now - tCtrl >= LOOP_CONTROL_MS) {
        tCtrl = now;

        enc.update();              // Tính tốc độ bánh từ tick encoder
        int dL = 0;
        int dR = 0;
        enc.getAndResetDelta(dL, dR); // Lấy delta tick cho odometry

        pose.update(dL, dR);      // Cập nhật (x, y, θ) theo dead-reckoning

        switch (mode) {
            case MANUAL:
                // cmdL/cmdR được set bởi ws_handlers khi nhận lệnh "drive"
                driveRaw(cmdL, cmdR);
                break;
            case AVOID:
                // State machine tránh vật cản — không blocking
                doAvoid();
                break;
            case GOTO:
                // Điều hướng đến (targetX, targetY) — trả true khi đến nơi
                if (nav.tick(pose.pose)) {
                    mode = MANUAL;
                    broadcastTelemetry("arrived");
                }
                break;
            case SCAN:
                // Đứng yên — chờ lệnh từ web
                break;
        }
    }

    // ── Sonar fullScan 0.4Hz: chỉ chạy khi AVOID hoặc GOTO ──────────
    // fullScan() blocking ~1.2s — doAvoid() dùng cờ last.fresh để biết khi nào xong
    if ((mode == AVOID || mode == GOTO) && (now - tScan >= LOOP_SCAN_MS)) {
        tScan = now;
        stopMotors();        // Dừng trước khi quét để tránh rung
        sonar.fullScan();    // Quét 7 góc (0°→180°), ~1.2s
        broadcastScan();     // Gửi kết quả về Web UI
    }

    // ── Telemetry 10Hz: gửi vị trí + tốc độ + mode qua WebSocket ────
    if (now - tTele >= LOOP_TELEMETRY_MS) {
        tTele = now;
        broadcastTelemetry("update");
    }

    // ── Log chẩn đoán 1Hz: in trạng thái ra Serial Monitor ──────────
    if (now - tDiag >= 1000) {
        tDiag = now;
        logRuntimeDiag();
    }

    // ── LCD refresh 2Hz: hiển thị mode, tốc độ, góc, IP ─────────────
    if (now - tDisp >= LOOP_DISPLAY_MS) {
        tDisp = now;
        const float avgSpd = (enc.speedL_cms + enc.speedR_cms) * 0.5f;
        const char* mstr = modeStr();
        // Dùng STA IP nếu đã kết nối, ngược lại dùng AP IP
        String ip = (WiFi.status() == WL_CONNECTED)
                        ? WiFi.localIP().toString()
                        : WiFi.softAPIP().toString();
        lcd.updateMain(mstr, avgSpd, currentYawDeg(), ip.c_str());
    }
}