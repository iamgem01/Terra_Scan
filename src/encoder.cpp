// ╔══════════════════════════════════════════════════════════════╗
// ║  src/encoder.cpp — Định nghĩa biến volatile & ISR encoder   ║
// ║                                                              ║
// ║  Tại sao cần file riêng thay vì định nghĩa trong header?    ║
// ║  Nếu dùng "static" trong header, mỗi .cpp include sẽ có    ║
// ║  bản sao riêng của biến. ISR trong main.cpp cập nhật       ║
// ║  _tickL của main.cpp; setDirection() từ ws_handlers.cpp     ║
// ║  cập nhật _dirL của ws_handlers.cpp → ISR không bao giờ    ║
// ║  thấy hướng thay đổi → tick luôn = 0 → odometry chết.     ║
// ║  Giải pháp: "extern" trong header + định nghĩa DUY NHẤT    ║
// ║  tại đây → tất cả TU dùng chung 1 bản biến.               ║
// ╚══════════════════════════════════════════════════════════════╝
#include "encoder.h"

// Bộ đếm xung — tăng/giảm theo hướng chạy của bánh
volatile long           _tickL = 0;  // Tổng tick bánh trái
volatile long           _tickR = 0;  // Tổng tick bánh phải

// Hướng hiện tại: +1 tiến, -1 lùi, 0 dừng
// Được set bởi enc.setDirection() khi driveRaw() gọi
volatile int8_t         _dirL  = 0;
volatile int8_t         _dirR  = 0;

// Thời điểm cuối cùng đổi hướng (millis)
// Dùng để lọc nhiễu: bỏ qua tick trong SETTLE_MS ms sau khi đổi chiều
volatile unsigned long  _chgL  = 0;
volatile unsigned long  _chgR  = 0;

// ISR phải nằm trong IRAM (không bị truy cập khi flash cache miss)
void IRAM_ATTR isr_encL() {
    // Bỏ tick trong khoảng thời gian ổn định sau khi đổi hướng
    if ((millis() - _chgL) < SETTLE_MS) return;
    // Chỉ tích tick khi biết hướng (tránh đếm sai khi dừng hẳn)
    if (_dirL != 0) _tickL += _dirL;
}
void IRAM_ATTR isr_encR() {
    if ((millis() - _chgR) < SETTLE_MS) return;
    if (_dirR != 0) _tickR += _dirR;
}
