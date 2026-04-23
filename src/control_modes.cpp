#include "app_context.h"
#include "motor.h"

// ╔══════════════════════════════════════════════════════════════╗
// ║  src/control_modes.cpp  [FIXED]                             ║
// ║  FIX: Loại bỏ toàn bộ delay() blocking trong doAvoid()     ║
// ║  Dùng state machine để loop() không bị chặn                ║
// ╚══════════════════════════════════════════════════════════════╝

// ─── State machine cho AVOID mode ────────────────────────────
// Trước đây doAvoid() dùng delay() → block loop() hoàn toàn
// Lệnh "drive" từ WebSocket bị set nhưng driveRaw() không được gọi
// cho đến khi delay() kết thúc (có thể mất 300+500+100 = ~900ms)
//
// Bây giờ: mỗi lần doAvoid() được gọi (từ loop() @ 50Hz),
// nó chỉ kiểm tra xem state hiện tại đã hết thời gian chưa
// rồi chuyển sang state tiếp theo — không block.

enum AvoidState {
    AV_CHECK,       // Kiểm tra có vật cản không
    AV_STOPPING,    // Đang dừng lại (ngắn)
    AV_WAIT_SCAN,   // Chờ fullScan() (vẫn blocking nhưng chỉ 1 lần)
    AV_REVERSING,   // Lùi nếu cần
    AV_TURNING,     // Quay tránh vật cản
    AV_SETTLE,      // Dừng ngắn sau khi quay
    AV_FORWARD      // Đi thẳng khi đường trống
};

static AvoidState avState = AV_CHECK;
static uint32_t   avTimer = 0;
static int        avTurnMs = 0;
static int        avTurnDir = 0;  // +1 = trái, -1 = phải

void doAvoid() {
    const uint32_t now = millis();

    switch (avState) {

        case AV_CHECK:
            if (sonar.obstacleAhead()) {
                stopMotors();
                avState = AV_STOPPING;
                avTimer = now;
            } else {
                // Đường trống → đi thẳng ngay, không cần state change
                driveRaw(SPEED_DEFAULT, SPEED_DEFAULT);
            }
            break;

        case AV_STOPPING:
            // *** FIX: thay delay(150) → đợi 150ms qua timer
            if (now - avTimer >= 150) {
                // *** Gọi fullScan() — vẫn blocking ~1.2s (đã fix ở sonar.h)
                //     Đây là blocking duy nhất còn lại, không thể tránh hoàn toàn
                //     vì HC-SR04 cần thời gian thực để đo
                sonar.fullScan();
                broadcastScan();

                // Tính hướng quay ngay sau scan
                int turn = sonar.turnAngleDeg();
                if (abs(turn) < 15) {
                    // Bí hoàn toàn → lùi
                    driveRaw(-SPEED_TURN, -SPEED_TURN);
                    avState = AV_REVERSING;
                    avTimer = now;
                    avTurnDir = 0;
                } else {
                    // Có hướng thoát → quay ngay
                    avTurnMs = map(abs(turn), 0, 90, 0, 500);
                    avTurnDir = (turn > 0) ? 1 : -1;
                    if (avTurnDir > 0)
                        driveRaw(-SPEED_TURN, SPEED_TURN);  // quay trái
                    else
                        driveRaw(SPEED_TURN, -SPEED_TURN);  // quay phải
                    avState = AV_TURNING;
                    avTimer = now;
                }
            }
            break;

        case AV_REVERSING:
            // *** FIX: thay delay(300) → đợi 300ms qua timer
            if (now - avTimer >= 300) {
                stopMotors();
                // Sau khi lùi, tính lại hướng từ scan vừa xong
                int turn = sonar.turnAngleDeg();
                avTurnMs = map(abs(turn), 0, 90, 150, 500);
                avTurnDir = (turn >= 0) ? 1 : -1;
                if (avTurnDir > 0)
                    driveRaw(-SPEED_TURN, SPEED_TURN);
                else
                    driveRaw(SPEED_TURN, -SPEED_TURN);
                avState = AV_TURNING;
                avTimer = now;
            }
            break;

        case AV_TURNING:
            // *** FIX: thay delay(turnMs) → đợi qua timer
            if (now - avTimer >= (uint32_t)avTurnMs) {
                stopMotors();
                avState = AV_SETTLE;
                avTimer = now;
            }
            break;

        case AV_SETTLE:
            // *** FIX: thay delay(100) → đợi 100ms qua timer
            if (now - avTimer >= 100) {
                avState = AV_CHECK;  // Quay lại kiểm tra
            }
            break;

        default:
            avState = AV_CHECK;
            break;
    }
}

// Reset state machine khi thoát AVOID mode
void resetAvoidState() {
    avState = AV_CHECK;
    avTimer = 0;
}