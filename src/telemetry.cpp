#include "app_context.h"

#include <ArduinoJson.h>

void broadcastTelemetry(const char* event) {
    StaticJsonDocument<256> doc;
    doc["type"] = "telemetry";
    doc["event"] = event;
    doc["x"] = pose.pose.x;
    doc["y"] = pose.pose.y;
    doc["yaw"] = currentYawDeg();
    doc["spdL"] = enc.speedL_cms;
    doc["spdR"] = enc.speedR_cms;
    doc["temp"] = currentTempC();
    doc["imu"] = false;
    doc["mode"] = modeStr();

    String out;
    serializeJson(doc, out);
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

    String out;
    serializeJson(doc, out);
    wss.broadcastTXT(out);
}

void handleHttpStatus() {
    StaticJsonDocument<128> doc;
    doc["mode"] = modeStr();
    doc["x"] = pose.pose.x;
    doc["y"] = pose.pose.y;
    doc["yaw"] = currentYawDeg();
    doc["temp"] = currentTempC();
    doc["imu"] = false;

    String out;
    serializeJson(doc, out);
    http.send(200, "application/json", out);
}
