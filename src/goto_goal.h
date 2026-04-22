#pragma once
#include <Arduino.h>
#include <math.h>
#include "config.h"
#include "motor.h"
#include "pose.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/goto_goal.h — Điều hướng đến điểm đích                 ║
// ║  Dùng pose estimation + heading correction                  ║
// ╚══════════════════════════════════════════════════════════════╝

class GotoGoal {
public:
    bool  active   = false;
    bool  arrived  = false;
    float targetX  = 0;
    float targetY  = 0;

    void set(float x, float y) {
        targetX  = x; targetY = y;
        active   = true;
        arrived  = false;
    }

    void cancel() { active = false; stopMotors(); }

    // Gọi mỗi control loop khi active=true
    // pose: vị trí hiện tại từ PoseEstimator
    // Trả về true khi đã tới nơi
    bool tick(const Pose& pose) {
        if (!active) return false;

        float dist  = sqrtf(powf(targetX - pose.x, 2) +
                            powf(targetY - pose.y, 2));
        float angle = _headingErr(pose);

        // Đã đến đích
        if (dist < GOAL_RADIUS_CM) {
            stopMotors();
            active  = false;
            arrived = true;
            return true;
        }

        // Tốc độ tiến: giảm dần khi gần (proportional)
        float vBase = constrain(dist * 3.0f, (float)SPEED_DEADZONE,
                                             (float)SPEED_DEFAULT);

        // Correction lái: tỉ lệ với heading error
        float correction = HEADING_KP * (angle / PI);
        correction = constrain(correction, -110.0f, 110.0f);

        int sL = constrain((int)(vBase - correction), -255, 255);
        int sR = constrain((int)(vBase + correction), -255, 255);

        driveRaw(sL, sR);
        return false;
    }

private:
    float _headingErr(const Pose& p) {
        float target = atan2f(targetY - p.y, targetX - p.x);
        float err    = target - p.theta;
        while (err >  PI) err -= TWO_PI;
        while (err < -PI) err += TWO_PI;
        return err;
    }
};
