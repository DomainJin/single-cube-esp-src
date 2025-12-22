# HỆ THỐNG PID DUY TRÌ TỐC ĐỘ ĐỘNG CƠ

## 📋 TỔNG QUAN

Hệ thống PID (Proportional-Integral-Derivative) tự động duy trì tốc độ động cơ ổn định bằng cách:
- Đọc tốc độ thực tế từ encoder
- So sánh với tốc độ mục tiêu
- Điều chỉnh PWM liên tục để đạt tốc độ chính xác

## 🎯 ƯU ĐIỂM CỦA PID CONTROL

### ✅ Không dùng PID:
```cpp
setMotorSpeed(motor1, 150, MOTOR_FORWARD);
// Tốc độ sẽ thay đổi khi:
// - Tải thay đổi (robot đi lên dốc)
// - Pin yếu
// - Ma sát khác nhau giữa các bánh
```

### ✅ Có dùng PID:
```cpp
enableMotorPID(motor1, true);
setMotorSpeedWithPID(motor1, 150, MOTOR_FORWARD);
// PID tự động điều chỉnh PWM để:
// - Duy trì tốc độ ổn định dù có tải
// - Bù trừ pin yếu
// - Đảm bảo các bánh chạy đồng đều
```

## 🔧 CẤU TRÚC CODE

### Motor Structure (motor.h)
```cpp
struct Motor {
    // ... existing fields ...
    
    // PID Control
    bool pid_enabled;              // Bật/tắt PID
    float kp, ki, kd;              // PID parameters
    float error_sum;               // Tích phân error (I)
    float last_error;              // Error trước (D)
    float target_rpm;              // Tốc độ mục tiêu (RPM)
    float current_rpm;             // Tốc độ đo được (RPM)
    unsigned long last_pid_update; // Timestamp
};
```

### Các Hàm PID (motor.h)
```cpp
// Cấu hình PID parameters
void setMotorPID(Motor& motor, float kp, float ki, float kd);

// Bật/tắt PID control
void enableMotorPID(Motor& motor, bool enable);

// Đặt tốc độ với PID (thay vì setMotorSpeed)
void setMotorSpeedWithPID(Motor& motor, int speed, int direction);

// Cập nhật PID (được gọi tự động bởi task)
void updateMotorPID(Motor& motor);

// Reset PID state
void resetMotorPID(Motor& motor);
```

## 🚀 CÁCH SỬ DỤNG

### 1. Setup Ban Đầu
```cpp
void setup() {
    Serial.begin(115200);
    
    // 1. Khởi tạo motor system
    setupMotors();
    
    // 2. Cấu hình PID parameters
    setMotorPID(motor1, 2.0, 0.5, 0.1);  // Kp, Ki, Kd
    setMotorPID(motor2, 2.0, 0.5, 0.1);
    setMotorPID(motor3, 2.0, 0.5, 0.1);
    
    // 3. Bật PID control
    enableMotorPID(motor1, true);
    enableMotorPID(motor2, true);
    enableMotorPID(motor3, true);
    
    // 4. Tạo task để cập nhật PID (50ms interval)
    xTaskCreate(
        motorControlTask,   // Task function
        "MotorPID",        // Task name
        4096,              // Stack size
        NULL,              // Parameters
        1,                 // Priority
        NULL               // Task handle
    );
}
```

### 2. Điều Khiển Motor
```cpp
void loop() {
    // Đặt tốc độ - PID tự động duy trì
    setMotorSpeedWithPID(motor1, 150, MOTOR_FORWARD);
    delay(5000);
    
    // Thay đổi tốc độ
    setMotorSpeedWithPID(motor1, 200, MOTOR_FORWARD);
    delay(5000);
    
    // Dừng
    setMotorSpeedWithPID(motor1, 0, MOTOR_STOP);
    delay(2000);
}
```

## ⚙️ TUNING PID PARAMETERS

### Giải Thích Các Tham Số

#### Kp (Proportional - Tỷ lệ)
- **Chức năng:** Phản ứng với error hiện tại
- **Tăng Kp:** Phản ứng nhanh hơn, nhưng dễ dao động
- **Giảm Kp:** Ổn định hơn, nhưng chậm
- **Khởi đầu:** 1.0 - 3.0

#### Ki (Integral - Tích phân)
- **Chức năng:** Loại bỏ steady-state error (sai số dài hạn)
- **Tăng Ki:** Loại bỏ error lâu dài tốt hơn
- **Quá cao:** Gây overshoot và dao động
- **Khởi đầu:** 0.1 - 1.0

#### Kd (Derivative - Vi phân)
- **Chức năng:** Giảm overshoot (vượt mục tiêu)
- **Tăng Kd:** Giảm dao động
- **Quá cao:** Nhạy cảm với nhiễu
- **Khởi đầu:** 0.0 - 0.5

### Quy Trình Tuning

```cpp
// Bước 1: Bắt đầu với Kd = 0, Ki = 0
setMotorPID(motor1, 1.0, 0.0, 0.0);

// Bước 2: Tăng Kp cho đến khi dao động nhẹ
setMotorPID(motor1, 2.0, 0.0, 0.0);  // Test
setMotorPID(motor1, 3.0, 0.0, 0.0);  // Test
// ... tìm Kp tối ưu

// Bước 3: Thêm Ki để loại bỏ steady-state error
setMotorPID(motor1, 2.5, 0.3, 0.0);  // Test
setMotorPID(motor1, 2.5, 0.5, 0.0);  // Test

// Bước 4: Thêm Kd để giảm overshoot
setMotorPID(motor1, 2.5, 0.5, 0.1);  // Test
```

### Ví Dụ Cấu Hình

```cpp
// Aggressive (phản ứng nhanh, có thể dao động)
setMotorPID(motor1, 3.0, 1.0, 0.2);

// Balanced (cân bằng, khuyên dùng)
setMotorPID(motor1, 2.0, 0.5, 0.1);

// Conservative (ổn định, chậm)
setMotorPID(motor1, 1.5, 0.3, 0.05);
```

## 📊 MONITORING VÀ DEBUG

### Serial Monitor Output
```
[MOTOR_1] PID tuned: Kp=2.00, Ki=0.50, Kd=0.10
[MOTOR_1] PID control ENABLED
[MOTOR_1] Target set: PWM=150, RPM=58.8, Dir=1, PID=ON
[PID_1] Target:58.8 Current:52.3 Error:6.5 PWM:163
[PID_1] Target:58.8 Current:57.1 Error:1.7 PWM:156
[PID_1] Target:58.8 Current:58.5 Error:0.3 PWM:151
```

### Kiểm Tra Hiệu Suất
```cpp
void checkPIDPerformance() {
    enableMotorPID(motor1, true);
    setMotorSpeedWithPID(motor1, 200, MOTOR_FORWARD);
    
    delay(5000);  // Đợi ổn định
    
    // Kiểm tra độ chính xác
    float target_rpm = (200.0 / 255.0) * 100.0;
    float error_percent = abs(motor1.current_rpm - target_rpm) / target_rpm * 100.0;
    
    Serial.printf("Target: %.1f RPM\n", target_rpm);
    Serial.printf("Actual: %.1f RPM\n", motor1.current_rpm);
    Serial.printf("Error: %.1f%%\n", error_percent);
}
```

## 🎮 ỨNG DỤNG THỰC TẾ

### Di Chuyển Robot Omni-Directional
```cpp
void moveRobotForward(int speed) {
    enableMotorPID(motor1, true);
    enableMotorPID(motor2, true);
    enableMotorPID(motor3, true);
    
    // Cả 3 bánh chạy cùng tốc độ - PID đảm bảo đồng đều
    setMotorSpeedWithPID(motor1, speed, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor2, speed, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor3, speed, MOTOR_FORWARD);
}

void rotateRobot(int speed) {
    // Xoay - PID đảm bảo cả 3 bánh đồng bộ
    setMotorSpeedWithPID(motor1, speed, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor2, speed, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor3, speed, MOTOR_FORWARD);
}

void strafeLeft(int speed) {
    // Di chuyển ngang - cần tốc độ chính xác
    setMotorSpeedWithPID(motor1, speed, MOTOR_FORWARD);
    setMotorSpeedWithPID(motor2, 0, MOTOR_STOP);
    setMotorSpeedWithPID(motor3, speed, MOTOR_BACKWARD);
}
```

## 🔍 TROUBLESHOOTING

### Vấn Đề: Motor Dao Động
**Nguyên nhân:** Kp hoặc Ki quá cao
**Giải pháp:**
```cpp
// Giảm Kp và Ki
setMotorPID(motor1, 1.5, 0.3, 0.1);
```

### Vấn Đề: Motor Chậm Đạt Tốc Độ
**Nguyên nhân:** Kp quá thấp
**Giải pháp:**
```cpp
// Tăng Kp
setMotorPID(motor1, 3.0, 0.5, 0.1);
```

### Vấn Đề: Có Steady-State Error
**Nguyên nhân:** Ki quá thấp hoặc = 0
**Giải pháp:**
```cpp
// Tăng Ki
setMotorPID(motor1, 2.0, 0.8, 0.1);
```

### Vấn Đề: Overshoot Lớn
**Nguyên nhân:** Kd quá thấp
**Giải pháp:**
```cpp
// Tăng Kd
setMotorPID(motor1, 2.0, 0.5, 0.3);
```

### Vấn Đề: PID Không Hoạt Động
**Kiểm tra:**
```cpp
// 1. PID đã được bật?
if (!motor1.pid_enabled) {
    enableMotorPID(motor1, true);
}

// 2. Task đã được tạo?
xTaskCreate(motorControlTask, "MotorPID", 4096, NULL, 1, NULL);

// 3. Encoder có hoạt động?
Serial.println(getEncoderCount(motor1));  // Phải thay đổi khi motor chạy
```

## 📈 THÔNG SỐ KỸ THUẬT

### Timing
- **PID Update Rate:** 50ms (20Hz)
- **Encoder Sample Rate:** Mỗi xung (interrupt-driven)
- **RPM Calculation:** Mỗi 50ms

### Constraints
- **PWM Range:** 0-255
- **RPM Range:** 0-100 RPM (ước tính, có thể điều chỉnh)
- **Integral Anti-windup:** ±100

### Performance
- **Response Time:** ~1-3 giây (tùy tuning)
- **Steady-State Error:** <5% (với Ki phù hợp)
- **Overshoot:** <10% (với Kd phù hợp)

## 💡 TIPS & TRICKS

### 1. Khởi Động Mềm
```cpp
// Tăng tốc dần thay vì đột ngột
for (int speed = 0; speed <= 200; speed += 20) {
    setMotorSpeedWithPID(motor1, speed, MOTOR_FORWARD);
    delay(500);
}
```

### 2. PID Khác Nhau Cho Mỗi Motor
```cpp
// Motor 1 cần phản ứng nhanh
setMotorPID(motor1, 3.0, 0.5, 0.1);

// Motor 2 cần ổn định
setMotorPID(motor2, 1.5, 0.3, 0.05);

// Motor 3 cân bằng
setMotorPID(motor3, 2.0, 0.5, 0.1);
```

### 3. Chuyển Đổi Động
```cpp
// Tạm tắt PID khi cần điều khiển thủ công
enableMotorPID(motor1, false);
setMotorSpeed(motor1, 100, MOTOR_FORWARD);  // Manual control

// Bật lại khi cần ổn định
enableMotorPID(motor1, true);
setMotorSpeedWithPID(motor1, 150, MOTOR_FORWARD);
```

## 📝 CHECKLIST SỬ DỤNG

- [ ] Gọi `setupMotors()` trong setup()
- [ ] Cấu hình PID với `setMotorPID()`
- [ ] Bật PID với `enableMotorPID(motor, true)`
- [ ] Tạo `motorControlTask()` trong setup()
- [ ] Dùng `setMotorSpeedWithPID()` thay vì `setMotorSpeed()`
- [ ] Kiểm tra encoder hoạt động (`getEncoderCount()`)
- [ ] Tuning PID parameters theo ứng dụng
- [ ] Monitor qua Serial để debug

## 🎯 KẾT LUẬN

Hệ thống PID giúp:
- ✅ Duy trì tốc độ ổn định
- ✅ Bù trừ tải thay đổi
- ✅ Đồng bộ nhiều motor
- ✅ Tăng độ chính xác di chuyển robot
- ✅ Cải thiện hiệu suất tổng thể

**Khuyến nghị:** Luôn dùng PID cho ứng dụng cần độ chính xác cao như robot autonomous, xe tự hành, cánh tay robot, v.v.
