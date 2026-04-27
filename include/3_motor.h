#ifndef THREE_MOTOR_H
#define THREE_MOTOR_H

#include <Arduino.h>

//================= Pin mapping =================
// Motor 1 (L298 #1)
#define MOTOR1_EN   4
#define MOTOR1_DIR  2
#define ENC1_A     21
#define ENC1_B     13
// Motor 2 (L298 #1)
#define MOTOR2_EN  16
#define MOTOR2_DIR 17
#define ENC2_A     14
#define ENC2_B     27
// Motor 3 (L298 #2)
#define MOTOR3_EN   5
#define MOTOR3_DIR 18
#define ENC3_A     26
#define ENC3_B     25

//================= Encoder spec =================
#define ENCODER_PPR  12
#define GEAR_RATIO   21   // Đo thực tế từ test: CPR_X1≈253 ≈ 12×21
#define ENCODER_CPR  (ENCODER_PPR * 4 * GEAR_RATIO)  // X4 quadrature (cả 2 cạnh, cả 2 kênh) = 1008

//================= PWM config ===================
#define PWM_FREQ        2000
#define PWM_RESOLUTION  8      // 0..255
#define M1_CH 0
#define M2_CH 1
#define M3_CH 2

//================= PID config ===================
// Giới hạn output (50% công suất). Đổi 255 nếu muốn full.
extern const int OUT_MAX;
extern const int OUT_MIN;        // PWM tối thiểu để khởi động motor (vượt ma sát)
extern const float MIN_DRIVE;    // PWM tối thiểu khi có sai số
extern const float MIN_ERR_FOR_DRIVE; // RPM, vùng chết (giảm xuống)
// PID gains - Tăng mạnh để đáp ứng nhanh
extern const float Kp;
extern const float Ki;
extern const float Kd;
// Feedforward - PWM cơ bản để duy trì tốc độ
extern const float BASE_PWM_PER_RPM;
// Mục tiêu RPM (ví dụ ~50% của 280 RPM ~ 140 RPM)
extern float SETPOINT_RPM;
// Lọc RPM (EMA) - Tăng để phản ứng nhanh hơn
extern const float RPM_ALPHA;
extern const unsigned long RPM_WINDOW_MS;

//================= Omni Robot Geometry ===========
// Cấu hình robot omni 3 bánh (tam giác đều)
extern const float WHEEL_RADIUS_CM;      // Bán kính bánh xe: 5cm (đường kính 10cm)
extern const float ROBOT_RADIUS_CM;      // Khoảng cách từ tâm đến bánh xe: 35cm
extern const float WHEEL_THICKNESS_CM;   // Độ dày bánh xe: 3.5cm

//================= Struct cho từng motor =========
struct Motor {
  // pins
  uint8_t en, dir, encA, encB, pwmCh;
  // encoder counter
  volatile long count;
  long lastCount;
  // rpm
  float rpmRaw, rpmFilt;
  // PID state
  float integral, lastErr, out;
  unsigned long lastPIDt;
};

//================= External motor instances ======
extern Motor m1, m2, m3;
extern unsigned long lastRPMt;

//================= ISR declarations (X4 quadrature: 2 ISR per motor) ==
void IRAM_ATTR enc1_A_ISR();
void IRAM_ATTR enc1_B_ISR();
void IRAM_ATTR enc2_A_ISR();
void IRAM_ATTR enc2_B_ISR();
void IRAM_ATTR enc3_A_ISR();
void IRAM_ATTR enc3_B_ISR();

//================= Function declarations =========
void setupMotor(Motor &m);
void setupEncoder(Motor &m, void (*isrA)(), void (*isrB)());
void applyPWM(Motor &m, int pwm, bool forward);
void updateRPMAll();
float stepPID(Motor &m, float target_rpm);

// Initialization and update functions
void init3Motors();
void update3Motors();

//================= Omni Robot Control ============
// Chuyển đổi vận tốc (cm/s) sang RPM
float velocityToRPM(float velocity_cm_s);

// Đặt tốc độ cho một motor (RPM, có thể âm)
void setMotorTargetRPM(Motor &m, float rpm);

// Điều khiển robot với vận tốc từ sensor fusion
void omniSetVelocity(float vx, float vy, float omega);

// Bù trượt từ sensor fusion vào target hiện tại (không ghi đè)
void omniCompensateDrift(float vx, float vy, float omega, float gain = 0.1f);

// 5 hàm điều khiển cơ bản
void omniForward(float speed_cm_s);   // Tiến
void omniBackward(float speed_cm_s);  // Lùi
void omniStrafeLeft(float speed_cm_s);  // Trái (strafe)
void omniStrafeRight(float speed_cm_s); // Phải (strafe)
void omniRotate(int dir, float rpm = 100.0); // Xoay tròn (dir: 1=thuận, 0=nghịch)
void omniStop();                       // Dừng

// Position control: quay đúng số vòng rồi tự dừng
// revs > 0: tiến, revs < 0: lùi; rpm là tốc độ (mặc định 60)
void motorRunRevs(Motor &m, float revs, float rpm = 60.0f);
bool motorPosActive(Motor &m);  // true nếu đang chạy position mode

// Test mode: tiến N vòng → dừng 500ms → lùi N vòng → báo drift
void motorStartTest(int motorNum, float revs = 5.0f, float rpm = 60.0f);
void updateMotorTest();  // gọi từ loop()

// Record & Return: chạy tự do → STOP (soft, vẫn ghi) → RETURN đúng số vòng
void motorRecordStart(int motorNum, float rpm = 100.0f);
void motorRecordStop(int motorNum);
void motorRecordReturn(int motorNum);
void updateMotorRecord();  // gọi từ loop()

#endif // THREE_MOTOR_H
