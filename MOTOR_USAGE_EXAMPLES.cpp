// ============================================
// HƯỚNG DẪN SỬ DỤNG MOTOR CONTROL SYSTEM
// ============================================

#include "motor.h"

// ============================================
// 1. KHỞI TẠO MOTORS (trong setup())
// ============================================
void setup() {
    Serial.begin(115200);
    
    // Khởi tạo motor control system
    setupMotors();  // Tự động config PWM, encoder, interrupts
}

// ============================================
// 2. ĐIỀU KHIỂN MOTOR CỞ BẢN
// ============================================
void example_basic_control() {
    // Motor tiến với tốc độ 150 (0-255)
    setMotorSpeed(motor1, 150, MOTOR_FORWARD);
    delay(2000);
    
    // Motor lùi với tốc độ 100
    setMotorSpeed(motor1, 100, MOTOR_BACKWARD);
    delay(2000);
    
    // Dừng motor (coast stop - quán tính)
    stopMotor(motor1);
    delay(1000);
    
    // Dừng motor (brake - phanh tích cực)
    brakeMotor(motor1);
}

// ============================================
// 3. ĐỌC ENCODER
// ============================================
void example_read_encoder() {
    // Đọc số xung encoder đã đếm được
    long count1 = getEncoderCount(motor1);
    long count2 = getEncoderCount(motor2);
    long count3 = getEncoderCount(motor3);
    
    Serial.printf("Encoder counts: M1=%ld, M2=%ld, M3=%ld\n", 
                  count1, count2, count3);
    
    // Đọc RPM (vòng/phút)
    float rpm1 = getMotorRPM(motor1);
    float rpm2 = getMotorRPM(motor2);
    float rpm3 = getMotorRPM(motor3);
    
    Serial.printf("Motor RPM: M1=%.2f, M2=%.2f, M3=%.2f\n", 
                  rpm1, rpm2, rpm3);
    
    // Đọc hướng encoder (-1=backward, 0=stop, 1=forward)
    int dir1 = getEncoderDirection(motor1);
    Serial.printf("Motor 1 direction: %d\n", dir1);
}

// ============================================
// 4. RESET ENCODER COUNT
// ============================================
void example_reset_encoder() {
    // Reset encoder count về 0 (khi cần đo khoảng cách mới)
    resetEncoderCount(motor1);
    resetEncoderCount(motor2);
    resetEncoderCount(motor3);
    
    Serial.println("All encoder counts reset to 0");
}

// ============================================
// 5. ĐIỀU KHIỂN 3 MOTOR ĐỒNG THỜI
// ============================================
void example_control_all_motors() {
    // Cả 3 motor tiến với tốc độ khác nhau
    setMotorSpeed(motor1, 150, MOTOR_FORWARD);
    setMotorSpeed(motor2, 180, MOTOR_FORWARD);
    setMotorSpeed(motor3, 200, MOTOR_FORWARD);
    
    delay(3000);
    
    // Dừng tất cả
    stopMotor(motor1);
    stopMotor(motor2);
    stopMotor(motor3);
}

// ============================================
// 6. DI CHUYỂN THEO KHOẢNG CÁCH (dùng encoder)
// ============================================
void example_move_distance() {
    // Ví dụ: Di chuyển motor 1 đến khi encoder đếm được 1000 xung
    long target_count = 1000;
    
    resetEncoderCount(motor1);  // Reset về 0
    setMotorSpeed(motor1, 150, MOTOR_FORWARD);  // Bắt đầu chạy
    
    // Chờ đến khi đạt target
    while (abs(getEncoderCount(motor1)) < target_count) {
        delay(10);
        
        // In progress
        if (millis() % 200 == 0) {
            Serial.printf("Progress: %ld / %ld\n", 
                          getEncoderCount(motor1), target_count);
        }
    }
    
    stopMotor(motor1);  // Dừng khi đạt target
    Serial.println("Target distance reached!");
}

// ============================================
// 7. ĐIỀU KHIỂN TỐC ĐỘ PID (nâng cao)
// ============================================
void example_pid_control() {
    // Set PID gains (Kp, Ki, Kd)
    setMotorPID(motor1, 1.0, 0.1, 0.05);
    
    // Set target RPM (ví dụ: 100 RPM)
    float target_rpm = 100.0;
    
    // Trong loop, gọi updateMotorPID định kỳ
    while (true) {
        updateMotorPID(motor1, target_rpm);
        
        float current_rpm = getMotorRPM(motor1);
        Serial.printf("Target: %.2f RPM, Current: %.2f RPM\n", 
                      target_rpm, current_rpm);
        
        delay(50);  // Update rate 20Hz
    }
}

// ============================================
// 8. PRINT STATUS CỦA TẤT CẢ MOTORS
// ============================================
void example_print_status() {
    // In trạng thái chi tiết của từng motor
    printMotorStatus(motor1);
    printMotorStatus(motor2);
    printMotorStatus(motor3);
    
    // Hoặc in tất cả một lần
    printAllMotorStatus();
    
    /* Output mẫu:
     * ========== MOTOR STATUS ==========
     * [MOTOR 1] Speed: 150, Dir: 1, Encoder: 2345, RPM: 98.50
     * [MOTOR 2] Speed: 0, Dir: 0, Encoder: 120, RPM: 0.00
     * [MOTOR 3] Speed: 200, Dir: 2, Encoder: -456, RPM: -75.30
     * ==================================
     */
}

// ============================================
// 9. TRUY CẬP TRỰC TIẾP STRUCT MOTOR
// ============================================
void example_direct_access() {
    // Có thể truy cập trực tiếp các thuộc tính của motor
    Serial.printf("Motor 1 ID: %d\n", motor1.id);
    Serial.printf("Motor 1 Current Speed: %d\n", motor1.current_speed);
    Serial.printf("Motor 1 Direction: %d\n", motor1.direction);
    Serial.printf("Motor 1 Encoder Count: %ld\n", motor1.encoder_count);
    
    // Set target speed để PID sử dụng
    motor1.target_speed = 150;
}

// ============================================
// 10. EXAMPLE COMPLETE - ROBOT 3 BÁNH OMNI
// ============================================
void example_omni_robot() {
    // Omni wheel robot - di chuyển theo hướng
    
    // Tiến thẳng
    setMotorSpeed(motor1, 150, MOTOR_FORWARD);
    setMotorSpeed(motor2, 150, MOTOR_FORWARD);
    setMotorSpeed(motor3, 150, MOTOR_FORWARD);
    delay(2000);
    
    // Lùi
    setMotorSpeed(motor1, 150, MOTOR_BACKWARD);
    setMotorSpeed(motor2, 150, MOTOR_BACKWARD);
    setMotorSpeed(motor3, 150, MOTOR_BACKWARD);
    delay(2000);
    
    // Xoay phải (motor 1 tiến, motor 2,3 lùi)
    setMotorSpeed(motor1, 150, MOTOR_FORWARD);
    setMotorSpeed(motor2, 150, MOTOR_BACKWARD);
    setMotorSpeed(motor3, 150, MOTOR_BACKWARD);
    delay(1000);
    
    // Xoay trái
    setMotorSpeed(motor1, 150, MOTOR_BACKWARD);
    setMotorSpeed(motor2, 150, MOTOR_FORWARD);
    setMotorSpeed(motor3, 150, MOTOR_FORWARD);
    delay(1000);
    
    // Dừng
    stopMotor(motor1);
    stopMotor(motor2);
    stopMotor(motor3);
}

// ============================================
// 11. ENCODER INTERRUPT (tự động - không cần code)
// ============================================
// Encoder được xử lý tự động bằng interrupt
// ISR: encoder1_ISR(), encoder2_ISR(), encoder3_ISR()
// - Đếm xung encoder trên rising edge của pin A
// - Xác định hướng bằng cách đọc pin B
// - Có chống nhiễu (debounce 2ms)
// - Chạy trong IRAM để tốc độ cao

// ============================================
// 12. CẤU HÌNH PWM (đã config sẵn)
// ============================================
// PWM_FREQ = 5000 Hz (5kHz)
// PWM_RESOLUTION = 8-bit (0-255)
// PWM Channels:
//   - Motor 1: Channel 0, 1
//   - Motor 2: Channel 2, 3
//   - Motor 3: Channel 4, 5

// ============================================
// 13. ENCODER CONFIGURATION
// ============================================
// ENCODER_PPR = 20 (pulses per revolution)
// Thay đổi giá trị này trong motor.h nếu encoder của bạn khác
// Ví dụ:
// - Encoder 20 PPR: #define ENCODER_PPR 20
// - Encoder 360 PPR: #define ENCODER_PPR 360
// - Encoder 600 PPR: #define ENCODER_PPR 600

// ============================================
// 14. NOTES & TIPS
// ============================================
// - GPIO 36, 39 (Motor 3 encoder) chỉ INPUT-only
// - GPIO 32, 33 có ADC - có thể đọc current sensing
// - PWM frequency 5kHz phù hợp với hầu hết L298N
// - Encoder debounce 2ms để tránh nhiễu
// - RPM tính sau mỗi 100ms để ổn định
// - PID update rate khuyến nghị: 20-50Hz (20-50ms)
// - Tốc độ motor: 0-255, khuyến nghị min speed > 50 để động cơ quay được
