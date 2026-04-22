#pragma once
#include <Arduino.h>
#include "config.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/encoder.h — LM393 Optocoupler Encoder                  ║
// ║  GPIO 34 (Trái) & GPIO 35 (Phải) — input-only pins ESP32    ║
// ║  Dùng interrupt RISING để đếm xung                         ║
// ╚══════════════════════════════════════════════════════════════╝

// ─── Biến chia sẻ với ISR (phải volatile) ────────────────────
static volatile long  _tickL     = 0;
static volatile long  _tickR     = 0;
static volatile int8_t _dirL     = 0;   // +1 tiến / -1 lùi / 0 dừng
static volatile int8_t _dirR     = 0;
static volatile unsigned long _chgL = 0; // millis() lúc đổi hướng
static volatile unsigned long _chgR = 0;

// ─── ISR — chạy trực tiếp trong interrupt ────────────────────
void IRAM_ATTR isr_encL() {
    if ((millis() - _chgL) < SETTLE_MS) return; // bỏ qua quán tính
    if (_dirL != 0) _tickL += _dirL;
}
void IRAM_ATTR isr_encR() {
    if ((millis() - _chgR) < SETTLE_MS) return;
    if (_dirR != 0) _tickR += _dirR;
}

// ─── Encoder Manager ─────────────────────────────────────────
class EncoderManager {
public:
    // Tốc độ cm/s (cập nhật bởi update())
    float speedL_cms = 0.0f;
    float speedR_cms = 0.0f;

    // Snapshot cho pose estimation
    long  snapL = 0, snapR = 0;

    void begin() {
        pinMode(ENC_LEFT_PIN,  INPUT);
        pinMode(ENC_RIGHT_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(ENC_LEFT_PIN),  isr_encL, RISING);
        attachInterrupt(digitalPinToInterrupt(ENC_RIGHT_PIN), isr_encR, RISING);
        _lastMs = millis();
    }

    // Gọi mỗi 20ms trong control loop
    void update() {
        unsigned long now = millis();
        float dt = (now - _lastMs) / 1000.0f;
        if (dt < 0.005f) return;

        long curL = _tickL, curR = _tickR; // atomic snapshot
        long dL = curL - _prevL;
        long dR = curR - _prevR;
        _prevL = curL; _prevR = curR;

        float mmPT = (PI * WHEEL_DIAM_MM) / TICKS_PER_REV;
        speedL_cms = (dL * mmPT / 10.0f) / dt;
        speedR_cms = (dR * mmPT / 10.0f) / dt;
        _lastMs = now;
    }

    // Lấy delta ticks kể từ lần gọi getDelta() trước
    // Dùng riêng cho pose estimation (không trùng với update())
    void getAndResetDelta(int& dL, int& dR) {
        long curL = _tickL, curR = _tickR;
        dL = (int)(curL - snapL);
        dR = (int)(curR - snapR);
        snapL = curL; snapR = curR;
    }

    // Thông báo hướng motor (để ISR biết chiều tích tick)
    void setDirection(int pwmL, int pwmR) {
        int8_t newDL = (pwmL > 10) ? 1 : (pwmL < -10) ? -1 : 0;
        int8_t newDR = (pwmR > 10) ? 1 : (pwmR < -10) ? -1 : 0;
        if (newDL != _dirL) { _dirL = newDL; _chgL = millis(); }
        if (newDR != _dirR) { _dirR = newDR; _chgR = millis(); }
    }

    void reset() {
        _tickL = _tickR = 0;
        snapL = snapR = 0;
        _prevL = _prevR = 0;
        speedL_cms = speedR_cms = 0;
    }

    long totalL() const { return _tickL; }
    long totalR() const { return _tickR; }

private:
    long          _prevL = 0, _prevR = 0;
    unsigned long _lastMs = 0;
};
