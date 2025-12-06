/**
 * @file omni.h
 * @brief 3-Wheel Omni Robot Control System
 * @date 2025-12-06
 * 
 * Hệ thống điều khiển robot 3 bánh omni (holonomic)
 * - Inverse kinematics cho di chuyển toàn hướng
 * - PID control cho từng motor
 * - Các hàm di chuyển cao cấp: translate, rotate, strafe
 */

#ifndef OMNI_H
#define OMNI_H

#include <Arduino.h>
#include <math.h>

// ============================================
// OMNI ROBOT CONFIGURATION
// ============================================

// Cấu hình vật lý robot
// Bánh xe: đường kính 100mm → bán kính 50mm
// Tam giác đều cạnh 40cm → bán kính nội tiếp R = a/(2√3) = 40/(2√3) ≈ 11.547cm
#define OMNI_WHEEL_RADIUS       0.050f      // Bán kính bánh xe (m) - 50mm
#define OMNI_ROBOT_RADIUS       0.23094f    // Bán kính robot (m) - từ tâm đến bánh xe
                                            // Công thức: R = cạnh_tam_giác / √3 = 0.4 / 1.732 = 0.23094m
#define OMNI_GEAR_RATIO         30.0f       // Tỷ số truyền 1:30
#define OMNI_MAX_SPEED          255         // PWM max (0-255)
#define OMNI_MIN_SPEED          60          // PWM min để bù voltage drop L298N (~2V)
#define OMNI_MAX_WHEEL_RPM      333.0f      // RPM max của bánh xe (theo spec)

// Góc đặt bánh xe (degree) - Standard Y-shape configuration
// Motor đặt cân đối 120° nhau, với Motor 1 ở phía trước
#define OMNI_MOTOR1_ANGLE       90.0f       // Motor 1: 90° (front, trục X+)
#define OMNI_MOTOR2_ANGLE       210.0f      // Motor 2: 210° (back-left)
#define OMNI_MOTOR3_ANGLE       330.0f      // Motor 3: 330° (back-right)

// PID constants cho omni control
#define OMNI_KP                 2.0f        // Proportional gain
#define OMNI_KI                 0.5f        // Integral gain
#define OMNI_KD                 0.1f        // Derivative gain

// ============================================
// OMNI ROBOT STRUCTURE
// ============================================

/**
 * @brief Omni robot state structure
 */
typedef struct {
    // Vận tốc mục tiêu (m/s và rad/s)
    float target_vx;        // Vận tốc X (forward/backward)
    float target_vy;        // Vận tốc Y (left/right strafe)
    float target_omega;     // Vận tốc góc (rotation)
    
    // Vận tốc hiện tại (từ encoder)
    float current_vx;
    float current_vy;
    float current_omega;
    
    // Vị trí ước tính (odometry)
    float pos_x;            // Vị trí X (m)
    float pos_y;            // Vị trí Y (m)
    float heading;          // Hướng (radians)
    
    // Vận tốc bánh xe (RPM)
    float wheel_rpm[3];     // RPM của 3 bánh xe
    int wheel_pwm[3];       // PWM của 3 bánh xe (-255 to 255)
    
    // PID error tracking
    float error_sum[3];     // Tích phân sai số
    float last_error[3];    // Sai số trước đó
    
    // Control flags
    bool enabled;           // Robot có được kích hoạt không
    unsigned long last_update; // Timestamp cập nhật cuối
    
} OmniRobotState;

// ============================================
// FUNCTION PROTOTYPES
// ============================================

/**
 * @brief Khởi tạo hệ thống omni robot
 */
void setupOmni();

/**
 * @brief Cập nhật trạng thái robot (gọi trong loop)
 */
void updateOmni();

/**
 * @brief Reset odometry về gốc tọa độ
 */
void resetOmniOdometry();

/**
 * @brief Enable/disable robot control
 * @param enable true để kích hoạt, false để dừng
 */
void setOmniEnabled(bool enable);

// ============================================
// INVERSE KINEMATICS - Tính vận tốc bánh xe
// ============================================

/**
 * @brief Tính vận tốc bánh xe từ vận tốc robot (inverse kinematics)
 * @param vx Vận tốc X của robot (m/s)
 * @param vy Vận tốc Y của robot (m/s)
 * @param omega Vận tốc góc của robot (rad/s)
 * @param wheel_speeds Output: vận tốc 3 bánh xe (m/s)
 */
void omniInverseKinematics(float vx, float vy, float omega, float wheel_speeds[3]);

/**
 * @brief Chuyển đổi vận tốc bánh xe (m/s) sang PWM
 * @param wheel_speed Vận tốc bánh xe (m/s)
 * @return PWM value (-255 to 255)
 */
int omniSpeedToPWM(float wheel_speed);

// ============================================
// FORWARD KINEMATICS - Tính vận tốc robot từ encoder
// ============================================

/**
 * @brief Tính vận tốc robot từ vận tốc bánh xe (forward kinematics)
 * @param wheel_speeds Vận tốc 3 bánh xe (m/s)
 * @param vx Output: vận tốc X của robot (m/s)
 * @param vy Output: vận tốc Y của robot (m/s)
 * @param omega Output: vận tốc góc của robot (rad/s)
 */
void omniForwardKinematics(float wheel_speeds[3], float *vx, float *vy, float *omega);

/**
 * @brief Tính vận tốc bánh xe từ RPM encoder
 * @param rpm RPM của bánh xe
 * @return Vận tốc (m/s)
 */
float omniRPMToSpeed(float rpm);

// ============================================
// HIGH-LEVEL CONTROL FUNCTIONS
// ============================================

/**
 * @brief Di chuyển robot theo vận tốc Cartesian
 * @param vx Vận tốc X (m/s) - forward(+)/backward(-)
 * @param vy Vận tốc Y (m/s) - right(+)/left(-)
 * @param omega Vận tốc góc (rad/s) - CCW(+)/CW(-)
 */
void omniMove(float vx, float vy, float omega);

/**
 * @brief Di chuyển thẳng với tốc độ (cm/s)
 * @param speed_cm_s Tốc độ (cm/s) - dương: tiến, âm: lùi
 */
void omniForward(float speed_cm_s);

/**
 * @brief Di chuyển ngang (strafe) với tốc độ (cm/s)
 * @param speed_cm_s Tốc độ (cm/s) - dương: phải, âm: trái
 */
void omniStrafe(float speed_cm_s);

/**
 * @brief Quay tại chỗ với vận tốc góc (deg/s)
 * @param angular_speed_deg_s Vận tốc góc (deg/s) - dương: CCW, âm: CW
 */
void omniRotate(float angular_speed_deg_s);

/**
 * @brief Di chuyển theo hướng với tốc độ
 * @param angle_deg Góc di chuyển (degrees, 0=forward, 90=right, 180=backward, 270=left)
 * @param speed_cm_s Tốc độ di chuyển (cm/s)
 */
void omniMoveDirection(float angle_deg, float speed_cm_s);

/**
 * @brief Dừng robot
 */
void omniStop();

/**
 * @brief Phanh khẩn cấp (brake)
 */
void omniBrake();

// ============================================
// ADVANCED CONTROL FUNCTIONS
// ============================================

/**
 * @brief Di chuyển đến vị trí mục tiêu (relative)
 * @param delta_x Khoảng cách X (cm)
 * @param delta_y Khoảng cách Y (cm)
 * @param speed_cm_s Tốc độ di chuyển (cm/s)
 * @return true nếu đã đến đích
 */
bool omniMoveTo(float delta_x, float delta_y, float speed_cm_s);

/**
 * @brief Quay đến góc mục tiêu
 * @param target_angle_deg Góc mục tiêu (degrees)
 * @param angular_speed_deg_s Tốc độ quay (deg/s)
 * @return true nếu đã đến góc mục tiêu
 */
bool omniRotateTo(float target_angle_deg, float angular_speed_deg_s);

/**
 * @brief Điều chỉnh hướng robot (field-centric control)
 * @param vx_field Vận tốc X trong hệ tọa độ sân (m/s)
 * @param vy_field Vận tốc Y trong hệ tọa độ sân (m/s)
 * @param omega Vận tốc góc (rad/s)
 */
void omniFieldCentricMove(float vx_field, float vy_field, float omega);

/**
 * @brief Lấy trạng thái robot hiện tại
 * @return Pointer to OmniRobotState
 */
OmniRobotState* getOmniState();

/**
 * @brief In thông tin debug về serial
 */
void omniPrintDebug();

/**
 * @brief Cập nhật odometry từ encoder
 * @param dt Delta time (seconds)
 */
void omniUpdateOdometry(float dt);

// ============================================
// UTILITY FUNCTIONS
// ============================================

/**
 * @brief Chuẩn hóa góc về khoảng [-PI, PI]
 * @param angle Góc (radians)
 * @return Góc đã chuẩn hóa
 */
float omniNormalizeAngle(float angle);

/**
 * @brief Chuyển đổi degrees sang radians
 */
float omniDegToRad(float degrees);

/**
 * @brief Chuyển đổi radians sang degrees
 */
float omniRadToDeg(float radians);

/**
 * @brief Constrain vận tốc trong giới hạn
 * @param speed Vận tốc cần giới hạn
 * @param max_speed Vận tốc tối đa
 * @return Vận tốc đã giới hạn
 */
float omniConstrainSpeed(float speed, float max_speed);

#endif // OMNI_H
