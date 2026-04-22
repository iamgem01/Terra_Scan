#pragma once
#include <Arduino.h>
#include <math.h>
#include "config.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/pose.h — Pose Estimation                               ║
// ║  Dead-reckoning encoder + MPU6050 yaw fusion                ║
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
    // imuYawDeg      : góc yaw tích lũy từ MPU6050 (độ)
    void update(int deltaL, int deltaR, float imuYawDeg) {
        const float mmPerTick = (PI * WHEEL_DIAM_MM) / TICKS_PER_REV;
        float dL = deltaL * mmPerTick;
        float dR = deltaR * mmPerTick;
        float dc = (dL + dR) * 0.5f;        // mm đi được trung tâm

        // Góc thay đổi theo encoder (dead-reckoning)
        float dTheta_enc = (dR - dL) / WHEEL_BASE_MM;

        // Góc thay đổi theo IMU
        float imuRad    = imuYawDeg * DEG_TO_RAD;
        float dTheta_imu = imuRad - pose.theta;
        // Chuẩn hoá [-π, π]
        while (dTheta_imu >  PI) dTheta_imu -= TWO_PI;
        while (dTheta_imu < -PI) dTheta_imu += TWO_PI;

        // Sensor fusion: encoder tốt với khoảng cách, IMU tốt với góc quay
        float dTheta = (1.0f - IMU_WEIGHT) * dTheta_enc
                      +        IMU_WEIGHT  * dTheta_imu;

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
