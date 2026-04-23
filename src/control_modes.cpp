#include "app_context.h"

#include "motor.h"

void doAvoid() {
    if (sonar.obstacleAhead()) {
        stopMotors();
        delay(150);
        sonar.fullScan();
        broadcastScan();

        int turn = sonar.turnAngleDeg();
        if (abs(turn) < 15) {
            driveRaw(-SPEED_TURN, -SPEED_TURN);
            delay(300);
        }

        int turnMs = map(abs(turn), 0, 90, 0, 500);
        if (turn > 0) {
            driveRaw(-SPEED_TURN, SPEED_TURN);
        } else {
            driveRaw(SPEED_TURN, -SPEED_TURN);
        }
        delay(turnMs);
        stopMotors();
        delay(100);
    } else {
        driveRaw(SPEED_DEFAULT, SPEED_DEFAULT);
    }
}
