# Smart Car - Pin Connection (ESP32)

## L298N -> ESP32
- ENA -> GPIO14
- IN1 -> GPIO13
- IN2 -> GPIO23
- ENB -> GPIO25
- IN3 -> GPIO26
- IN4 -> GPIO27

## Encoder -> ESP32
- Encoder Trai OUT -> GPIO34
- Encoder Phai OUT -> GPIO35
- VCC -> 5V
- GND -> GND

## HC-SR04 + Servo -> ESP32
- TRIG -> GPIO33
- ECHO -> GPIO18
- Servo -> GPIO19
- VCC -> 5V
- GND -> GND

## LCD I2C -> ESP32
- SDA -> GPIO21
- SCL -> GPIO22
- VCC -> 5V
- GND -> GND

## Luu y
- Tat ca module can noi chung GND voi ESP32.
- Neu dung HC-SR04 voi ESP32, nen dung chia ap cho chan ECHO de dua muc 5V xuong 3.3V an toan cho GPIO.
