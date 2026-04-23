#pragma once

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/app_context.h — Trạng thái toàn cục & khai báo extern  ║
// ║  Mọi module #include file này để truy cập shared state      ║
// ║  Định nghĩa thực sự nằm trong app_context.cpp               ║
// ╚══════════════════════════════════════════════════════════════╝

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "encoder.h"
#include "pose.h"
#include "sonar.h"
#include "display.h"
#include "goto_goal.h"

// ─── Chế độ hoạt động của xe ──────────────────────────────────
// MANUAL : điều khiển trực tiếp qua WebSocket
// AVOID  : tự tránh vật cản bằng sonar + servo quét
// GOTO   : tự di chuyển đến tọa độ (x, y) bằng odometry
// SCAN   : đứng yên, chờ dữ liệu quét sonar
enum Mode { MANUAL, AVOID, GOTO, SCAN };

// ─── Đối tượng phần cứng (định nghĩa trong app_context.cpp) ──
extern EncoderManager enc;    // Encoder hai bánh (GPIO34/35)
extern PoseEstimator  pose;   // Dead-reckoning odometry
extern SonarScanner   sonar;  // HC-SR04 + servo sweep
extern LCDDisplay     lcd;    // LCD 16×2 I2C (0x27)
extern GotoGoal       nav;    // Bộ điều hướng đến đích

// ─── Máy chủ mạng ────────────────────────────────────────────
extern WebServer        http;  // HTTP port 80 — phục vụ Web UI
extern WebSocketsServer wss;   // WebSocket port 81 — realtime control

// ─── Trạng thái điều khiển ───────────────────────────────────
extern Mode    mode;      // Chế độ hiện tại
extern int     cmdL;      // PWM bánh trái từ lệnh WebSocket (-255…+255)
extern int     cmdR;      // PWM bánh phải từ lệnh WebSocket
extern uint8_t wsClient;  // ID client WS đang kết nối (0xFF = không có)

// ─── Timer cho các vòng lặp không blocking ───────────────────
extern uint32_t tCtrl;   // Timer control loop 50Hz (20ms)
extern uint32_t tTele;   // Timer telemetry 10Hz (100ms)
extern uint32_t tDisp;   // Timer LCD refresh 2Hz (500ms)
extern uint32_t tScan;   // Timer sonar fullScan 0.4Hz (2500ms)
extern uint32_t tDiag;   // Timer log chẩn đoán 1Hz (1000ms)

// ─── Khai báo hàm từ các module khác ─────────────────────────
void handleHttpStatus();   // Trả JSON trạng thái qua HTTP GET /status
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len); // WebSocket event handler
void doAvoid();            // State machine tránh vật cản — gọi mỗi 20ms
void resetAvoidState();    // Reset AVOID state machine về AV_CHECK
void checkServoAutoDetach(); // Auto-detach servo sau 2s idle (gọi từ loop())
void broadcastTelemetry(const char* event); // Gửi JSON telemetry đến WS clients
void broadcastScan();      // Gửi kết quả quét sonar đến WS clients
const char* modeStr();     // Chuyển Mode enum → chuỗi ("MANUAL", "AVOID", …)
void logRuntimeDiag();     // In log chẩn đoán ra Serial mỗi giây
float currentYawDeg();     // Góc yaw hiện tại (độ) từ odometry
float currentTempC();      // Nhiệt độ (°C) — placeholder, luôn trả 0