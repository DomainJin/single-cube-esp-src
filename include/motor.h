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
#define MOTOR_1_ENCODER_B 32  // GPIO 32 - INPUT (có ADC)

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
#define PWM_FREQ        5000  // 5kHz PWM frequency
#define PWM_RESOLUTION  8     // 8-bit resolution (0-255)

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
#define MOTOR_STOP       0
#define MOTOR_FORWARD    1
#define MOTOR_BACKWARD   2
#define MOTOR_BRAKE      3

#define MAX_SPEED       255
#define MIN_SPEED       0

// ============================================
// ENCODER CONFIGURATION
// ============================================
#define ENCODER_PPR     20    // Pulses Per Revolution (tùy encoder của bạn)
#define ENCODER_DEBOUNCE_MS  2   // Thời gian chống nhiễu (ms)

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
    
    // PID control (nếu cần)
    float kp, ki, kd;        // PID gains
    float error_sum;         // Tích phân error
    float last_error;        // Error trước đó
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

// PID control (nếu cần)
void setMotorPID(Motor& motor, float kp, float ki, float kd);
void updateMotorPID(Motor& motor, float target_rpm);

// ISR handlers (khai báo để dùng trong main.cpp)
void IRAM_ATTR encoder1_ISR();
void IRAM_ATTR encoder2_ISR();
void IRAM_ATTR encoder3_ISR();

// Utility functions
void printMotorStatus(Motor& motor);
void printAllMotorStatus();

// Task cho FreeRTOS (nếu cần)
void motorControlTask(void* parameter);

#endif // MOTOR_H
