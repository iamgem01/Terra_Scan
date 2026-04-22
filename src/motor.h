#pragma once
#include <Arduino.h>
#include "config.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/motor.h — L298N Dual H-Bridge Driver                   ║
// ║  ENA/ENB = LEDC PWM    IN1-4 = Direction                   ║
// ╚══════════════════════════════════════════════════════════════╝

class Motor {
public:
    Motor(uint8_t in1, uint8_t in2, uint8_t ena, uint8_t pwmCh)
        : _in1(in1), _in2(in2), _ena(ena), _ch(pwmCh) {}

    void begin() {
        pinMode(_in1, OUTPUT);
        pinMode(_in2, OUTPUT);
        ledcSetup(_ch, PWM_FREQ_HZ, PWM_BITS);
        ledcAttachPin(_ena, _ch);
        brake();
    }

    // pwm: -255 (lùi) … 0 (dừng) … +255 (tiến)
    void set(int pwm) {
        pwm = constrain(pwm, -255, 255);
        if (pwm > 0) {
            digitalWrite(_in1, HIGH);
            digitalWrite(_in2, LOW);
            ledcWrite(_ch, (uint8_t)pwm);
        } else if (pwm < 0) {
            digitalWrite(_in1, LOW);
            digitalWrite(_in2, HIGH);
            ledcWrite(_ch, (uint8_t)(-pwm));
        } else {
            brake();
        }
    }

    // Dừng tự do (coast)
    void coast() {
        digitalWrite(_in1, LOW);
        digitalWrite(_in2, LOW);
        ledcWrite(_ch, 0);
    }

    // Dừng cứng (brake) — IN1=IN2=HIGH
    void brake() {
        digitalWrite(_in1, HIGH);
        digitalWrite(_in2, HIGH);
        ledcWrite(_ch, 0);
    }

private:
    uint8_t _in1, _in2, _ena, _ch;
};

// ─── Singleton hai motor ──────────────────────────────────────
// Khởi tạo với đúng GPIO theo sơ đồ
inline Motor& getMotorL() {
    static Motor m(MOTOR_A_IN1, MOTOR_A_IN2, MOTOR_A_ENA, PWM_CH_A);
    return m;
}
inline Motor& getMotorR() {
    static Motor m(MOTOR_B_IN3, MOTOR_B_IN4, MOTOR_B_ENB, PWM_CH_B);
    return m;
}

// ─── Helper toàn cục ─────────────────────────────────────────
inline void motorsBegin() {
    getMotorL().begin();
    getMotorR().begin();
}

inline void driveRaw(int l, int r) {
    getMotorL().set(l);
    getMotorR().set(r);
}

inline void stopMotors() {
    getMotorL().brake();
    getMotorR().brake();
}
