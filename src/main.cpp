// ╔══════════════════════════════════════════════════════════════╗
// ║  src/main.cpp — EzRover Pro                                  ║
// ║                                                              ║
// ║  Dựa trên:                                                   ║
// ║    • Ezward/Esp32CameraRover2  (core framework)              ║
// ║    • lily-osp/esp-robot-control (obstacle avoidance, web)    ║
// ║                                                              ║
// ║  Hardware theo sơ đồ của bạn:                                ║
// ║    L298N  : ENA=14,IN1=13,IN2=23 / ENB=25,IN3=26,IN4=27     ║
// ║    Encoder: GPIO34 (Trái), GPIO35 (Phải)                     ║
// ║    HC-SR04: Trig=33, Echo=18 |  Servo=19                    ║
// ║    I2C    : SDA=21, SCL=22 → LCD(0x27) + MPU6050(0x68)      ║
// ║    Power  : 18650×2 → LM2596 → 5V → ESP32 + L298N           ║
// ╚══════════════════════════════════════════════════════════════╝

// ─── Arduino / ESP32 ─────────────────────────────────────────
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ─── Config ──────────────────────────────────────────────────
#include "config.h"
#include "wifi_credentials.h"

// ─── Modules ─────────────────────────────────────────────────
#include "motor.h"
#include "encoder.h"
#include "pose.h"
#include "mpu.h"
#include "sonar.h"
#include "display.h"
#include "goto_goal.h"
#include "webui.h"

// ─── Đối tượng ────────────────────────────────────────────────
EncoderManager  enc;
PoseEstimator   pose;
MPU6050Driver   mpu;
SonarScanner    sonar;
LCDDisplay      lcd;
GotoGoal        nav;

WebServer         http(80);
WebSocketsServer  wss(81);

// ─── Robot state ──────────────────────────────────────────────
enum Mode { MANUAL, AVOID, GOTO, SCAN };
Mode    mode     = MANUAL;
int     cmdL     = 0, cmdR = 0;    // PWM lệnh thủ công
uint8_t wsClient = 0xFF;           // WebSocket client số hiện tại
bool    mpuOk    = false;

// ─── Timing ───────────────────────────────────────────────────
uint32_t tCtrl = 0, tTele = 0, tDisp = 0, tScan = 0, tDiag = 0;

// ─── Forward declarations ─────────────────────────────────────
void handleHttpStatus();
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len);
void doAvoid();
void broadcastTelemetry(const char* event);
void broadcastScan();
const char* modeStr();
void logRuntimeDiag();

// ═══════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    Serial.println("\n[EzRover Pro] Booting...");

    // ── I2C (LCD + MPU6050 cùng bus) ─────────────────────────
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(50);

    // ── LCD ──────────────────────────────────────────────────
    lcd.begin();

    // ── Motors ───────────────────────────────────────────────
    motorsBegin();
    Serial.println("[Motor] OK");

    // ── Encoder ──────────────────────────────────────────────
    enc.begin();
    Serial.println("[Encoder] GPIO34+35 attached");

    // ── MPU6050 ──────────────────────────────────────────────
    lcd.show("MPU6050", "Calibrating...");
    mpuOk = mpu.begin();
    if (mpuOk) {
        Serial.println("[MPU6050] OK — calibrated");
        lcd.show("MPU6050", "OK");
    } else {
        Serial.println("[MPU6050] NOT FOUND — check wiring!");
        lcd.show("MPU6050 ERROR", "Check I2C wires");
        delay(3000);
    }

    // ── Sonar + Servo ─────────────────────────────────────────
    sonar.begin();
    Serial.printf("[Sonar] HC-SR04 + Servo OK (Trig=%d Echo=%d Servo=%d)\n",
                  SONAR_TRIG_PIN, SONAR_ECHO_PIN, SERVO_PIN);

    // ── WiFi ──────────────────────────────────────────────────
    // Luôn bật AP để truy cập ổn định tại 192.168.4.1,
    // đồng thời vẫn thử kết nối router WiFi nội bộ (STA).
    lcd.show("WiFi", "AP+STA...");
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    String apIp = WiFi.softAPIP().toString();
    Serial.println("[WiFi] AP ready: " + apIp + " (SSID: " + String(AP_SSID) + ")");

    Serial.printf("[WiFi] Attempting to connect to SSID: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint8_t tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 60) {
        delay(500);
        Serial.print('.');
        tries++;
    }

    String ip;
    if (WiFi.status() == WL_CONNECTED) {
        ip = WiFi.localIP().toString();
        Serial.println("\n[WiFi] STA connected: " + ip);
    } else {
        ip = apIp;
        Serial.println("\n[WiFi] STA failed, stay on AP: " + apIp);
    }
    lcd.showIP(ip);
    delay(2000);

    // ── HTTP server ───────────────────────────────────────────
    http.on("/", []() {
        http.send_P(200, "text/html", HTML_PAGE);
    });
    http.on("/status", handleHttpStatus);
    http.begin();
    Serial.println("[HTTP] port 80 OK");

    // ── WebSocket server ──────────────────────────────────────
    wss.begin();
    wss.onEvent(onWsEvent);
    Serial.println("[WS]   port 81 OK");

    Serial.printf("[Pins] Motor ENA=%d IN1=%d IN2=%d | ENB=%d IN3=%d IN4=%d\n",
                  MOTOR_A_ENA, MOTOR_A_IN1, MOTOR_A_IN2,
                  MOTOR_B_ENB, MOTOR_B_IN3, MOTOR_B_IN4);
    Serial.printf("[Pins] Encoder L=%d R=%d | I2C SDA=%d SCL=%d\n",
                  ENC_LEFT_PIN, ENC_RIGHT_PIN, I2C_SDA, I2C_SCL);

    lcd.show("EzRover Pro", ("IP:" + ip.substring(ip.lastIndexOf('.')+1)).c_str());
    Serial.println("[EzRover Pro] Ready!");
}

// ═══════════════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════════════
void loop() {
    http.handleClient();
    wss.loop();

    uint32_t now = millis();

    // ╔═══ Control Loop — 50Hz (every 20ms) ══════════════════╗
    if (now - tCtrl >= LOOP_CONTROL_MS) {
        float dt = (now - tCtrl) / 1000.0f;
        tCtrl = now;

        // 1. Cập nhật IMU
        mpu.update(dt);

        // 2. Cập nhật encoder, lấy delta ticks
        enc.update();
        int dL, dR;
        enc.getAndResetDelta(dL, dR);

        // 3. Cập nhật pose (encoder + IMU fusion)
        pose.update(dL, dR, mpu.yawDeg);

        // 4. Xử lý theo mode
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
                // Xử lý trong tScan block
                break;
        }
    } // end control loop

    // ╔═══ Scan siêu âm khi AVOID/GOTO — mỗi 2.5s ═══════════╗
    if ((mode == AVOID || mode == GOTO) && (now - tScan >= LOOP_SCAN_MS)) {
        tScan = now;
        stopMotors();           // Dừng để scan chính xác
        sonar.fullScan();
        broadcastScan();
        // GotoGoal sẽ tự tiếp tục ở loop tiếp theo
    }

    // ╔═══ Telemetry WS — 10Hz (every 100ms) ═════════════════╗
    if (now - tTele >= LOOP_TELEMETRY_MS) {
        tTele = now;
        broadcastTelemetry("update");
    }

    // ╔═══ Serial diagnostics — every 1s ═════════════════════╗
    if (now - tDiag >= 1000) {
        tDiag = now;
        logRuntimeDiag();
    }

    // ╔═══ LCD — 2Hz (every 500ms) ════════════════════════════╗
    if (now - tDisp >= LOOP_DISPLAY_MS) {
        tDisp = now;
        float avgSpd = (enc.speedL_cms + enc.speedR_cms) * 0.5f;
        const char* mstr = modeStr();
        String ip = (WiFi.status() == WL_CONNECTED)
                    ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
        lcd.updateMain(mstr, avgSpd, mpu.yawDeg, ip.c_str());
    }
}

// ═══════════════════════════════════════════════════════════════
//  OBSTACLE AVOIDANCE (lấy từ lily-osp/esp-robot-control)
// ═══════════════════════════════════════════════════════════════
void doAvoid() {
    static uint32_t tAvoidScan = 0;
    uint32_t now = millis();

    if (sonar.obstacleAhead()) {
        stopMotors();
        delay(150);
        sonar.fullScan();
        broadcastScan();

        int turn = sonar.turnAngleDeg(); // dương=trái, âm=phải

        if (abs(turn) < 15) {
            // Thẳng trước cũng bị chặn → lùi + quay
            driveRaw(-SPEED_TURN, -SPEED_TURN);
            delay(300);
        }

        // Quay về hướng thoáng
        int turnMs = map(abs(turn), 0, 90, 0, 500);
        if (turn > 0) driveRaw(-SPEED_TURN,  SPEED_TURN);  // rẽ trái
        else          driveRaw( SPEED_TURN, -SPEED_TURN);  // rẽ phải
        delay(turnMs);
        stopMotors();
        delay(100);
    } else {
        driveRaw(SPEED_DEFAULT, SPEED_DEFAULT);
    }
}

// ═══════════════════════════════════════════════════════════════
//  WEBSOCKET EVENT HANDLER
// ═══════════════════════════════════════════════════════════════
void onWsEvent(uint8_t num, WStype_t type,
               uint8_t* payload, size_t len) {

    if (type == WStype_CONNECTED) {
        wsClient = num;
        Serial.printf("[WS] Client #%d connected\n", num);
        return;
    }
    if (type == WStype_DISCONNECTED) {
        Serial.printf("[WS] Client #%d disconnected\n", num);
        return;
    }
    if (type != WStype_TEXT) return;

    // Parse JSON
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload, len) != DeserializationError::Ok) return;

    const char* cmd = doc["cmd"] | "";
    Serial.printf("[WS] RX cmd=%s len=%u\n", cmd, (unsigned int)len);

    // ── drive ─────────────────────────────────────────────────
    if (strcmp(cmd, "drive") == 0) {
        mode = MANUAL;
        cmdL = constrain((int)doc["l"], -255, 255);
        cmdR = constrain((int)doc["r"], -255, 255);
        enc.setDirection(cmdL, cmdR);
        Serial.printf("[CMD] drive L=%d R=%d\n", cmdL, cmdR);
    }
    // ── stop ──────────────────────────────────────────────────
    else if (strcmp(cmd, "stop") == 0) {
        mode = MANUAL;
        cmdL = cmdR = 0;
        stopMotors();
        Serial.println("[CMD] stop");
    }
    // ── goto ──────────────────────────────────────────────────
    else if (strcmp(cmd, "goto") == 0) {
        float gx = doc["x"] | 0.0f;
        float gy = doc["y"] | 0.0f;
        nav.set(gx, gy);
        mode = GOTO;
        Serial.printf("[GoTo] Target: (%.1f, %.1f)\n", gx, gy);
    }
    // ── mode ──────────────────────────────────────────────────
    else if (strcmp(cmd, "mode") == 0) {
        const char* m = doc["mode"] | "manual";
        if      (strcmp(m, "avoid")  == 0) {
            mode = AVOID;
            Serial.println("[Mode] AVOID");
        }
        else if (strcmp(m, "manual") == 0) {
            mode = MANUAL; cmdL = cmdR = 0; stopMotors();
            Serial.println("[Mode] MANUAL");
        }
        else if (strcmp(m, "scan")   == 0) {
            stopMotors();
            sonar.fullScan();
            broadcastScan();
            mode = MANUAL;
            Serial.println("[Mode] SCAN -> MANUAL");
        }
    }
    // ── reset_pose ────────────────────────────────────────────
    else if (strcmp(cmd, "reset_pose") == 0) {
        pose.reset();
        mpu.resetYaw();
        enc.reset();
        Serial.println("[Pose] Reset");
    }
    // ── servo pan ─────────────────────────────────────────────
    else if (strcmp(cmd, "servo") == 0) {
        int pan = constrain((int)doc["pan"], 0, 180);
        sonar.setAngle(pan);
    }
    // ── path waypoints (từ web Wavefront BFS) ─────────────────
    else if (strcmp(cmd, "path") == 0) {
        // Rover nhận waypoints — hiện tại GoToGoal dùng 1 target
        // Có thể mở rộng thành queue waypoints
        Serial.println("[Path] Received waypoints from web BFS");
    }
}

// ═══════════════════════════════════════════════════════════════
//  BROADCAST HELPERS
// ═══════════════════════════════════════════════════════════════
void broadcastTelemetry(const char* event) {
    StaticJsonDocument<256> doc;
    doc["type"]  = "telemetry";
    doc["event"] = event;
    doc["x"]     = pose.pose.x;
    doc["y"]     = pose.pose.y;
    doc["yaw"]   = mpu.yawDeg;
    doc["spdL"]  = enc.speedL_cms;
    doc["spdR"]  = enc.speedR_cms;
    doc["temp"]  = mpu.chipTemp;
    doc["mode"]  = modeStr();
    String out; serializeJson(doc, out);
    wss.broadcastTXT(out);
}

void broadcastScan() {
    StaticJsonDocument<512> doc;
    doc["type"] = "scan";
    JsonArray a = doc.createNestedArray("angles");
    JsonArray d = doc.createNestedArray("dists");
    for (int i = 0; i < SCAN_STEPS; i++) {
        a.add(sonar.last.angle[i]);
        d.add(sonar.last.dist[i]);
    }
    String out; serializeJson(doc, out);
    wss.broadcastTXT(out);
}

// ─── HTTP /status ─────────────────────────────────────────────
void handleHttpStatus() {
    StaticJsonDocument<128> doc;
    doc["mode"] = modeStr();
    doc["x"]    = pose.pose.x;
    doc["y"]    = pose.pose.y;
    doc["yaw"]  = mpu.yawDeg;
    doc["temp"] = mpu.chipTemp;
    String out; serializeJson(doc, out);
    http.send(200, "application/json", out);
}

const char* modeStr() {
    switch (mode) {
        case MANUAL: return "MANUAL";
        case AVOID:  return "AVOID";
        case GOTO:   return "GOTO";
        case SCAN:   return "SCAN";
        default:     return "?";
    }
}

void logRuntimeDiag() {
    float sonarFront = sonar.measureCm();
    bool wsOnline = (wsClient != 0xFF);
    Serial.printf("[Diag] mode=%s cmdL=%d cmdR=%d spdL=%.1f spdR=%.1f sonar=%.1fcm yaw=%.1f temp=%.1f mpu=%s ws=%s\n",
                  modeStr(), cmdL, cmdR,
                  enc.speedL_cms, enc.speedR_cms,
                  sonarFront, mpu.yawDeg, mpu.chipTemp,
                  mpuOk ? "OK" : "ERR",
                  wsOnline ? "ON" : "OFF");
}
