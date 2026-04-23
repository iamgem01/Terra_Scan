// ╔══════════════════════════════════════════════════════════════╗
// ║  src/runtime_utils.cpp — Các hàm tiện ích thời gian chạy   ║
// ╚══════════════════════════════════════════════════════════════╝
#include "app_context.h"

// Chuyển góc θ từ radian sang độ
float currentYawDeg() {
    return pose.pose.theta * RAD_TO_DEG;
}

// Đọc nhiệt độ — placeholder (chưa có cảm biến nhiệt độ)
float currentTempC() {
    return 0.0f;
}

// Chuyển enum Mode → chuỗi để log và gửi qua WebSocket
const char* modeStr() {
    switch (mode) {
        case MANUAL: return "MANUAL";
        case AVOID:  return "AVOID";
        case GOTO:   return "GOTO";
        case SCAN:   return "SCAN";
        default:     return "?";
    }
}

// In log chẩn đoán ra Serial mỗi giây
// Chỉ đo sonar khi xe đang tránh vật cản hoặc đi đến đích —
// tránh phát xung hồi âm không cần thiết khi MANUAL/SCAN
void logRuntimeDiag() {
    float sonarFront = (mode == AVOID || mode == GOTO) ? sonar.measureCm() : -1.0f;
    bool wsOnline = (wsClient != 0xFF);
    Serial.printf(
        "[Diag] mode=%s cmdL=%d cmdR=%d spdL=%.1f spdR=%.1f sonar=%.1fcm yaw=%.1f temp=%.1f ws=%s\n",
        modeStr(),
        cmdL,
        cmdR,
        enc.speedL_cms,
        enc.speedR_cms,
        sonarFront,
        currentYawDeg(),
        currentTempC(),
        wsOnline ? "ON" : "OFF");
}
