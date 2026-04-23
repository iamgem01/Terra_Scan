// ╔══════════════════════════════════════════════════════════════╗
// ║  src/telemetry.cpp — Gửi dữ liệu đến Web UI qua WebSocket   ║
// ║  broadcastTelemetry() : vị trí + tốc độ + mode (10Hz)      ║
// ║  broadcastScan()      : kết quả quét sonar (sau fullScan)   ║
// ║  handleHttpStatus()   : HTTP GET /status → JSON snapshot    ║
// ╚══════════════════════════════════════════════════════════════╝
#include "app_context.h"

#include <ArduinoJson.h>

// Gửi gói telemetry tới tất cả WS clients đang kết nối
// event: chuỗi mô tả sự kiện — "update", "arrived", "wifi_sta_connected", …
void broadcastTelemetry(const char* event) {
    StaticJsonDocument<256> doc;
    doc["type"]  = "telemetry";
    doc["event"] = event;
    doc["x"]     = pose.pose.x;       // Tọa độ X (cm, tương đối điểm khởi động)
    doc["y"]     = pose.pose.y;       // Tọa độ Y (cm)
    doc["yaw"]   = currentYawDeg();   // Góc hướng (độ)
    doc["spdL"]  = enc.speedL_cms;    // Tốc độ bánh trái (cm/s)
    doc["spdR"]  = enc.speedR_cms;    // Tốc độ bánh phải (cm/s)
    doc["temp"]  = currentTempC();    // Nhiệt độ (°C) — placeholder
    doc["imu"]   = false;             // Chưa có IMU — để Web UI biết
    doc["mode"]  = modeStr();         // Chế độ hiện tại

    String out;
    serializeJson(doc, out);
    wss.broadcastTXT(out);
}

// Gửi kết quả fullScan (7 điểm, 0°→180°) sau khi sonar.fullScan() chạy xong
void broadcastScan() {
    StaticJsonDocument<512> doc;
    doc["type"] = "scan";

    JsonArray a = doc.createNestedArray("angles");  // Mảng góc (độ)
    JsonArray d = doc.createNestedArray("dists");   // Mảng khoảng cách (cm)
    for (int i = 0; i < SCAN_STEPS; i++) {
        a.add(sonar.last.angle[i]);
        d.add(sonar.last.dist[i]);
    }

    String out;
    serializeJson(doc, out);
    wss.broadcastTXT(out);
}

// Xử lý HTTP GET /status — trả snapshot JSON cho client polling
void handleHttpStatus() {
    StaticJsonDocument<128> doc;
    doc["mode"] = modeStr();
    doc["x"]    = pose.pose.x;
    doc["y"]    = pose.pose.y;
    doc["yaw"]  = currentYawDeg();
    doc["temp"] = currentTempC();
    doc["imu"]  = false;

    String out;
    serializeJson(doc, out);
    http.send(200, "application/json", out);
}
