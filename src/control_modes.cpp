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
    AV_WAIT_SCAN,   // Chờ scan từ main loop (non-blocking)
    AV_REVERSING,   // Lùi nếu cần
    AV_TURNING,     // Quay tránh vật cản
    AV_SETTLE,      // Dừng ngắn sau khi quay
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
                driveRaw(SPEED_DEFAULT, SPEED_DEFAULT);
                enc.setDirection(SPEED_DEFAULT, SPEED_DEFAULT);
            }
            break;

        case AV_STOPPING:
            if (now - avTimer >= 150) {
                // Clear fresh flag — wait for main loop's LOOP_SCAN_MS to fire
                // a real scan. Avoids the 1.2s blocking fullScan() inside doAvoid().
                sonar.last.fresh = false;
                avState = AV_WAIT_SCAN;
                avTimer = now;
            }
            break;

        case AV_WAIT_SCAN:
            // main loop calls sonar.fullScan() + broadcastScan() every LOOP_SCAN_MS
            // when mode == AVOID. Wait for that scan to complete.
            if (sonar.last.fresh) {
                sonar.last.fresh = false;  // consume
                int turn = sonar.turnAngleDeg();
                if (abs(turn) < 15) {
                    driveRaw(-SPEED_TURN, -SPEED_TURN);
                    enc.setDirection(-SPEED_TURN, -SPEED_TURN);
                    avState = AV_REVERSING;
                    avTimer = now;
                    avTurnDir = 0;
                } else {
                    avTurnMs = map(abs(turn), 0, 90, 0, 500);
                    avTurnDir = (turn > 0) ? 1 : -1;
                    if (avTurnDir > 0) {
                        driveRaw(-SPEED_TURN, SPEED_TURN);
                        enc.setDirection(-SPEED_TURN, SPEED_TURN);
                    } else {
                        driveRaw(SPEED_TURN, -SPEED_TURN);
                        enc.setDirection(SPEED_TURN, -SPEED_TURN);
                    }
                    avState = AV_TURNING;
                    avTimer = now;
                }
            }
            break;

        case AV_REVERSING:
            if (now - avTimer >= 300) {
                stopMotors();
                int turn = sonar.turnAngleDeg();
                avTurnMs = map(abs(turn), 0, 90, 150, 500);
                avTurnDir = (turn >= 0) ? 1 : -1;
                if (avTurnDir > 0) {
                    driveRaw(-SPEED_TURN, SPEED_TURN);
                    enc.setDirection(-SPEED_TURN, SPEED_TURN);
                } else {
                    driveRaw(SPEED_TURN, -SPEED_TURN);
                    enc.setDirection(SPEED_TURN, -SPEED_TURN);
                }
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