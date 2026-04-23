#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>
#include "config.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/sonar.h — HC-SR04 Ultrasonic + Servo Sweep  [FIXED]   ║
// ║  FIX 1: servo.detach() sau khi đến vị trí → hết nóng      ║
// ║  FIX 2: fullScan() dùng delay ngắn hơn (150ms/step)       ║
// ╚══════════════════════════════════════════════════════════════╝

#define SCAN_STEPS   7      // 7 bước: 0°,30°,60°,90°,120°,150°,180°
#define SCAN_STEP    30
// *** FIX: giảm delay mỗi bước từ 220ms → 150ms
//     Servo SG90 thực tế chỉ cần ~100-120ms để quay 30°
//     Tổng fullScan: 7×150ms + 150ms faceForward = ~1.2s (trước là ~1.7s)
#define SERVO_SETTLE_MS  150

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
        ESP32PWM::allocateTimer(2);
        servo.setPeriodHertz(50);
        servo.attach(SERVO_PIN, 500, 2400);

        pinMode(SONAR_TRIG_PIN, OUTPUT);
        pinMode(SONAR_ECHO_PIN, INPUT);

        _faceForward();
        // *** FIX 1: detach ngay sau begin() để servo không giữ PWM liên tục
        //     Servo đã ở 90° → không cần tín hiệu nữa → hết tỏa nhiệt
        _safeDetach();
    }

    // Đo khoảng cách tại góc servo hiện tại (cm)
    float measureCm() {
        digitalWrite(SONAR_TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(SONAR_TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(SONAR_TRIG_PIN, LOW);
        long dur = pulseIn(SONAR_ECHO_PIN, HIGH, SONAR_TIMEOUT);
        return (dur == 0) ? 300.0f : (dur * 0.0171f);
    }

    // Kiểm tra nhanh phía trước (không quay servo)
    bool obstacleAhead() {
        // *** FIX: _faceForward() chỉ re-attach nếu servo đang detached
        if (currentAngle != 90) {
            _attachAndMove(90);
            _safeDetach();
        }
        return measureCm() < (float)OBSTACLE_CM;
    }

    // Quét toàn bộ 0°→180° — blocking ~1.2s (giảm từ ~1.7s)
    void fullScan() {
        // *** FIX: re-attach trước khi quét, detach sau khi xong
        _attachServo();
        for (int i = 0; i < SCAN_STEPS; i++) {
            uint8_t a = i * SCAN_STEP;
            last.angle[i] = a;
            servo.write(a);
            currentAngle = a;
            delay(SERVO_SETTLE_MS);  // 150ms thay vì 220ms
            last.dist[i] = measureCm();
        }
        last.fresh = true;
        _faceForward();
        _safeDetach();  // *** FIX: detach sau khi scan xong
    }

    // Quét nhanh 3 hướng: trái/trước/phải
    struct Quick3 { float left; float front; float right; };
    Quick3 quickScan() {
        _attachServo();
        servo.write(150); delay(200); float l = measureCm();
        servo.write(90);  delay(150); float f = measureCm();
        servo.write(30);  delay(200); float r = measureCm();
        _faceForward();
        _safeDetach();  // *** FIX: detach sau khi xong
        return { l, f, r };
    }

    // Góc thoáng nhất (distance lớn nhất)
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

    int turnAngleDeg() const { return bestAngle() - 90; }

    void setAngle(int deg) {
        deg = constrain(deg, 0, 180);
        _attachAndMove(deg);
        // *** FIX: detach sau khi servo đã đến vị trí yêu cầu thủ công
        //     Giữ attach khi đang manual pan (user có thể tiếp tục điều chỉnh)
        //     Nếu muốn detach: gọi detachServo() từ ws_handlers sau 1s không có lệnh mới
        currentAngle = deg;
    }

    // Public helper để ws_handlers gọi detach sau khi servo idle
    void detachServo() {
        _safeDetach();
    }

private:
    bool _attached = false;

    void _attachServo() {
        if (!_attached) {
            servo.attach(SERVO_PIN, 500, 2400);
            _attached = true;
        }
    }

    void _safeDetach() {
        if (_attached) {
            servo.detach();
            _attached = false;
        }
    }

    void _faceForward() {
        servo.write(90);
        currentAngle = 90;
        delay(SERVO_SETTLE_MS);
    }

    void _attachAndMove(int deg) {
        _attachServo();
        servo.write(deg);
        currentAngle = deg;
        delay(SERVO_SETTLE_MS);
    }
};