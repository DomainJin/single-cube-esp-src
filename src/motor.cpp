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
    .kp = 1.0, .ki = 0.0, .kd = 0.0,
    .error_sum = 0.0, .last_error = 0.0
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
    .kp = 1.0, .ki = 0.0, .kd = 0.0,
    .error_sum = 0.0, .last_error = 0.0
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
    .kp = 1.0, .ki = 0.0, .kd = 0.0,
    .error_sum = 0.0, .last_error = 0.0
};

// ============================================
// ENCODER ISR HANDLERS
// ============================================
void IRAM_ATTR encoder1_ISR() {
    unsigned long current_time = millis();
    
    // Chống nhiễu
    if (current_time - motor1.last_pulse_time < ENCODER_DEBOUNCE_MS) {
        return;
    }
    
    // Đọc trạng thái encoder B để xác định hướng
    int b_state = digitalRead(MOTOR_1_ENCODER_B);
    
    if (b_state == HIGH) {
        motor1.encoder_count++;
        motor1.encoder_direction = 1;  // Forward
    } else {
        motor1.encoder_count--;
        motor1.encoder_direction = -1; // Backward
    }
    
    motor1.last_pulse_time = current_time;
}

void IRAM_ATTR encoder2_ISR() {
    unsigned long current_time = millis();
    
    if (current_time - motor2.last_pulse_time < ENCODER_DEBOUNCE_MS) {
        return;
    }
    
    int b_state = digitalRead(MOTOR_2_ENCODER_B);
    
    if (b_state == HIGH) {
        motor2.encoder_count++;
        motor2.encoder_direction = 1;
    } else {
        motor2.encoder_count--;
        motor2.encoder_direction = -1;
    }
    
    motor2.last_pulse_time = current_time;
}

void IRAM_ATTR encoder3_ISR() {
    unsigned long current_time = millis();
    
    if (current_time - motor3.last_pulse_time < ENCODER_DEBOUNCE_MS) {
        return;
    }
    
    int b_state = digitalRead(MOTOR_3_ENCODER_B);
    
    if (b_state == HIGH) {
        motor3.encoder_count++;
        motor3.encoder_direction = 1;
    } else {
        motor3.encoder_count--;
        motor3.encoder_direction = -1;
    }
    
    motor3.last_pulse_time = current_time;
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
    
    // Attach interrupts cho encoder A pins (đếm xung trên rising edge)
    attachInterrupt(digitalPinToInterrupt(MOTOR_1_ENCODER_A), encoder1_ISR, RISING);
    attachInterrupt(digitalPinToInterrupt(MOTOR_2_ENCODER_A), encoder2_ISR, RISING);
    attachInterrupt(digitalPinToInterrupt(MOTOR_3_ENCODER_A), encoder3_ISR, RISING);
    
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
    
    // Tính RPM dựa trên encoder count
    // RPM = (encoder_count / ENCODER_PPR) * (60 / time_in_seconds)
    
    static unsigned long last_time[3] = {0, 0, 0};
    static long last_count[3] = {0, 0, 0};
    
    int motor_id = motor.id - 1; // 0, 1, 2
    unsigned long current_time = millis();
    long current_count = motor.encoder_count;
    
    unsigned long time_diff = current_time - last_time[motor_id];
    
    if (time_diff < 50) {  // ✅ Tối thiểu 50ms để tính RPM (realtime hơn)
        return 0.0;
    }
    
    long count_diff = current_count - last_count[motor_id];
    float rpm = (count_diff * 60000.0) / (ENCODER_PPR * time_diff);
    
    last_time[motor_id] = current_time;
    last_count[motor_id] = current_count;
    
    return rpm;
}

// ============================================
// PID CONTROL
// ============================================
void setMotorPID(Motor& motor, float kp, float ki, float kd) {
    motor.kp = kp;
    motor.ki = ki;
    motor.kd = kd;
    motor.error_sum = 0.0;
    motor.last_error = 0.0;
}

void updateMotorPID(Motor& motor, float target_rpm) {
    float current_rpm = getMotorRPM(motor);
    float error = target_rpm - current_rpm;
    
    motor.error_sum += error;
    float error_diff = error - motor.last_error;
    
    float output = motor.kp * error + motor.ki * motor.error_sum + motor.kd * error_diff;
    
    // Giới hạn output
    int speed = constrain((int)output, MIN_SPEED, MAX_SPEED);
    
    int direction = (target_rpm >= 0) ? MOTOR_FORWARD : MOTOR_BACKWARD;
    setMotorSpeed(motor, abs(speed), direction);
    
    motor.last_error = error;
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
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // 50ms update rate
    
    while (true) {
        // Cập nhật PID cho tất cả motors (nếu cần)
        // updateMotorPID(motor1, motor1.target_speed);
        // updateMotorPID(motor2, motor2.target_speed);
        // updateMotorPID(motor3, motor3.target_speed);
        
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
