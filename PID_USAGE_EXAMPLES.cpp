/*
 * ============================================
 * PID MOTOR CONTROL - USAGE EXAMPLES
 * ============================================
 * 
 * Hệ thống PID tự động duy trì tốc độ động cơ ổn định
 * bằng cách đọc encoder và điều chỉnh PWM liên tục
 */

#include <Arduino.h>
#include "motor.h"

// ============================================
// VÍ DỤ 1: SETUP CƠ BẢN VỚI PID
// ============================================
void example_basic_pid_setup() {
    // 1. Khởi tạo motor system
    setupMotors();
    
    // 2. Cấu hình PID parameters (có thể tuning)
    setMotorPID(motor1, 2.0, 0.5, 0.1);  // Kp, Ki, Kd
    setMotorPID(motor2, 2.0, 0.5, 0.1);
    setMotorPID(motor3, 2.0, 0.5, 0.1);
    
    // 3. Bật PID control
    enableMotorPID(motor1, true);
    enableMotorPID(motor2, true);
    enableMotorPID(motor3, true);
    
    // 4. Tạo task để cập nhật PID liên tục (50ms)
    xTaskCreate(
        motorControlTask,     // Task function
        "MotorPIDTask",       // Task name
        4096,                 // Stack size
        NULL,                 // Parameters
        1,                    // Priority
        NULL                  // Task handle
    );
}

// ============================================
// VÍ DỤ 2: ĐIỀU KHIỂN MOTOR VỚI PID
// ============================================
void example_motor_with_pid() {
    // Đặt tốc độ với PID - motor sẽ tự động duy trì tốc độ này
    setMotorSpeedWithPID(motor1, 150, MOTOR_FORWARD);  // PWM 150, tiến
    
    // PID task sẽ tự động:
    // - Đọc RPM hiện tại từ encoder
    // - So sánh với RPM mục tiêu
    // - Điều chỉnh PWM để đạt tốc độ chính xác
    
    delay(5000);  // Chạy 5 giây
    
    // Thay đổi tốc độ
    setMotorSpeedWithPID(motor1, 200, MOTOR_FORWARD);  // Tăng tốc
    
    delay(5000);
    
    // Dừng motor
    setMotorSpeedWithPID(motor1, 0, MOTOR_STOP);
}

// ============================================
// VÍ DỤ 3: ĐIỀU KHIỂN KHÔNG DÙNG PID
// ============================================
void example_motor_without_pid() {
    // Tắt PID - điều khiển trực tiếp PWM
    enableMotorPID(motor1, false);
    
    // Điều khiển thủ công
    setMotorSpeed(motor1, 150, MOTOR_FORWARD);
    
    delay(3000);
    
    stopMotor(motor1);
}

// ============================================
// VÍ DỤ 4: TUNING PID PARAMETERS
// ============================================
void example_pid_tuning() {
    /*
     * PID Tuning Guide:
     * 
     * Kp (Proportional): Phản ứng với error hiện tại
     *    - Tăng Kp: phản ứng nhanh hơn, nhưng có thể dao động
     *    - Giảm Kp: ổn định hơn, nhưng chậm
     *    - Khởi đầu: 1.0 - 3.0
     * 
     * Ki (Integral): Loại bỏ steady-state error
     *    - Tăng Ki: loại bỏ error lâu dài
     *    - Quá cao: overshoot và dao động
     *    - Khởi đầu: 0.1 - 1.0
     * 
     * Kd (Derivative): Giảm overshoot
     *    - Tăng Kd: giảm dao động
     *    - Quá cao: nhạy cảm với nhiễu
     *    - Khởi đầu: 0.0 - 0.5
     */
    
    // Test PID khác nhau
    Serial.println("[TEST] Testing aggressive PID...");
    setMotorPID(motor1, 3.0, 1.0, 0.2);  // Aggressive
    setMotorSpeedWithPID(motor1, 150, MOTOR_FORWARD);
    delay(5000);
    
    Serial.println("[TEST] Testing conservative PID...");
    setMotorPID(motor1, 1.5, 0.3, 0.05); // Conservative
    setMotorSpeedWithPID(motor1, 150, MOTOR_FORWARD);
    delay(5000);
    
    stopMotor(motor1);
}

// ============================================
// VÍ DỤ 5: 3 MOTOR CÙNG LÚC VỚI PID
// ============================================
void example_three_motors_pid() {
    // Bật PID cho cả 3 motor
    enableMotorPID(motor1, true);
    enableMotorPID(motor2, true);
    enableMotorPID(motor3, true);
    
    // Đặt tốc độ khác nhau
    setMotorSpeedWithPID(motor1, 150, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor2, 180, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor3, 120, MOTOR_BACKWARD);
    
    // Tất cả sẽ duy trì tốc độ của mình
    delay(10000);
    
    // Dừng tất cả
    setMotorSpeedWithPID(motor1, 0, MOTOR_STOP);
    setMotorSpeedWithPID(motor2, 0, MOTOR_STOP);
    setMotorSpeedWithPID(motor3, 0, MOTOR_STOP);
}

// ============================================
// VÍ DỤ 6: KIỂM TRA HIỆU SUẤT PID
// ============================================
void example_pid_performance_test() {
    Serial.println("[TEST] PID Performance Test");
    
    enableMotorPID(motor1, true);
    
    // Test response time
    Serial.println("[TEST] Testing response time...");
    unsigned long start = millis();
    setMotorSpeedWithPID(motor1, 200, MOTOR_FORWARD);
    
    // Đợi đạt 95% tốc độ mục tiêu
    float target_rpm = (200.0 / 255.0) * 100.0;  // 100 RPM max
    while (abs(motor1.current_rpm - target_rpm) > target_rpm * 0.05) {
        delay(50);
        if (millis() - start > 10000) break;  // Timeout 10s
    }
    
    unsigned long response_time = millis() - start;
    Serial.printf("[TEST] Response time: %lu ms\n", response_time);
    Serial.printf("[TEST] Target RPM: %.1f, Actual RPM: %.1f\n", 
                  target_rpm, motor1.current_rpm);
    
    delay(5000);
    
    // Test overshoot
    Serial.println("[TEST] Testing overshoot...");
    float max_rpm = 0;
    start = millis();
    while (millis() - start < 5000) {
        if (motor1.current_rpm > max_rpm) {
            max_rpm = motor1.current_rpm;
        }
        delay(50);
    }
    
    float overshoot = ((max_rpm - target_rpm) / target_rpm) * 100.0;
    Serial.printf("[TEST] Overshoot: %.1f%%\n", overshoot);
    
    stopMotor(motor1);
}

// ============================================
// VÍ DỤ 7: THAY ĐỔI TỐC ĐỘ ĐỘNG (RAMP)
// ============================================
void example_speed_ramp_with_pid() {
    enableMotorPID(motor1, true);
    
    Serial.println("[RAMP] Ramping up speed...");
    
    // Tăng tốc từ 0 -> 200 từng bước
    for (int speed = 0; speed <= 200; speed += 20) {
        setMotorSpeedWithPID(motor1, speed, MOTOR_FORWARD);
        Serial.printf("[RAMP] Speed set to: %d\n", speed);
        delay(2000);  // Đợi ổn định mỗi bước
    }
    
    Serial.println("[RAMP] Ramping down speed...");
    
    // Giảm tốc từ 200 -> 0
    for (int speed = 200; speed >= 0; speed -= 20) {
        setMotorSpeedWithPID(motor1, speed, MOTOR_FORWARD);
        Serial.printf("[RAMP] Speed set to: %d\n", speed);
        delay(2000);
    }
    
    stopMotor(motor1);
}

// ============================================
// VÍ DỤ 8: ỨNG DỤNG THỰC TẾ - DI CHUYỂN ROBOT
// ============================================
void example_robot_movement_with_pid() {
    // Bật PID cho cả 3 bánh omni
    enableMotorPID(motor1, true);
    enableMotorPID(motor2, true);
    enableMotorPID(motor3, true);
    
    Serial.println("[ROBOT] Moving forward...");
    // Tiến thẳng - cả 3 bánh cùng tốc độ
    setMotorSpeedWithPID(motor1, 150, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor2, 150, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor3, 150, MOTOR_FORWARD);
    delay(3000);
    
    Serial.println("[ROBOT] Rotating clockwise...");
    // Xoay phải
    setMotorSpeedWithPID(motor1, 120, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor2, 120, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor3, 120, MOTOR_FORWARD);
    delay(2000);
    
    Serial.println("[ROBOT] Strafing left...");
    // Di chuyển sang trái
    setMotorSpeedWithPID(motor1, 150, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor2, 0, MOTOR_STOP);
    setMotorSpeedWithPID(motor3, 150, MOTOR_BACKWARD);
    delay(3000);
    
    Serial.println("[ROBOT] Stopping...");
    setMotorSpeedWithPID(motor1, 0, MOTOR_STOP);
    setMotorSpeedWithPID(motor2, 0, MOTOR_STOP);
    setMotorSpeedWithPID(motor3, 0, MOTOR_STOP);
}

// ============================================
// MAIN EXAMPLE
// ============================================
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("========================================");
    Serial.println("PID MOTOR CONTROL - DEMO");
    Serial.println("========================================");
    
    // Setup cơ bản với PID
    example_basic_pid_setup();
    
    delay(2000);
    
    // Chọn ví dụ muốn chạy:
    // example_motor_with_pid();
    // example_motor_without_pid();
    // example_pid_tuning();
    // example_three_motors_pid();
    // example_pid_performance_test();
    // example_speed_ramp_with_pid();
    example_robot_movement_with_pid();
}

void loop() {
    // PID task chạy trong background
    // In status mỗi 2 giây
    printAllMotorStatus();
    delay(2000);
}

/*
 * ============================================
 * HƯỚNG DẪN SỬ DỤNG
 * ============================================
 * 
 * 1. SETUP BAN ĐẦU:
 *    - Gọi setupMotors()
 *    - Cấu hình PID với setMotorPID()
 *    - Bật PID với enableMotorPID()
 *    - Tạo motorControlTask()
 * 
 * 2. ĐIỀU KHIỂN:
 *    - Dùng setMotorSpeedWithPID() để đặt tốc độ
 *    - PID tự động duy trì tốc độ
 *    - Không cần gọi updateMotorPID() (task tự động)
 * 
 * 3. TUNING:
 *    - Bắt đầu với Kp=2.0, Ki=0.5, Kd=0.1
 *    - Tăng Kp nếu phản ứng chậm
 *    - Tăng Ki nếu có steady-state error
 *    - Tăng Kd nếu có overshoot
 * 
 * 4. MONITOR:
 *    - Dùng printMotorStatus() để xem thông số
 *    - Xem Serial Monitor để debug PID
 * 
 * ============================================
 */
