#pragma once
#include <Wire.h>
#include "config.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/mpu.h — MPU6050 Driver (bare I2C, không thư viện ngoài)║
// ║  Đọc gyro Z → tích lũy Yaw   |   I2C addr 0x68             ║
// ║  Dùng chung bus SDA=21, SCL=22 với LCD                      ║
// ╚══════════════════════════════════════════════════════════════╝

class MPU6050Driver {
public:
    float yawDeg    = 0.0f;  // Góc yaw tích lũy (độ)
    float chipTemp  = 0.0f;  // Nhiệt độ chip (°C)
    bool  ok        = false;

    bool begin() {
        // Thức dậy chip
        _write(0x6B, 0x00);
        delay(100);

        // Kiểm tra WHO_AM_I (phải = 0x68)
        uint8_t who = _read1(0x75);
        if (who != 0x68) return false;

        // Gyro range ±250°/s → sensitivity 131 LSB/(°/s)
        _write(0x1B, 0x00);
        // Low-pass filter 44Hz (giảm nhiễu)
        _write(0x1A, 0x03);
        // Sample rate = 1kHz / (1+4) = 200Hz
        _write(0x19, 0x04);

        _calibrate();
        ok = true;
        return true;
    }

    // Gọi mỗi control loop với dt (giây)
    void update(float dt) {
        if (!ok) return;

        // Đọc Gyro Z (2 bytes big-endian)
        int16_t gz = _read16(0x47);
        float dpsZ = (gz - _offsetZ) / 131.0f;   // °/s
        yawDeg += dpsZ * dt;

        // Chuẩn hóa -180…+180
        while (yawDeg >  180.0f) yawDeg -= 360.0f;
        while (yawDeg < -180.0f) yawDeg += 360.0f;

        // Đọc nhiệt độ (mỗi vài giây đã đủ, nhưng thêm đây cho tiện)
        int16_t rawT = _read16(0x41);
        chipTemp = rawT / 340.0f + 36.53f;
    }

    void resetYaw() { yawDeg = 0.0f; }

private:
    float _offsetZ = 0.0f;

    void _calibrate() {
        // Robot phải đứng yên ~2s khi calibrate
        long sum = 0;
        const int N = 300;
        for (int i = 0; i < N; i++) {
            sum += _read16(0x47);
            delay(8);
        }
        _offsetZ = (float)sum / N;
    }

    void _write(uint8_t reg, uint8_t val) {
        Wire.beginTransmission(MPU_I2C_ADDR);
        Wire.write(reg); Wire.write(val);
        Wire.endTransmission();
    }

    uint8_t _read1(uint8_t reg) {
        Wire.beginTransmission(MPU_I2C_ADDR);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom((uint8_t)MPU_I2C_ADDR, (uint8_t)1);
        return Wire.read();
    }

    int16_t _read16(uint8_t reg) {
        Wire.beginTransmission(MPU_I2C_ADDR);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom((uint8_t)MPU_I2C_ADDR, (uint8_t)2);
        int16_t val = ((int16_t)Wire.read() << 8) | Wire.read();
        return val;
    }
};
