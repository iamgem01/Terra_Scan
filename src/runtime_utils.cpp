#include "app_context.h"

float currentYawDeg() {
    return pose.pose.theta * RAD_TO_DEG;
}

float currentTempC() {
    return 0.0f;
}

const char* modeStr() {
    switch (mode) {
        case MANUAL: return "MANUAL";
        case AVOID: return "AVOID";
        case GOTO: return "GOTO";
        case SCAN: return "SCAN";
        default: return "?";
    }
}

void logRuntimeDiag() {
    float sonarFront = sonar.measureCm();
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
