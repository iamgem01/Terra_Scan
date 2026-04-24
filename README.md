# IOT mini project
 Robot thám hiểm thông minh. Khối Phần cứng : xây dựng mô hình 
• Chức năng: Thu thập dữ liệu (Siêu âm, DHT11, Ánh sáng) và thực thi lệnh cho động cơ
• IoT: Đóng gói dữ liệu thô gửi lên Cloud qua Wifi; không xử lý thuật toán nặng 
• An toàn: Tự động dừng toàn bộ hệ thống nếu mất kết nối và điều khiển thủ công bằng joystick
2. Khối Phần mềm : 
• Lưu trữ & Hiển thị: Quản lý Cơ sở dữ liệu và cung cấp Dashboard điều khiển trực quan.
• Xử lý thông minh: Chạy thuật toán tìm đường và lập bản đồ 
• AI : Phân tích rủi ro môi trường và chuyển đổi lệnh ngôn ngữ tự nhiên thành hành động 
Nguyên liệu:

- 1 mạch ESP32
- Module L298N
- 2 động cơ DC + 2 bánh xe + công tắc nguồn + động cơ servo + động cơ bước
- 1 bánh xe đa hướng ( bánh xe mắt trâu)
- 1 cảm biến siêu âm + 1 động cơ servo + cảm biến nhiệt độ độ ẩm  + joystich + cảm biến ánh sáng
- 1 pin sạc: pin sạc 12v
- 1 Mạch hạ áp DC-DC LM2596
- 1 khung xe
- Khác: buloong, ốc vít, tán, dây rút, dây nối Dupont

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
