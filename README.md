# EzRover Pro — Tổng Quan Codebase 

> Xe tự hành ESP32 với điều khiển web real-time, tránh vật cản, và điều hướng odometry.

---

## Phần cứng sử dụng

| Linh kiện | Mô tả | Giao tiếp |
|-----------|-------|-----------|
| **ESP32 Dev** | Vi điều khiển chính | — |
| **L298N** | Mạch cầu H kép điều khiển 2 động cơ DC | GPIO (PWM + DIR) |
| **LM393 Encoder** | Đọc xung quang học bánh xe | GPIO 34 (trái), 35 (phải) — interrupt |
| **HC-SR04** | Cảm biến siêu âm đo khoảng cách | GPIO Trig/Echo |
| **SG90 Servo** | Xoay cảm biến sonar 0°→180° | GPIO PWM (LEDC) |
| **LCD 16×2** | Hiển thị mode, tốc độ, IP | I2C (SDA=21, SCL=22, addr 0x27) |
| **WiFi** | Kết nối AP nội bộ + STA home WiFi | ESP32 built-in |

---

## Cấu trúc file

```
src/
├── main.cpp           — setup() + loop() chính, các timer non-blocking
├── config.h           — TẤT CẢ hằng số GPIO, PWM, PID, timing
├── app_context.h/.cpp — Shared state toàn cục, khai báo extern
├── motor.h            — Driver L298N qua LEDC PWM
├── encoder.h/.cpp     — Đọc encoder bằng interrupt RISING
├── pose.h             — Dead-reckoning odometry (x, y, θ)
├── sonar.h            — HC-SR04 + servo quét
├── goto_goal.h        — Bộ điều hướng đến tọa độ (x, y)
├── display.h          — LCD I2C
├── webui.h            — HTML/JS web UI nhúng vào firmware
├── ws_handlers.cpp    — Parser lệnh WebSocket từ web
├── telemetry.cpp      — Gửi JSON telemetry + scan qua WebSocket
├── control_modes.cpp  — State machine AVOID (tránh vật cản)
├── runtime_utils.cpp  — logRuntimeDiag(), currentYawDeg(), currentTempC()
└── wifi_credentials.h — SSID/Pass (gitignored, tạo thủ công)
```

**Thay đổi cấu hình phần cứng:** chỉ cần sửa `config.h` — không cần động vào logic.

---

## Vòng lặp chính (`main.cpp`)

`loop()` chạy **4 timer non-blocking** song song, không dùng `delay()`:

```
┌──────────────────────────────────────────────────────────────┐
│  loop() — mỗi iteration ~< 1ms                               │
│                                                               │
│  50 Hz  (20ms)  ──► đọc encoder → cập nhật pose → dispatch mode │
│  10 Hz (100ms)  ──► broadcast telemetry JSON qua WebSocket    │
│   2 Hz (500ms)  ──► refresh LCD (mode, tốc độ, góc, IP)       │
│ 0.4 Hz (2500ms) ──► sonar.fullScan() (chỉ khi AVOID/GOTO)    │
│                                                               │
│  + HTTP handleClient()  + WebSocket loop()  + servo autoDetach│
└──────────────────────────────────────────────────────────────┘
```

---

## Luồng dữ liệu phần cứng

### 1. Động cơ — `motor.h`

```
WebSocket "drive" cmd
        │
        ▼
ws_handlers.cpp
  cmdL, cmdR = ±255
        │
        ▼
driveRaw(L, R) — motor.h
        │
        ├── Motor A: IN1/IN2 (chiều quay) + ENA (LEDC PWM duty)
        └── Motor B: IN3/IN4 (chiều quay) + ENB (LEDC PWM duty)
                │
                ▼
           L298N → 2 motor DC bánh xe
```

- `pwm > 0` → tiến: IN1=HIGH, IN2=LOW  
- `pwm < 0` → lùi: IN1=LOW, IN2=HIGH  
- `pwm = 0` → brake: IN1=IN2=HIGH, duty=0  

### 2. Encoder — `encoder.h` + `encoder.cpp`

```
Bánh xe quay → LM393 tạo xung RISING
                    │
         attachInterrupt(GPIO34/35)
                    │
         ISR: _tickL++ / _tickR++  (IRAM_ATTR)
                    │
         EncoderManager.update()  @ 50Hz
                    │
         ├── speedL_cms / speedR_cms  (cm/s)
         └── getAndResetDelta(dL, dR) → PoseEstimator
```

**Hướng quay** được track qua `setDirection()`: ISR chỉ cộng tick, motor.h ghi chiều vào `_dirL/_dirR`.

### 3. Odometry / Pose — `pose.h`

```
deltaL, deltaR (ticks)
        │
        ▼
mmPerTick = π × WHEEL_DIAM_MM / TICKS_PER_REV

dL = deltaL × mmPerTick   (mm bánh trái)
dR = deltaR × mmPerTick   (mm bánh phải)

dTheta = (dR - dL) / WHEEL_BASE_MM   (radian)
dc     = (dL + dR) / 2               (mm đi trung tâm)

pose.theta += dTheta
pose.x += (dc/10) × cos(theta)   (cm)
pose.y += (dc/10) × sin(theta)   (cm)
```

Đây là **dead-reckoning** — tích lũy sai số theo thời gian, không có IMU bù.

### 4. Sonar + Servo — `sonar.h`

```
sonar.fullScan():
  servo.attach() → quét 7 góc (0°,30°,60°...180°)
  Mỗi góc:
    Trig HIGH 10µs → đo pulseIn(Echo) → dist = dur × 0.0171 cm
    delay(150ms) chờ servo đến vị trí
  → ScanResult last {angle[7], dist[7], fresh=true}
  servo.detach()  ← tránh servo nóng khi idle
```

`obstacleAhead()` — đo nhanh thẳng trước (servo giữ 90°), không quét.

### 5. LCD — `display.h`

```
I2C bus (SDA=21, SCL=22)
  └── LiquidCrystal_I2C addr 0x27
        │
        ├── show(l1, l2)         — 2 dòng tùy ý
        ├── showIP(ip)           — sau khi WiFi STA connect
        └── updateMain(mode, speed, yaw, ip)
              Dòng 1: "AVOID  192.168.1.5"
              Dòng 2: "S:145cm  Y:32°"
```

---

## Chế độ hoạt động (`Mode` enum)

### MANUAL
```
Web UI gửi: {"cmd":"drive","l":180,"r":180}
    │
    ▼
ws_handlers → cmdL=180, cmdR=180, mode=MANUAL
    │
    ▼
loop() @ 50Hz → driveRaw(cmdL, cmdR)
```
WebSocket handler còn gọi `driveRaw()` ngay lập tức (không chờ 20ms timer) để giảm latency.

### AVOID (Tránh vật cản)

State machine 6 trạng thái, chạy @ 50Hz — **không có `delay()` nào**:

```
AV_CHECK
  │ sonar.obstacleAhead() == true
  ▼
AV_STOPPING ──(150ms)──► AV_WAIT_SCAN
                               │ chờ sonar.last.fresh = true
                               │ (fullScan() chạy từ main loop @ 0.4Hz)
                               ▼
                         AV_REVERSING ──(300ms)──► AV_TURNING
                         hoặc AV_TURNING (nếu có góc quay)
                                                │
                                                │ (turnMs ms)
                                                ▼
                                          AV_SETTLE ──(100ms)──► AV_CHECK
```

`turnAngleDeg()` tính góc quay từ khoảng cách tốt nhất trong `ScanResult`.

### GOTO (Điều hướng đến tọa độ)

```
Web UI gửi: {"cmd":"goto","x":100,"y":50}  (cm)
    │
    ▼
nav.set(100, 50)  →  mode = GOTO
    │
    ▼
loop() @ 50Hz: nav.tick(pose.pose)
    │
    ├── dist = khoảng cách đến (x,y)
    ├── angle = headingErrTo(x,y)  (radian)
    │
    ├── dist < GOAL_RADIUS_CM → stopMotors(), mode=MANUAL, broadcast "arrived"
    │
    └── vBase = min(dist × 3, SPEED_DEFAULT)   (tỉ lệ khoảng cách)
        correction = HEADING_KP × (angle/π)    (tỉ lệ heading error)
        driveRaw(vBase - correction, vBase + correction)
```

### SCAN
Đứng yên. `ws_handlers` gọi `sonar.fullScan()` + `broadcastScan()` trực tiếp, sau đó về MANUAL.

---

## WiFi & Web

```
ESP32 khởi động:
  WiFi.mode(WIFI_AP_STA)
  softAP("EzRoverPro", "rover1234")  ← luôn sẵn sàng
  WiFi.begin(WIFI_SSID, WIFI_PASS)   ← không blocking

HTTP port 80:
  GET /       → HTML_PAGE (webui.h nhúng sẵn)
  GET /status → JSON snapshot (mode, x, y, yaw, temp)

WebSocket port 81:
  Nhận:  {"cmd":"drive","l":L,"r":R}
         {"cmd":"stop"}
         {"cmd":"goto","x":X,"y":Y}
         {"cmd":"mode","mode":"avoid|manual|scan"}
         {"cmd":"servo","pan":angle}
         {"cmd":"reset_pose"}
  Gửi:   {"type":"telemetry","x":...,"y":...,"yaw":...,...}  @ 10Hz
         {"type":"scan","angles":[...],"dists":[...]}        sau fullScan
```

STA kết nối xong → `lcd.showIP()` cập nhật + broadcast `"wifi_sta_connected"` qua WS.

---

## Thư viện (platformio.ini)

| Thư viện | Dùng cho |
|----------|----------|
| `WebSockets` (links2004) | WebSocket server port 81 |
| `ArduinoJson` (bblanchon) | Parse/serialize JSON commands & telemetry |
| `ESP32Servo` (madhephaestus) | Servo trên ESP32 (LEDC timer) |
| `LiquidCrystal_I2C` | LCD I2C |

---

## Sơ đồ tổng thể

```
                    ┌───────────-──────────────────────┐
  Browser           │         ESP32 (EzRover Pro)      │
  Web UI   ◄──WS──► │  ws_handlers.cpp                 │
  (port 81)         │        │                         │
                    │        ▼                         │
                    │  app_context (shared state)      │
                    │  mode / cmdL / cmdR / pose       │
                    │        │                         │
                    │  loop() 50Hz dispatcher          │
                    │  ├─ MANUAL → driveRaw(cmdL,cmdR) │
                    │  ├─ AVOID  → doAvoid() SM        │
                    │  └─ GOTO   → nav.tick(pose)      │
                    │        │                         │
                    │  ┌─────┴──────────────────────┐  │
                    │  │  Hardware Layer            │  │
                    │  │  L298N ← motor.h (PWM)     │  │
                    │  │  LM393 → encoder (ISR)     │  │
                    │  │  HC-SR04 ← sonar.h         │  │
                    │  │  SG90   ← servo (LEDC)     │  │
                    │  │  LCD    ← display (I2C)    │  │
                    │  └────────────────────────────┘  │
                    └-─────────────────────────────────┘
```

---

## Ghi chú quan trọng

1. **`wifi_credentials.h` không có trong repo** — phải tạo thủ công:
   ```cpp
   #pragma once
   const char* WIFI_SSID = "your_ssid";
   const char* WIFI_PASS = "your_password";
   ```

2. **Servo tự detach** sau 2s idle (`checkServoAutoDetach()` gọi từ `loop()`) để tránh nóng.

3. **`fullScan()` blocking ~1.2s** — được gọi từ main loop @ 0.4Hz, không từ bên trong `doAvoid()`, để không block WebSocket và telemetry.

4. **Encoder dùng GPIO 34/35** — input-only pins của ESP32, không có internal pull-up, cần pull-up ngoài hoặc từ module LM393.

5. **Odometry tích lũy sai số** — không có IMU, `reset_pose` để reset về (0,0,0°).
