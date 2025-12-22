#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

// ============================================
// MOTOR PIN DEFINITIONS (L298N Driver)
// ============================================

// ĐỘNG CƠ 1 - Cụm chân bên TRÁI board
#define MOTOR_1_IN1       4   // GPIO 4  - OUTPUT PWM
#define MOTOR_1_IN2       13  // GPIO 13 - OUTPUT PWM
#define MOTOR_1_ENCODER_A 14  // GPIO 14 - INPUT
#define MOTOR_1_ENCODER_B 27  // GPIO 27 - INPUT (✅ Đổi từ GPIO 32 để tránh xung đột ADC1 với IR)

// ĐỘNG CƠ 2 - Cụm chân GIỮA board
#define MOTOR_2_IN1       18  // GPIO 18 - OUTPUT PWM
#define MOTOR_2_IN2       19  // GPIO 19 - OUTPUT PWM
#define MOTOR_2_ENCODER_A 21  // GPIO 21 - INPUT
#define MOTOR_2_ENCODER_B 22  // GPIO 22 - INPUT

// ĐỘNG CƠ 3 - Cụm chân bên PHẢI board
#define MOTOR_3_IN1       23  // GPIO 23 - OUTPUT PWM
#define MOTOR_3_IN2       33  // GPIO 33 - OUTPUT PWM (có ADC)
#define MOTOR_3_ENCODER_A 36  // GPIO 36 - INPUT-only (có ADC)
#define MOTOR_3_ENCODER_B 39  // GPIO 39 - INPUT-only (có ADC)

// ============================================
// PWM CONFIGURATION
// ============================================
#define PWM_FREQ        25000  // 25kHz PWM frequency (giảm tiếng kêu)
#define PWM_RESOLUTION  8     // 8-bit resolution (0-255)
#define PWM_MIN_THRESHOLD 30   // PWM tối thiểu - 30 để chạy được với tốc độ thấp (bù L298N voltage drop ~2V)

// PWM Channels (ESP32 có 16 kênh PWM)
#define PWM_CHANNEL_M1_IN1  0
#define PWM_CHANNEL_M1_IN2  1
#define PWM_CHANNEL_M2_IN1  2
#define PWM_CHANNEL_M2_IN2  3
#define PWM_CHANNEL_M3_IN1  4
#define PWM_CHANNEL_M3_IN2  5

// ============================================
// MOTOR CONTROL CONSTANTS
// ============================================
// Motor specs: 36FR249000-19.2K-12ppr
// - Rated voltage: 24V DC
// - No load: 469 RPM @ 200mA
// - Load: 413 RPM @ 3900mA, 96 gf·cm (30W)
// - Stall torque: 3700 gf·cm @ 8000mA
// - Gear ratio: 19.2:1
// - Encoder: 12ppr (datasheet) → thực tế 120 PPR (480 counts/rev với 4x)

#define MOTOR_STOP       0
#define MOTOR_FORWARD    1
#define MOTOR_BACKWARD   2
#define MOTOR_BRAKE      3

#define MAX_SPEED       255
#define MIN_SPEED       0
#define MOTOR_RATED_VOLTAGE 24  // 24V DC
#define MOTOR_NO_LOAD_RPM   469 // RPM at no load
#define MOTOR_LOAD_RPM      413 // RPM at rated load

// ============================================
// ENCODER CONFIGURATION
// ============================================
// Motor: 36FR249000-19.2K-12ppr
// Datasheet ghi "12ppr" nhưng thực tế đo được ~480 counts/vòng (4x quadrature)
// → Encoder thực tế là 120 PPR (có thể là 12 pole-pairs magnetic encoder)
#define ENCODER_PPR     120   // Pulses Per Revolution - đo thực tế 480 counts ÷ 4 = 120 PPR
#define ENCODER_COUNTS_PER_REV  480  // 120 PPR × 4 (quadrature 4x) = 480 counts/vòng
#define ENCODER_DEBOUNCE_MS  2   // Thời gian chống nhiễu (ms)
#define MOTOR_GEAR_RATIO 19.2f   // Tỷ số truyền 1:19.2 (theo datasheet)

// ============================================
// MOTOR STRUCTURE
// ============================================
struct Motor {
    uint8_t id;              // Motor ID (1, 2, 3)
    uint8_t pin_in1;         // IN1 pin
    uint8_t pin_in2;         // IN2 pin
    uint8_t pwm_channel_in1; // PWM channel cho IN1
    uint8_t pwm_channel_in2; // PWM channel cho IN2
    uint8_t pin_encoder_a;   // Encoder A pin
    uint8_t pin_encoder_b;   // Encoder B pin
    
    // Encoder state
    volatile long encoder_count;     // Số xung đếm được
    volatile int encoder_direction;  // Hướng: 1 = forward, -1 = backward
    volatile unsigned long last_pulse_time;  // Thời gian xung cuối (ms)
    
    // Motor state
    int current_speed;       // Tốc độ hiện tại (0-255)
    int target_speed;        // Tốc độ mục tiêu (0-255)
    int direction;           // Hướng: MOTOR_FORWARD, MOTOR_BACKWARD, MOTOR_STOP
    
    // PID control for speed maintenance
    bool pid_enabled;        // Bật/tắt PID control
    float kp, ki, kd;        // PID gains
    float error_sum;         // Tích phân error
    float last_error;        // Error trước đó
    float target_rpm;        // Tốc độ mục tiêu (RPM)
    float current_rpm;       // Tốc độ đo được (RPM)
    unsigned long last_pid_update;  // Thời điểm cập nhật PID cuối
};

// ============================================
// GLOBAL MOTOR INSTANCES
// ============================================
extern Motor motor1, motor2, motor3;

// ============================================
// FUNCTION DECLARATIONS
// ============================================

// Khởi tạo hệ thống motor
void setupMotors();

// Điều khiển motor cơ bản
void setMotorSpeed(Motor& motor, int speed, int direction);
void stopMotor(Motor& motor);
void brakeMotor(Motor& motor);

// Đọc encoder
long getEncoderCount(Motor& motor);
void resetEncoderCount(Motor& motor);
float getMotorRPM(Motor& motor);
int getEncoderDirection(Motor& motor);

// PID control for speed maintenance
void setMotorPID(Motor& motor, float kp, float ki, float kd);
void enableMotorPID(Motor& motor, bool enable);
void setMotorSpeedWithPID(Motor& motor, int speed, int direction);
void updateMotorPID(Motor& motor);
void resetMotorPID(Motor& motor);

// ISR handlers (khai báo để dùng trong main.cpp)
void IRAM_ATTR encoder1_ISR();
void IRAM_ATTR encoder2_ISR();
void IRAM_ATTR encoder3_ISR();

// Utility functions
void printMotorStatus(Motor& motor);
void printAllMotorStatus();

// Task cho FreeRTOS (nếu cần)
void motorControlTask(void* parameter);

// ============================================
// ENCODER DEBUG
// ============================================
// Debug encoder liên tục (không phụ thuộc lệnh điều khiển)
void debugEncoderContinuous();

#endif // MOTOR_H
