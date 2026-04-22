#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  include/config.h — Toàn bộ cấu hình phần cứng              ║
// ║  Áp dụng đúng sơ đồ mạch của bạn                           ║
// ╚══════════════════════════════════════════════════════════════╝

// ════════════════════════════════════════════════════════════════
//  MOTOR L298N
//  Motor A = bánh TRÁI | Motor B = bánh PHẢI
// ════════════════════════════════════════════════════════════════
#define MOTOR_A_ENA     14    // PWM tốc độ Motor A  ← sơ đồ của bạn
#define MOTOR_A_IN1     13    // Hướng Motor A (pin 1)
#define MOTOR_A_IN2     23    // Hướng Motor A (pin 2)
#define MOTOR_B_ENB     25    // PWM tốc độ Motor B
#define MOTOR_B_IN3     26    // Hướng Motor B (pin 1)
#define MOTOR_B_IN4     27    // Hướng Motor B (pin 2)

// LEDC PWM channels (ESP32 có 16 channels, dùng 0 và 1)
#define PWM_CH_A        0
#define PWM_CH_B        1
#define PWM_FREQ_HZ     5000
#define PWM_BITS        8     // 0–255

// ════════════════════════════════════════════════════════════════
//  ENCODER — GPIO 34 & 35 (input-only, hỗ trợ interrupt)
// ════════════════════════════════════════════════════════════════
#define ENC_LEFT_PIN    34    // Encoder bánh TRÁI
#define ENC_RIGHT_PIN   35    // Encoder bánh PHẢI
#define TICKS_PER_REV   20    // Số xung / vòng bánh
#define WHEEL_DIAM_MM   65.0f // Đường kính bánh (mm)
#define WHEEL_BASE_MM   145.0f// Khoảng cách 2 bánh (mm)
#define SETTLE_MS       20    // ms sau đổi hướng: vẫn tích tick cũ

// ════════════════════════════════════════════════════════════════
//  HC-SR04 ULTRASONIC + SERVO
// ════════════════════════════════════════════════════════════════
#define SONAR_TRIG_PIN  33    // Trig đã đổi dây sang GPIO33
#define SONAR_ECHO_PIN  18    // ← sơ đồ của bạn
#define SERVO_PIN       19    // ← sơ đồ của bạn
#define OBSTACLE_CM     25    // Ngưỡng phát hiện vật cản (cm)
#define SONAR_TIMEOUT   25000 // µs timeout (~4m max)

// ════════════════════════════════════════════════════════════════
//  I2C BUS — dùng chung SDA=21, SCL=22
//  LCD (0x27) + MPU6050 (0x68) trên cùng 1 bus
// ════════════════════════════════════════════════════════════════
#define I2C_SDA         21    // ← sơ đồ của bạn
#define I2C_SCL         22    // ← sơ đồ của bạn
#define LCD_I2C_ADDR    0x27  // LCD 16×2 với PCF8574
#define MPU_I2C_ADDR    0x68  // MPU6050 (AD0=GND)

// ════════════════════════════════════════════════════════════════
//  TỐC ĐỘ & PID
// ════════════════════════════════════════════════════════════════
#define SPEED_MAX       220   // PWM tối đa (tránh quá nóng L298N)
#define SPEED_DEFAULT   160   // Tốc độ mặc định di chuyển
#define SPEED_TURN      120   // Tốc độ khi quay
#define SPEED_DEADZONE  75    // PWM tối thiểu để bánh quay được

// PID Gains cho speed controller (hiệu chỉnh theo thực tế)
#define PID_KP          2.5f
#define PID_KI          0.4f
#define PID_KD          0.08f

// ════════════════════════════════════════════════════════════════
//  POSE ESTIMATION
// ════════════════════════════════════════════════════════════════
#define IMU_WEIGHT      0.30f // Trọng số MPU6050 trong fusion (0–1)
#define GOAL_RADIUS_CM  6.0f  // Bán kính "đã đến đích"
#define HEADING_KP      90.0f // Hệ số nhạy lái (heading correction)

// ════════════════════════════════════════════════════════════════
//  TIMING (ms)
// ════════════════════════════════════════════════════════════════
#define LOOP_CONTROL_MS    20   // Control loop: 50Hz
#define LOOP_TELEMETRY_MS  100  // Gửi dữ liệu WS: 10Hz
#define LOOP_DISPLAY_MS    500  // Cập nhật LCD: 2Hz
#define LOOP_SCAN_MS       2500 // Scan siêu âm khi AVOID/GOTO mode

// ════════════════════════════════════════════════════════════════
//  WIFI AP FALLBACK
// ════════════════════════════════════════════════════════════════
#define AP_SSID     "EzRoverPro"
#define AP_PASSWORD "rover1234"
