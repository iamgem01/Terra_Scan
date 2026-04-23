#pragma once
#include <Arduino.h>
#include <math.h>
#include "config.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/pose.h — Pose Estimation                               ║
// ║  Dead-reckoning encoder                                      ║
// ╚══════════════════════════════════════════════════════════════╝

struct Pose {
    float x     = 0.0f;  // cm (tương đối so với điểm khởi động)
    float y     = 0.0f;  // cm
    float theta = 0.0f;  // radian  (-π … +π)
};

class PoseEstimator {
public:
    Pose pose;

    // Gọi mỗi control loop (20ms)
    // deltaL, deltaR : encoder ticks từ lần trước
    void update(int deltaL, int deltaR) {
        const float mmPerTick = (PI * WHEEL_DIAM_MM) / TICKS_PER_REV;
        float dL = deltaL * mmPerTick;
        float dR = deltaR * mmPerTick;
        float dc = (dL + dR) * 0.5f;        // mm đi được trung tâm

        // Góc thay đổi theo encoder (dead-reckoning)
        float dTheta = (dR - dL) / WHEEL_BASE_MM;

        pose.theta += dTheta;
        while (pose.theta >  PI) pose.theta -= TWO_PI;
        while (pose.theta < -PI) pose.theta += TWO_PI;

        // mm → cm (/10)
        pose.x += (dc / 10.0f) * cosf(pose.theta);
        pose.y += (dc / 10.0f) * sinf(pose.theta);
    }

    void reset() { pose = Pose(); }

    float distTo(float tx, float ty) const {
        float dx = tx - pose.x, dy = ty - pose.y;
        return sqrtf(dx * dx + dy * dy);
    }

    // Heading error đến điểm (tx, ty) — radian, [-π, π]
    float headingErrTo(float tx, float ty) const {
        float target = atan2f(ty - pose.y, tx - pose.x);
        float err    = target - pose.theta;
        while (err >  PI) err -= TWO_PI;
        while (err < -PI) err += TWO_PI;
        return err;
    }
};
