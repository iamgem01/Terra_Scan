#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#include "encoder.h"
#include "pose.h"
#include "sonar.h"
#include "display.h"
#include "goto_goal.h"

enum Mode { MANUAL, AVOID, GOTO, SCAN };

extern EncoderManager enc;
extern PoseEstimator pose;
extern SonarScanner sonar;
extern LCDDisplay lcd;
extern GotoGoal nav;

extern WebServer http;
extern WebSocketsServer wss;

extern Mode mode;
extern int cmdL;
extern int cmdR;
extern uint8_t wsClient;

extern uint32_t tCtrl;
extern uint32_t tTele;
extern uint32_t tDisp;
extern uint32_t tScan;
extern uint32_t tDiag;

void handleHttpStatus();
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len);
void doAvoid();
void broadcastTelemetry(const char* event);
void broadcastScan();
const char* modeStr();
void logRuntimeDiag();
float currentYawDeg();
float currentTempC();
