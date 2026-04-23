#include "app_context.h"

#include <ArduinoJson.h>

#include "motor.h"

void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
    if (type == WStype_CONNECTED) {
        wsClient = num;
        Serial.printf("[WS] Client #%d connected\n", num);
        return;
    }
    if (type == WStype_DISCONNECTED) {
        Serial.printf("[WS] Client #%d disconnected\n", num);
        return;
    }
    if (type != WStype_TEXT) {
        return;
    }

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, payload, len) != DeserializationError::Ok) {
        return;
    }

    const char* cmd = doc["cmd"] | "";
    Serial.printf("[WS] RX cmd=%s len=%u\n", cmd, (unsigned int)len);

    if (strcmp(cmd, "drive") == 0) {
        mode = MANUAL;
        cmdL = constrain((int)doc["l"], -SPEED_MAX, SPEED_MAX);
        cmdR = constrain((int)doc["r"], -SPEED_MAX, SPEED_MAX);
        enc.setDirection(cmdL, cmdR);
        Serial.printf("[CMD] drive L=%d R=%d\n", cmdL, cmdR);
    } else if (strcmp(cmd, "stop") == 0) {
        mode = MANUAL;
        cmdL = 0;
        cmdR = 0;
        stopMotors();
        Serial.println("[CMD] stop");
    } else if (strcmp(cmd, "goto") == 0) {
        float gx = doc["x"] | 0.0f;
        float gy = doc["y"] | 0.0f;
        nav.set(gx, gy);
        mode = GOTO;
        Serial.printf("[GoTo] Target: (%.1f, %.1f)\n", gx, gy);
    } else if (strcmp(cmd, "mode") == 0) {
        const char* m = doc["mode"] | "manual";
        if (strcmp(m, "avoid") == 0) {
            mode = AVOID;
            Serial.println("[Mode] AVOID");
        } else if (strcmp(m, "manual") == 0) {
            mode = MANUAL;
            cmdL = 0;
            cmdR = 0;
            stopMotors();
            Serial.println("[Mode] MANUAL");
        } else if (strcmp(m, "scan") == 0) {
            stopMotors();
            sonar.fullScan();
            broadcastScan();
            mode = MANUAL;
            Serial.println("[Mode] SCAN -> MANUAL");
        }
    } else if (strcmp(cmd, "reset_pose") == 0) {
        pose.reset();
        enc.reset();
        Serial.println("[Pose] Reset");
    } else if (strcmp(cmd, "servo") == 0) {
        int pan = constrain((int)doc["pan"], 0, 180);
        sonar.setAngle(pan);
    } else if (strcmp(cmd, "path") == 0) {
        Serial.println("[Path] Received waypoints from web BFS");
    }
}
