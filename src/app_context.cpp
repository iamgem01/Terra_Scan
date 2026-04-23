// ╔══════════════════════════════════════════════════════════════╗
// ║  src/app_context.cpp — Định nghĩa tất cả biến toàn cục      ║
// ║  File này là "nơi sinh ra" của shared state.                ║
// ║  Các module khác chỉ dùng extern qua app_context.h          ║
// ╚══════════════════════════════════════════════════════════════╝
#include "app_context.h"

// ─── Đối tượng phần cứng ─────────────────────────────────────
EncoderManager enc;   // Quản lý encoder hai bánh
PoseEstimator  pose;  // Ước tính vị trí (x, y, θ) bằng odometry
SonarScanner   sonar; // HC-SR04 + servo quét 180°
LCDDisplay     lcd;   // Màn hình LCD 16×2 I2C
GotoGoal       nav;   // Bộ điều hướng tự động

// ─── Máy chủ mạng ────────────────────────────────────────────
WebServer        http(80);  // Phục vụ trang web điều khiển
WebSocketsServer wss(81);   // Kênh realtime: lệnh + telemetry

// ─── Trạng thái điều khiển ───────────────────────────────────
Mode    mode     = MANUAL;  // Khởi động ở chế độ thủ công
int     cmdL     = 0;       // Lệnh PWM bánh trái (set bởi ws_handlers)
int     cmdR     = 0;       // Lệnh PWM bánh phải
uint8_t wsClient = 0xFF;    // 0xFF = chưa có client nào kết nối

// ─── Timer các vòng lặp trong loop() ─────────────────────────
// Tất cả đều dùng kiểu "millis() - tXxx >= interval" để non-blocking
uint32_t tCtrl = 0;  // Cập nhật encoder + mode dispatch (50Hz)
uint32_t tTele = 0;  // Gửi telemetry WebSocket (10Hz)
uint32_t tDisp = 0;  // Cập nhật LCD (2Hz)
uint32_t tScan = 0;  // Quét sonar fullScan khi AVOID/GOTO (0.4Hz)
uint32_t tDiag = 0;  // In log chẩn đoán ra Serial (1Hz)
