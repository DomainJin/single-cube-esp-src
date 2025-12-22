#include "motor.h"

// ============================================
// ENCODER DEBUG - Chỉ hiển thị khi có thay đổi
// ============================================
// Dùng để debug encoder khi robot bị ngoại lực tác động
// mà chưa có lệnh điều khiển motor

void debugEncoderContinuous() {
    static unsigned long last_debug = 0;
    static long last_enc1 = 0, last_enc2 = 0, last_enc3 = 0;
    
    // Kiểm tra mỗi 200ms
    if (millis() - last_debug < 200) {
        return;
    }
    last_debug = millis();
    
    // Đọc encoder count của cả 3 motor
    long enc1 = getEncoderCount(motor1);
    long enc2 = getEncoderCount(motor2);
    long enc3 = getEncoderCount(motor3);
    
    // Chỉ in khi CÓ THAY ĐỔI
    if (enc1 != last_enc1 || enc2 != last_enc2 || enc3 != last_enc3) {
        int dir1 = getEncoderDirection(motor1);
        int dir2 = getEncoderDirection(motor2);
        int dir3 = getEncoderDirection(motor3);
        
        Serial.printf("[ENC_DEBUG] M1:%ld(%s) M2:%ld(%s) M3:%ld(%s)\n",
                      enc1, (dir1 > 0 ? "FWD" : dir1 < 0 ? "BWD" : "---"),
                      enc2, (dir2 > 0 ? "FWD" : dir2 < 0 ? "BWD" : "---"),
                      enc3, (dir3 > 0 ? "FWD" : dir3 < 0 ? "BWD" : "---"));
        
        // Cập nhật giá trị cũ
        last_enc1 = enc1;
        last_enc2 = enc2;
        last_enc3 = enc3;
    }
}
