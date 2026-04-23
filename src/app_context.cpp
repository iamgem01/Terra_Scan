#include "app_context.h"

EncoderManager enc;
PoseEstimator pose;
SonarScanner sonar;
LCDDisplay lcd;
GotoGoal nav;

WebServer http(80);
WebSocketsServer wss(81);

Mode mode = MANUAL;
int cmdL = 0;
int cmdR = 0;
uint8_t wsClient = 0xFF;

uint32_t tCtrl = 0;
uint32_t tTele = 0;
uint32_t tDisp = 0;
uint32_t tScan = 0;
uint32_t tDiag = 0;
