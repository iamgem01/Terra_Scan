#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "config.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/sonar.h — HC-SR04 Ultrasonic + Servo Sweep             ║
// ║  Trig=GPIO5  Echo=GPIO18  Servo=GPIO19                      ║
// ╚══════════════════════════════════════════════════════════════╝

#define SCAN_STEPS   7      // 7 bước: 0°,30°,60°,90°,120°,150°,180°
#define SCAN_STEP    30

struct ScanResult {
    uint8_t angle[SCAN_STEPS];
    float   dist[SCAN_STEPS];
    bool    fresh = false;
};

class SonarScanner {
public:
    Servo      servo;
    ScanResult last;
    int        currentAngle = 90;

    void begin() {
        // ESP32Servo cần allocate timer trước khi attach
        ESP32PWM::allocateTimer(2);
        servo.setPeriodHertz(50);
        servo.attach(SERVO_PIN, 500, 2400);

        pinMode(SONAR_TRIG_PIN, OUTPUT);
        pinMode(SONAR_ECHO_PIN, INPUT);

        _faceForward();
    }

    // Đo khoảng cách tại góc servo hiện tại (cm)
    float measureCm() {
        digitalWrite(SONAR_TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(SONAR_TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(SONAR_TRIG_PIN, LOW);
        long dur = pulseIn(SONAR_ECHO_PIN, HIGH, SONAR_TIMEOUT);
        return (dur == 0) ? 300.0f : (dur * 0.0171f); // 0.034/2
    }

    // Kiểm tra nhanh phía trước (không quay servo)
    bool obstacleAhead() {
        if (currentAngle != 90) _faceForward();
        return measureCm() < (float)OBSTACLE_CM;
    }

    // Quét toàn bộ 0°→180° — blocking ~2s
    // Gọi khi robot đứng yên
    void fullScan() {
        for (int i = 0; i < SCAN_STEPS; i++) {
            uint8_t a = i * SCAN_STEP;
            last.angle[i] = a;
            servo.write(a);
            delay(220);
            last.dist[i] = measureCm();
        }
        last.fresh = true;
        _faceForward();
    }

    // Quét nhanh 3 hướng: trái/trước/phải
    struct Quick3 { float left; float front; float right; };
    Quick3 quickScan() {
        servo.write(150); delay(200); float l = measureCm();
        servo.write(90);  delay(150); float f = measureCm();
        servo.write(30);  delay(200); float r = measureCm();
        _faceForward();
        return { l, f, r };
    }

    // Góc thoáng nhất (distance lớn nhất) trong lần scan cuối
    int bestAngle() const {
        if (!last.fresh) return 90;
        int best = 90; float maxD = 0;
        for (int i = 0; i < SCAN_STEPS; i++) {
            if (last.dist[i] > maxD) {
                maxD = last.dist[i];
                best = last.angle[i];
            }
        }
        return best;
    }

    // Góc quay robot cần thiết: dương=trái, âm=phải (độ)
    int turnAngleDeg() const { return bestAngle() - 90; }

    void setAngle(int deg) {
        deg = constrain(deg, 0, 180);
        servo.write(deg);
        currentAngle = deg;
    }

private:
    void _faceForward() {
        servo.write(90);
        currentAngle = 90;
        delay(150);
    }
};
