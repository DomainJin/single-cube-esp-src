#include "motor.h"

// ============================================
// GLOBAL MOTOR INSTANCES
// ============================================
Motor motor1 = {
    .id = 1,
    .pin_in1 = MOTOR_1_IN1,
    .pin_in2 = MOTOR_1_IN2,
    .pwm_channel_in1 = PWM_CHANNEL_M1_IN1,
    .pwm_channel_in2 = PWM_CHANNEL_M1_IN2,
    .pin_encoder_a = MOTOR_1_ENCODER_A,
    .pin_encoder_b = MOTOR_1_ENCODER_B,
    .encoder_count = 0,
    .encoder_direction = 0,
    .last_pulse_time = 0,
    .current_speed = 0,
    .target_speed = 0,
    .direction = MOTOR_STOP,
    .pid_enabled = false,
    .kp = 2.0, .ki = 0.1, .kd = 0.02,
    .error_sum = 0.0, .last_error = 0.0,
    .target_rpm = 0.0, .current_rpm = 0.0,
    .last_pid_update = 0
};

Motor motor2 = {
    .id = 2,
    .pin_in1 = MOTOR_2_IN1,
    .pin_in2 = MOTOR_2_IN2,
    .pwm_channel_in1 = PWM_CHANNEL_M2_IN1,
    .pwm_channel_in2 = PWM_CHANNEL_M2_IN2,
    .pin_encoder_a = MOTOR_2_ENCODER_A,
    .pin_encoder_b = MOTOR_2_ENCODER_B,
    .encoder_count = 0,
    .encoder_direction = 0,
    .last_pulse_time = 0,
    .current_speed = 0,
    .target_speed = 0,
    .direction = MOTOR_STOP,
    .pid_enabled = false,
    .kp = 2.0, .ki = 0.1, .kd = 0.02,
    .error_sum = 0.0, .last_error = 0.0,
    .target_rpm = 0.0, .current_rpm = 0.0,
    .last_pid_update = 0
};

Motor motor3 = {
    .id = 3,
    .pin_in1 = MOTOR_3_IN1,
    .pin_in2 = MOTOR_3_IN2,
    .pwm_channel_in1 = PWM_CHANNEL_M3_IN1,
    .pwm_channel_in2 = PWM_CHANNEL_M3_IN2,
    .pin_encoder_a = MOTOR_3_ENCODER_A,
    .pin_encoder_b = MOTOR_3_ENCODER_B,
    .encoder_count = 0,
    .encoder_direction = 0,
    .last_pulse_time = 0,
    .current_speed = 0,
    .target_speed = 0,
    .direction = MOTOR_STOP,
    .pid_enabled = false,
    .kp = 2.0, .ki = 0.1, .kd = 0.02,
    .error_sum = 0.0, .last_error = 0.0,
    .target_rpm = 0.0, .current_rpm = 0.0,
    .last_pid_update = 0
};

// ============================================
// ENCODER ISR HANDLERS - QUADRATURE 4X DECODING
// ============================================
// Quadrature decoding: đếm cả RISING và FALLING của A và B
// PPR = 3, với 4x decoding = 12 counts/vòng
// Sử dụng state table để xác định chiều chính xác

// Lưu trạng thái trước của encoder (2 bit: A_prev << 1 | B_prev)
volatile uint8_t encoder1_last_state = 0;
volatile uint8_t encoder2_last_state = 0;
volatile uint8_t encoder3_last_state = 0;

void IRAM_ATTR encoder1_ISR() {
    unsigned long current_time = millis();
    
    // Chống nhiễu
    if (current_time - motor1.last_pulse_time < ENCODER_DEBOUNCE_MS) {
        return;
    }
    
    // Đọc trạng thái hiện tại của A và B
    int a_state = digitalRead(MOTOR_1_ENCODER_A);
    int b_state = digitalRead(MOTOR_1_ENCODER_B);
    uint8_t current_state = (a_state << 1) | b_state;  // 2-bit state: AB
    
    // Quadrature state table lookup
    // Previous -> Current: 00->01, 01->11, 11->10, 10->00 = Forward (+1)
    // Previous -> Current: 00->10, 10->11, 11->01, 01->00 = Backward (-1)
    int8_t lookup_table[16] = {
         0, -1,  1,  0,   // from 00: ->00(no), ->01(bwd), ->10(fwd), ->11(no)
         1,  0,  0, -1,   // from 01: ->00(fwd), ->01(no), ->10(no), ->11(bwd)
        -1,  0,  0,  1,   // from 10: ->00(bwd), ->01(no), ->10(no), ->11(fwd)
         0,  1, -1,  0    // from 11: ->00(no), ->01(fwd), ->10(bwd), ->11(no)
    };
    
    int8_t direction = lookup_table[(encoder1_last_state << 2) | current_state];
    
    if (direction != 0) {
        motor1.encoder_count += direction;
        motor1.encoder_direction = direction;
        motor1.last_pulse_time = current_time;
    }
    
    encoder1_last_state = current_state;
}

void IRAM_ATTR encoder2_ISR() {
    unsigned long current_time = millis();
    
    if (current_time - motor2.last_pulse_time < ENCODER_DEBOUNCE_MS) {
        return;
    }
    
    int a_state = digitalRead(MOTOR_2_ENCODER_A);
    int b_state = digitalRead(MOTOR_2_ENCODER_B);
    uint8_t current_state = (a_state << 1) | b_state;
    
    // Quadrature state table lookup
    int8_t lookup_table[16] = {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };
    
    int8_t direction = lookup_table[(encoder2_last_state << 2) | current_state];
    
    // ✅ Motor 2 đảo chiều vật lý - đảo direction
    direction = -direction;
    
    if (direction != 0) {
        motor2.encoder_count += direction;
        motor2.encoder_direction = direction;
        motor2.last_pulse_time = current_time;
    }
    
    encoder2_last_state = current_state;
}

void IRAM_ATTR encoder3_ISR() {
    unsigned long current_time = millis();
    
    if (current_time - motor3.last_pulse_time < ENCODER_DEBOUNCE_MS) {
        return;
    }
    
    int a_state = digitalRead(MOTOR_3_ENCODER_A);
    int b_state = digitalRead(MOTOR_3_ENCODER_B);
    uint8_t current_state = (a_state << 1) | b_state;
    
    // Quadrature state table lookup
    int8_t lookup_table[16] = {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };
    
    int8_t direction = lookup_table[(encoder3_last_state << 2) | current_state];
    
    if (direction != 0) {
        motor3.encoder_count += direction;
        motor3.encoder_direction = direction;
        motor3.last_pulse_time = current_time;
    }
    
    encoder3_last_state = current_state;
}

// ============================================
// SETUP MOTORS
// ============================================
void setupMotors() {
    Serial.println("[MOTOR] Initializing motor control system...");
    
    // Setup PWM cho Motor 1
    ledcSetup(PWM_CHANNEL_M1_IN1, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_M1_IN2, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR_1_IN1, PWM_CHANNEL_M1_IN1);
    ledcAttachPin(MOTOR_1_IN2, PWM_CHANNEL_M1_IN2);
    
    // Setup PWM cho Motor 2
    ledcSetup(PWM_CHANNEL_M2_IN1, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_M2_IN2, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR_2_IN1, PWM_CHANNEL_M2_IN1);
    ledcAttachPin(MOTOR_2_IN2, PWM_CHANNEL_M2_IN2);
    
    // Setup PWM cho Motor 3
    ledcSetup(PWM_CHANNEL_M3_IN1, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PWM_CHANNEL_M3_IN2, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(MOTOR_3_IN1, PWM_CHANNEL_M3_IN1);
    ledcAttachPin(MOTOR_3_IN2, PWM_CHANNEL_M3_IN2);
    
    // Setup Encoder pins
    pinMode(MOTOR_1_ENCODER_A, INPUT_PULLUP);
    pinMode(MOTOR_1_ENCODER_B, INPUT_PULLUP);
    pinMode(MOTOR_2_ENCODER_A, INPUT_PULLUP);
    pinMode(MOTOR_2_ENCODER_B, INPUT_PULLUP);
    pinMode(MOTOR_3_ENCODER_A, INPUT_PULLUP);
    pinMode(MOTOR_3_ENCODER_B, INPUT_PULLUP);
    
    // Attach interrupts cho encoder A pins ONLY (state table tự đọc B)
    // State table decoder đã đọc cả A và B trong ISR nên không cần interrupt B
    attachInterrupt(digitalPinToInterrupt(MOTOR_1_ENCODER_A), encoder1_ISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(MOTOR_2_ENCODER_A), encoder2_ISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(MOTOR_3_ENCODER_A), encoder3_ISR, CHANGE);
    
    // Stop tất cả motors khi khởi động
    stopMotor(motor1);
    stopMotor(motor2);
    stopMotor(motor3);
    
    Serial.println("[MOTOR] Motor control system initialized!");
    Serial.printf("[MOTOR] PWM Frequency: %d Hz, Resolution: %d-bit\n", PWM_FREQ, PWM_RESOLUTION);
}

// ============================================
// MOTOR CONTROL FUNCTIONS
// ============================================
void setMotorSpeed(Motor& motor, int speed, int direction) {
    // Giới hạn tốc độ
    speed = constrain(speed, MIN_SPEED, MAX_SPEED);
    
    motor.current_speed = speed;
    motor.direction = direction;
    
    // ✅ Motor 2 đấu ngược chiều - cần đảo FORWARD/BACKWARD
    if (motor.id == 2) {
        if (direction == MOTOR_FORWARD) direction = MOTOR_BACKWARD;
        else if (direction == MOTOR_BACKWARD) direction = MOTOR_FORWARD;
    }
    
    switch (direction) {
        case MOTOR_FORWARD:
            // IN1 = PWM, IN2 = 0
            ledcWrite(motor.pwm_channel_in1, speed);
            ledcWrite(motor.pwm_channel_in2, 0);
            break;
            
        case MOTOR_BACKWARD:
            // IN1 = 0, IN2 = PWM
            ledcWrite(motor.pwm_channel_in1, 0);
            ledcWrite(motor.pwm_channel_in2, speed);
            break;
            
        case MOTOR_STOP:
            // Coast stop (IN1 = 0, IN2 = 0)
            ledcWrite(motor.pwm_channel_in1, 0);
            ledcWrite(motor.pwm_channel_in2, 0);
            break;
            
        case MOTOR_BRAKE:
            // Active brake (IN1 = HIGH, IN2 = HIGH)
            ledcWrite(motor.pwm_channel_in1, 255);
            ledcWrite(motor.pwm_channel_in2, 255);
            break;
            
        default:
            stopMotor(motor);
            break;
    }
}

void stopMotor(Motor& motor) {
    ledcWrite(motor.pwm_channel_in1, 0);
    ledcWrite(motor.pwm_channel_in2, 0);
    motor.current_speed = 0;
    motor.direction = MOTOR_STOP;
}

void brakeMotor(Motor& motor) {
    ledcWrite(motor.pwm_channel_in1, 255);
    ledcWrite(motor.pwm_channel_in2, 255);
    motor.current_speed = 0;
    motor.direction = MOTOR_BRAKE;
}

// ============================================
// ENCODER FUNCTIONS
// ============================================
long getEncoderCount(Motor& motor) {
    return motor.encoder_count;
}

void resetEncoderCount(Motor& motor) {
    noInterrupts();
    motor.encoder_count = 0;
    interrupts();
}

int getEncoderDirection(Motor& motor) {
    return motor.encoder_direction;
}

float getMotorRPM(Motor& motor) {
    // ✅ Nếu motor đang dừng, trả về 0 ngay
    if (motor.current_speed == 0 || motor.direction == MOTOR_STOP || motor.direction == MOTOR_BRAKE) {
        return 0.0;
    }
    
    // Lưu RPM cuối cùng để trả về khi chưa đủ thời gian tính lại
    static float last_rpm[3] = {0.0, 0.0, 0.0};
    static unsigned long last_time[3] = {0, 0, 0};
    static long last_count[3] = {0, 0, 0};
    
    int motor_id = motor.id - 1; // 0, 1, 2
    unsigned long current_time = millis();
    long current_count = motor.encoder_count;
    
    unsigned long time_diff = current_time - last_time[motor_id];
    
    // ✅ Nếu chưa đủ 100ms, trả về RPM cuối cùng thay vì 0
    if (time_diff < 100) {
        return last_rpm[motor_id];
    }
    
    long count_diff = current_count - last_count[motor_id];
    float rpm_encoder = (count_diff * 60000.0) / (ENCODER_COUNTS_PER_REV * time_diff);
    
    // ✅ Chia cho tỷ số truyền để ra RPM bánh xe
    float rpm_wheel = rpm_encoder / MOTOR_GEAR_RATIO;
    
    // ✅ DEBUG: In encoder info
    static unsigned long last_encoder_debug[3] = {0, 0, 0};
    if (millis() - last_encoder_debug[motor_id] > 1000) {
        last_encoder_debug[motor_id] = millis();
        Serial.printf("[ENCODER_%d] Count:%ld Diff:%ld Time:%lums PWM:%d RPM:%.1f\n", 
                      motor.id, current_count, count_diff, time_diff, motor.current_speed, rpm_wheel);
    }
    
    last_time[motor_id] = current_time;
    last_count[motor_id] = current_count;
    last_rpm[motor_id] = rpm_wheel;
    
    return rpm_wheel;  // ✅ Trả về RPM bánh xe
}

// ============================================
// PID CONTROL FOR SPEED MAINTENANCE
// ============================================
void setMotorPID(Motor& motor, float kp, float ki, float kd) {
    motor.kp = kp;
    motor.ki = ki;
    motor.kd = kd;
    Serial.printf("[MOTOR_%d] PID tuned: Kp=%.2f, Ki=%.2f, Kd=%.2f\n", 
                  motor.id, kp, ki, kd);
}

void enableMotorPID(Motor& motor, bool enable) {
    motor.pid_enabled = enable;
    if (enable) {
        resetMotorPID(motor);
        Serial.printf("[MOTOR_%d] PID control ENABLED\n", motor.id);
    } else {
        Serial.printf("[MOTOR_%d] PID control DISABLED\n", motor.id);
    }
}

void resetMotorPID(Motor& motor) {
    motor.error_sum = 0.0;
    motor.last_error = 0.0;
    motor.current_rpm = 0.0;
    motor.last_pid_update = millis();
}

void setMotorSpeedWithPID(Motor& motor, int speed, int direction) {
    // Đặt tốc độ PWM ban đầu
    motor.target_speed = constrain(speed, MIN_SPEED, MAX_SPEED);
    motor.direction = direction;
    
    // ✅ Apply PWM deadzone
    if (motor.target_speed > 0 && motor.target_speed < PWM_MIN_THRESHOLD) {
        motor.target_speed = PWM_MIN_THRESHOLD;
    }
    
    // Tính RPM mục tiêu dựa trên tốc độ PWM
    // Motor spec: 6000 RPM no-load, 320 RPM qua giảm tốc 19.2:1
    // Thực tế với tải: ~250 RPM max
    float max_rpm = 250.0;  // RPM tối đa thực tế khi PWM = 255
    motor.target_rpm = (motor.target_speed / 255.0) * max_rpm;
    
    // Nếu backward, RPM âm
    if (direction == MOTOR_BACKWARD) {
        motor.target_rpm = -motor.target_rpm;
    } else if (direction == MOTOR_STOP || direction == MOTOR_BRAKE) {
        motor.target_rpm = 0.0;
    }
    
    // Đặt tốc độ ban đầu với PWM cao hơn 20% để khởi động nhanh
    int initial_pwm = motor.target_speed;
    if (motor.pid_enabled && initial_pwm > 0) {
        initial_pwm = min(255, (int)(initial_pwm * 1.2));  // Boost 20% lúc start
    }
    setMotorSpeed(motor, initial_pwm, direction);
    
    // KHÔNG reset PID - để tích luỹ error cho bù tải
    // resetMotorPID(motor);
    
    // Serial.printf("[MOTOR_%d] Target: PWM=%d (initial=%d), RPM=%.1f, Dir=%d, PID=%s\n", 
    //               motor.id, motor.target_speed, initial_pwm, motor.target_rpm, 
    //               direction, motor.pid_enabled ? "ON" : "OFF");
}

void updateMotorPID(Motor& motor) {
    // Chỉ cập nhật PID nếu được bật
    if (!motor.pid_enabled) {
        return;
    }
    
    // Kiểm tra thời gian cập nhật (mỗi 200ms - rất chậm để tránh dao động)
    unsigned long current_time = millis();
    if (current_time - motor.last_pid_update < 200) {
        return;
    }
    
    float dt = (current_time - motor.last_pid_update) / 1000.0;  // chuyển sang giây
    motor.last_pid_update = current_time;
    
    // Đọc RPM hiện tại
    motor.current_rpm = getMotorRPM(motor);
    
    // Nếu target = 0, dừng motor
    if (abs(motor.target_rpm) < 0.1) {
        stopMotor(motor);
        resetMotorPID(motor);
        return;
    }
    
    // ⚠️ Kiểm tra encoder có hoạt động không
    static long last_encoder_check[3] = {0, 0, 0};
    static unsigned long encoder_stuck_time[3] = {0, 0, 0};
    int motor_idx = motor.id - 1;
    
    if (motor.current_speed > PWM_MIN_THRESHOLD) {
        if (motor.encoder_count == last_encoder_check[motor_idx]) {
            if (encoder_stuck_time[motor_idx] == 0) {
                encoder_stuck_time[motor_idx] = current_time;
            } else if (current_time - encoder_stuck_time[motor_idx] > 2000) {
                Serial.printf("[PID_%d] ⚠️ Encoder stuck! Enc=%ld PWM=%d\n", 
                              motor.id, motor.encoder_count, motor.current_speed);
                encoder_stuck_time[motor_idx] = current_time;
            }
        } else {
            encoder_stuck_time[motor_idx] = 0;
        }
        last_encoder_check[motor_idx] = motor.encoder_count;
    }
    
    // Tính error
    float error = motor.target_rpm - motor.current_rpm;
    
    // PID calculation
    motor.error_sum += error * dt;
    
    // Anti-windup - giảm mạnh để tránh overshoot
    float max_integral = 30.0;
    motor.error_sum = constrain(motor.error_sum, -max_integral, max_integral);
    
    float error_derivative = (error - motor.last_error) / dt;
    
    // Low-pass filter cho derivative để giảm nhiễu
    static float filtered_derivative[3] = {0, 0, 0};
    filtered_derivative[motor_idx] = 0.7 * filtered_derivative[motor_idx] + 0.3 * error_derivative;
    
    // PID output với derivative đã filter
    float pid_output = motor.kp * error + 
                      motor.ki * motor.error_sum + 
                      motor.kd * filtered_derivative[motor_idx];
    
    // ✅ Tính PWM mới: BẮT ĐẦU từ target_speed (không phải current_speed)
    // Điều này tránh drift khi omni set target mới
    int new_speed = motor.target_speed + (int)pid_output;
    new_speed = constrain(new_speed, MIN_SPEED, MAX_SPEED);
    
    // PWM Deadzone
    if (new_speed > 0 && new_speed < PWM_MIN_THRESHOLD) {
        new_speed = PWM_MIN_THRESHOLD;
    }
    
    // Xác định hướng
    int direction = (motor.target_rpm >= 0) ? MOTOR_FORWARD : MOTOR_BACKWARD;
    
    // ✅ CHỈ update PWM nếu thay đổi đáng kể (>5) để PID phản ứng nhanh với tải
    if (abs(new_speed - motor.current_speed) > 5) {
        setMotorSpeed(motor, new_speed, direction);
    }
    
    motor.last_error = error;
    
    // Debug output mỗi 500ms
    static unsigned long last_debug[3] = {0, 0, 0};
    if (current_time - last_debug[motor_idx] > 500) {
        Serial.printf("[PID_%d] T:%.1f C:%.1f Err:%.1f I:%.2f D:%.2f PWM:%d->%d Enc:%ld\n", 
                      motor.id, motor.target_rpm, motor.current_rpm, 
                      error, motor.error_sum, filtered_derivative[motor_idx], 
                      motor.current_speed, new_speed, motor.encoder_count);
        last_debug[motor_idx] = current_time;
    }
}

// ============================================
// DEBUG FUNCTIONS
// ============================================
void printMotorStatus(Motor& motor) {
    Serial.printf("[MOTOR %d] Speed: %d, Dir: %d, Encoder: %ld, RPM: %.2f\n",
                  motor.id,
                  motor.current_speed,
                  motor.direction,
                  motor.encoder_count,
                  getMotorRPM(motor));
}

void printAllMotorStatus() {
    Serial.println("========== MOTOR STATUS ==========");
    printMotorStatus(motor1);
    printMotorStatus(motor2);
    printMotorStatus(motor3);
    Serial.println("==================================");
}

// ============================================
// FREERTOS TASK (Optional)
// ============================================
void motorControlTask(void* parameter) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(200); // 200ms - rất chậm để tránh dao động
    
    Serial.println("[MOTOR TASK] PID control task started!");
    
    while (true) {
        // Cập nhật PID cho tất cả motors
        updateMotorPID(motor1);
        updateMotorPID(motor2);
        updateMotorPID(motor3);
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
