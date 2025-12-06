/**
 * @file omni.cpp
 * @brief 3-Wheel Omni Robot Control Implementation
 * @date 2025-12-06
 */

#include "omni.h"
#include "motor.h"
#include "udpconfig.h"  // ✅ Thêm để gửi tọa độ qua UDP

// Global robot state
static OmniRobotState omni_state = {0};

// ============================================
// INITIALIZATION
// ============================================

void setupOmni() {
    Serial.println("=== Omni Robot Setup ===");
    
    // Reset state
    memset(&omni_state, 0, sizeof(OmniRobotState));
    
    omni_state.enabled = false;
    omni_state.last_update = millis();
    
    // Setup motors (đã setup trong main)
    // Motor 1, 2, 3 tương ứng với 3 bánh omni
    
    Serial.println("Omni Robot initialized");
    Serial.printf("  Wheel radius: %.3f m\n", OMNI_WHEEL_RADIUS);
    Serial.printf("  Robot radius: %.3f m\n", OMNI_ROBOT_RADIUS);
    Serial.printf("  Motor angles: %.0f, %.0f, %.0f degrees\n", 
                  OMNI_MOTOR1_ANGLE, OMNI_MOTOR2_ANGLE, OMNI_MOTOR3_ANGLE);
}

void updateOmni() {
    unsigned long current_time = millis();
    float dt = (current_time - omni_state.last_update) / 1000.0f; // seconds
    
    if (dt < 0.001f) return; // Skip if too fast
    
    if (!omni_state.enabled) {
        omniStop();
        omni_state.last_update = current_time;
        return;
    }
    
    // 1. Tính vận tốc bánh xe từ mục tiêu (Inverse Kinematics)
    float wheel_speeds[3];
    omniInverseKinematics(omni_state.target_vx, 
                         omni_state.target_vy, 
                         omni_state.target_omega, 
                         wheel_speeds);
    
    // 2. Chuyển đổi sang PWM
    for (int i = 0; i < 3; i++) {
        omni_state.wheel_pwm[i] = omniSpeedToPWM(wheel_speeds[i]);
    }
    
    // ✅ DEBUG: In thông tin target, wheel speeds và PWM
    static unsigned long last_debug = 0;
    if (current_time - last_debug > 1000) {  // Log mỗi 1 giây
        last_debug = current_time;
        Serial.printf("[OMNI] Target: vx=%.3f vy=%.3f omega=%.3f\n",
                      omni_state.target_vx, omni_state.target_vy, omni_state.target_omega);
        Serial.printf("[OMNI] Wheel speeds (m/s): [%.3f, %.3f, %.3f]\n",
                      wheel_speeds[0], wheel_speeds[1], wheel_speeds[2]);
        Serial.printf("[OMNI] PWM: [%d, %d, %d]\n",
                      omni_state.wheel_pwm[0], omni_state.wheel_pwm[1], omni_state.wheel_pwm[2]);
    }
    
    // 3. Điều khiển motors - SỬA: dùng MOTOR_FORWARD/MOTOR_BACKWARD thay vì true/false
    setMotorSpeed(motor1, abs(omni_state.wheel_pwm[0]), 
                  omni_state.wheel_pwm[0] >= 0 ? MOTOR_FORWARD : MOTOR_BACKWARD);
    setMotorSpeed(motor2, abs(omni_state.wheel_pwm[1]), 
                  omni_state.wheel_pwm[1] >= 0 ? MOTOR_FORWARD : MOTOR_BACKWARD);
    setMotorSpeed(motor3, abs(omni_state.wheel_pwm[2]), 
                  omni_state.wheel_pwm[2] >= 0 ? MOTOR_FORWARD : MOTOR_BACKWARD);
    
    // 4. Đọc encoder và tính vận tốc thực tế (CHỈ mỗi 100ms để tránh trả về 0)
    static unsigned long last_rpm_update = 0;
    if (current_time - last_rpm_update >= 100) {  // ✅ Tính RPM mỗi 100ms
        last_rpm_update = current_time;
        
        omni_state.wheel_rpm[0] = getMotorRPM(motor1);
        omni_state.wheel_rpm[1] = getMotorRPM(motor2);
        omni_state.wheel_rpm[2] = getMotorRPM(motor3);
    }
    // Nếu chưa đủ 100ms, giữ nguyên giá trị RPM cũ trong omni_state
    
    // 5. Forward Kinematics - tính vận tốc robot từ encoder
    float wheel_speeds_actual[3];
    for (int i = 0; i < 3; i++) {
        wheel_speeds_actual[i] = omniRPMToSpeed(omni_state.wheel_rpm[i]);
    }
    
    omniForwardKinematics(wheel_speeds_actual, 
                         &omni_state.current_vx,
                         &omni_state.current_vy,
                         &omni_state.current_omega);
    
    // 6. Cập nhật odometry
    omniUpdateOdometry(dt);
    
    // 7. Gửi tọa độ robot qua UDP để server cập nhật map
    sendRobotPosition(omni_state.pos_x, omni_state.pos_y, omni_state.heading,
                      omni_state.current_vx, omni_state.current_vy, omni_state.current_omega);
    
    omni_state.last_update = current_time;
}

void resetOmniOdometry() {
    omni_state.pos_x = 0;
    omni_state.pos_y = 0;
    omni_state.heading = 0;
    Serial.println("Omni odometry reset");
}

void setOmniEnabled(bool enable) {
    omni_state.enabled = enable;
    if (!enable) {
        omniStop();
    }
    Serial.printf("Omni robot %s\n", enable ? "ENABLED" : "DISABLED");
}

// ============================================
// INVERSE KINEMATICS
// ============================================

void omniInverseKinematics(float vx, float vy, float omega, float wheel_speeds[3]) {
    // ✅ Standard 3-wheel omni inverse kinematics
    // Motors at 90°, 210°, 330° (Y-shape with front motor at 90°)
    //
    // For FORWARD (Vy): all 3 motors same direction
    // For STRAFE (Vx): front motor full, back motors opposite
    // For ROTATE (ω): all motors same direction
    
    float theta1 = omniDegToRad(OMNI_MOTOR1_ANGLE);  
    float theta2 = omniDegToRad(OMNI_MOTOR2_ANGLE);  
    float theta3 = omniDegToRad(OMNI_MOTOR3_ANGLE);  
    
    // Standard omni formula: V = -sin(θ)*Vx + cos(θ)*Vy + R*ω
    wheel_speeds[0] = -sin(theta1) * vx + cos(theta1) * vy + OMNI_ROBOT_RADIUS * omega;
    wheel_speeds[1] = -sin(theta2) * vx + cos(theta2) * vy + OMNI_ROBOT_RADIUS * omega;
    wheel_speeds[2] = -sin(theta3) * vx + cos(theta3) * vy + OMNI_ROBOT_RADIUS * omega;
}

int omniSpeedToPWM(float wheel_speed) {
    // Chuyển đổi vận tốc bánh xe (m/s) sang PWM
    // 1. Tính RPM từ vận tốc: RPM = v / (2πr) * 60
    // 2. Map RPM -> PWM: PWM = (RPM / MAX_RPM) * 255
    // 3. Thêm offset để bù voltage drop L298N (~2V)
    
    // ✅ NẾU tốc độ quá nhỏ (< 0.05 m/s = 26 RPM), trả về 0
    if (abs(wheel_speed) < 0.05f) return 0;
    
    float rpm = (wheel_speed / (2.0f * PI * OMNI_WHEEL_RADIUS)) * 60.0f;
    
    // Map RPM sang PWM (0-333 RPM -> 60-255 PWM để bù L298N voltage drop)
    // Sử dụng range 60-255 thay vì 0-255
    float pwm_range = OMNI_MAX_SPEED - OMNI_MIN_SPEED;  // 255 - 60 = 195
    float pwm_float = (abs(rpm) / OMNI_MAX_WHEEL_RPM) * pwm_range + OMNI_MIN_SPEED;
    
    int pwm = (int)pwm_float;
    
    // Giữ dấu của wheel_speed
    if (wheel_speed < 0) pwm = -pwm;
    
    // Constrain
    if (pwm > OMNI_MAX_SPEED) pwm = OMNI_MAX_SPEED;
    if (pwm < -OMNI_MAX_SPEED) pwm = -OMNI_MAX_SPEED;
    
    return pwm;
}

// ============================================
// FORWARD KINEMATICS
// ============================================

void omniForwardKinematics(float wheel_speeds[3], float *vx, float *vy, float *omega) {
    // Công thức forward kinematics (pseudo-inverse):
    // Inverse: V[i] = Vy*cos(θ[i]) + Vx*sin(θ[i]) + R*ω
    // Forward: Vx = k * sum(sin(θ[i]) * V[i])
    //          Vy = k * sum(cos(θ[i]) * V[i])
    //          ω = k * sum(V[i]) / R
    
    float theta1 = omniDegToRad(OMNI_MOTOR1_ANGLE);
    float theta2 = omniDegToRad(OMNI_MOTOR2_ANGLE);
    float theta3 = omniDegToRad(OMNI_MOTOR3_ANGLE);
    
    // Hệ số k = 2/3 cho 3 bánh xe cân bằng
    float k = 2.0f / 3.0f;
    
    *vx = k * (sin(theta1) * wheel_speeds[0] 
               + sin(theta2) * wheel_speeds[1] 
               + sin(theta3) * wheel_speeds[2]);
    
    *vy = k * (cos(theta1) * wheel_speeds[0] 
               + cos(theta2) * wheel_speeds[1] 
               + cos(theta3) * wheel_speeds[2]);
    
    *omega = k * (wheel_speeds[0] + wheel_speeds[1] + wheel_speeds[2]) / OMNI_ROBOT_RADIUS;
}

float omniRPMToSpeed(float rpm) {
    // Chuyển đổi RPM sang vận tốc tuyến tính (m/s)
    // v = (RPM * 2π * r) / 60
    return (rpm * 2.0f * PI * OMNI_WHEEL_RADIUS) / 60.0f;
}

// ============================================
// HIGH-LEVEL CONTROL
// ============================================

void omniMove(float vx, float vy, float omega) {
    omni_state.target_vx = vx;
    omni_state.target_vy = vy;
    omni_state.target_omega = omega;
}

void omniForward(float speed_cm_s) {
    float vy = speed_cm_s / 100.0f; // cm/s to m/s
    omniMove(0, vy, 0);  // ✅ FORWARD: Vy dương = tiến thẳng
}

void omniStrafe(float speed_cm_s) {
    float vx = speed_cm_s / 100.0f; // cm/s to m/s
    omniMove(vx, 0, 0);  // ✅ STRAFE: Vx dương = sang phải, Vx âm = sang trái
}

void omniRotate(float angular_speed_deg_s) {
    float omega = omniDegToRad(angular_speed_deg_s);
    omniMove(0, 0, omega);
}

void omniMoveDirection(float angle_deg, float speed_cm_s) {
    float speed_m_s = speed_cm_s / 100.0f;
    float angle_rad = omniDegToRad(angle_deg);
    
    float vx = speed_m_s * cos(angle_rad);
    float vy = speed_m_s * sin(angle_rad);
    
    omniMove(vx, vy, 0);
}

void omniStop() {
    omni_state.target_vx = 0;
    omni_state.target_vy = 0;
    omni_state.target_omega = 0;
    
    setMotorSpeed(motor1, 0, true);
    setMotorSpeed(motor2, 0, true);
    setMotorSpeed(motor3, 0, true);
}

void omniBrake() {
    omniStop();
    
    // Có thể thêm dynamic braking bằng cách đảo chiều ngắn hạn
    // hoặc short brake (nối 2 cực motor lại với nhau)
}

// ============================================
// ADVANCED CONTROL
// ============================================

bool omniMoveTo(float delta_x, float delta_y, float speed_cm_s) {
    float distance = sqrt(delta_x * delta_x + delta_y * delta_y);
    float threshold = 2.0f; // 2 cm tolerance
    
    if (distance < threshold) {
        omniStop();
        return true; // Đã đến đích
    }
    
    // Tính góc di chuyển
    float angle_rad = atan2(delta_y, delta_x);
    float angle_deg = omniRadToDeg(angle_rad);
    
    omniMoveDirection(angle_deg, speed_cm_s);
    return false;
}

bool omniRotateTo(float target_angle_deg, float angular_speed_deg_s) {
    float current_angle_deg = omniRadToDeg(omni_state.heading);
    float error = target_angle_deg - current_angle_deg;
    
    // Normalize error to [-180, 180]
    while (error > 180) error -= 360;
    while (error < -180) error += 360;
    
    float threshold = 2.0f; // 2 degrees tolerance
    
    if (abs(error) < threshold) {
        omniStop();
        return true; // Đã đến góc mục tiêu
    }
    
    // Điều khiển với tốc độ giảm dần khi gần đích
    float speed = angular_speed_deg_s * (abs(error) / 180.0f);
    speed = constrain(speed, 10.0f, angular_speed_deg_s);
    
    if (error > 0) {
        omniRotate(speed);
    } else {
        omniRotate(-speed);
    }
    
    return false;
}

void omniFieldCentricMove(float vx_field, float vy_field, float omega) {
    // Field-centric control: điều khiển theo hệ tọa độ sân (không phụ thuộc hướng robot)
    // Chuyển đổi từ hệ tọa độ field sang robot-centric
    
    float heading = omni_state.heading;
    
    // Rotation matrix
    float vx_robot = vx_field * cos(-heading) - vy_field * sin(-heading);
    float vy_robot = vx_field * sin(-heading) + vy_field * cos(-heading);
    
    omniMove(vx_robot, vy_robot, omega);
}

OmniRobotState* getOmniState() {
    return &omni_state;
}

void omniPrintDebug() {
    Serial.println("=== Omni Robot State ===");
    Serial.printf("Target velocity: vx=%.3f, vy=%.3f, omega=%.3f\n",
                  omni_state.target_vx, omni_state.target_vy, omni_state.target_omega);
    Serial.printf("Current velocity: vx=%.3f, vy=%.3f, omega=%.3f\n",
                  omni_state.current_vx, omni_state.current_vy, omni_state.current_omega);
    Serial.printf("Position: x=%.3f, y=%.3f, heading=%.1f deg\n",
                  omni_state.pos_x, omni_state.pos_y, omniRadToDeg(omni_state.heading));
    Serial.printf("Wheel PWM: [%d, %d, %d]\n",
                  omni_state.wheel_pwm[0], omni_state.wheel_pwm[1], omni_state.wheel_pwm[2]);
    Serial.printf("Wheel RPM: [%.1f, %.1f, %.1f]\n",
                  omni_state.wheel_rpm[0], omni_state.wheel_rpm[1], omni_state.wheel_rpm[2]);
    Serial.printf("Enabled: %s\n", omni_state.enabled ? "YES" : "NO");
}

void omniUpdateOdometry(float dt) {
    // ✅ Cập nhật odometry dựa trên TARGET velocity (lệnh điều khiển)
    // Thay vì dùng current velocity từ encoder (bị ảnh hưởng bởi góc motor sai)
    
    float heading = omni_state.heading;
    
    // ✅ Dùng TARGET velocity thay vì current velocity
    float vx_robot = omni_state.target_vx;
    float vy_robot = omni_state.target_vy;
    float omega = omni_state.target_omega;
    
    // Chuyển đổi vận tốc robot (local frame) sang global frame
    // Global X = Robot Vx*cos(θ) - Robot Vy*sin(θ)
    // Global Y = Robot Vx*sin(θ) + Robot Vy*cos(θ)
    float vx_global = vx_robot * cos(heading) - vy_robot * sin(heading);
    float vy_global = vx_robot * sin(heading) + vy_robot * cos(heading);
    
    // Update position (integration)
    omni_state.pos_x += vx_global * dt;
    omni_state.pos_y += vy_global * dt;
    omni_state.heading += omega * dt;
    
    // Normalize heading to [-PI, PI]
    omni_state.heading = omniNormalizeAngle(omni_state.heading);
}

// ============================================
// UTILITY FUNCTIONS
// ============================================

float omniNormalizeAngle(float angle) {
    while (angle > PI) angle -= 2 * PI;
    while (angle < -PI) angle += 2 * PI;
    return angle;
}

float omniDegToRad(float degrees) {
    return degrees * PI / 180.0f;
}

float omniRadToDeg(float radians) {
    return radians * 180.0f / PI;
}

float omniConstrainSpeed(float speed, float max_speed) {
    if (speed > max_speed) return max_speed;
    if (speed < -max_speed) return -max_speed;
    return speed;
}
