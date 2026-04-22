#include "3_motor.h"

//================= PID config ===================
const int OUT_MAX = 255;
const int OUT_MIN = 0;        // PWM tối thiểu để khởi động motor (vượt ma sát)
const float MIN_DRIVE = 25.0f; // PWM tối thiểu khi có sai số
const float MIN_ERR_FOR_DRIVE = 1.5f; // RPM, vùng chết (giảm xuống)
// PID gains - Tăng mạnh để đáp ứng nhanh
const float Kp = 3.5f;  // Tăng từ 2.0
const float Ki = 0.8f;  // Tăng từ 0.3
const float Kd = 0.2f;  // Tăng từ 0.15
// Feedforward - PWM cơ bản để duy trì tốc độ
const float BASE_PWM_PER_RPM = 0.65f; // ~58 PWM cho 90 RPM
// Mục tiêu RPM (ví dụ ~50% của 280 RPM ~ 140 RPM)
float SETPOINT_RPM = 90.0f;
// Lọc RPM (EMA) - Tăng để phản ứng nhanh hơn
const float RPM_ALPHA = 0.4f; // 0..1, càng nhỏ càng mượt
const unsigned long RPM_WINDOW_MS = 100; // cửa sổ tính RPM (giảm xuống để đồng bộ với PID)

//================= Omni Robot Geometry ===========
const float WHEEL_RADIUS_CM = 5.0f;       // Bán kính bánh xe: 5cm (đường kính 10cm)
const float ROBOT_RADIUS_CM = 35.0f;      // Khoảng cách từ tâm đến bánh xe: 35cm
const float WHEEL_THICKNESS_CM = 3.5f;    // Độ dày bánh xe: 3.5cm

// Target RPM cho từng motor (PID sẽ điều khiển về giá trị này)
float m1_target_rpm = 0.0f;
float m2_target_rpm = 0.0f;
float m3_target_rpm = 0.0f;

//================= Motor instances ===============
Motor m1 = {MOTOR1_EN, MOTOR1_DIR, ENC1_A, ENC1_B, M1_CH, 0, 0, 0, 0, 0, 0, 0};
Motor m2 = {MOTOR2_EN, MOTOR2_DIR, ENC2_A, ENC2_B, M2_CH, 0, 0, 0, 0, 0, 0, 0};
Motor m3 = {MOTOR3_EN, MOTOR3_DIR, ENC3_A, ENC3_B, M3_CH, 0, 0, 0, 0, 0, 0, 0};

unsigned long lastRPMt = 0;

//================= ISR ===========================
// Xác định chiều quay dựa trên chân B khi chân A RISING
void IRAM_ATTR enc1ISR() {
  if (digitalRead(m1.encB) == HIGH) {
    m1.count++;  // Quay tiến
  } else {
    m1.count--;  // Quay lùi
  }
}

void IRAM_ATTR enc2ISR() {
  if (digitalRead(m2.encB) == HIGH) {
    m2.count++;
  } else {
    m2.count--;
  }
}

void IRAM_ATTR enc3ISR() {
  if (digitalRead(m3.encB) == HIGH) {
    m3.count++;
  } else {
    m3.count--;
  }
}

//================= Helper ========================
void setupMotor(Motor &m) {
  ledcSetup(m.pwmCh, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(m.en, m.pwmCh);
  pinMode(m.dir, OUTPUT);
  digitalWrite(m.dir, LOW); // Mặc định LOW (sẽ điều khiển theo target RPM)
  ledcWrite(m.pwmCh, 0);
}

void setupEncoder(Motor &m, void (*isr)()) {
  pinMode(m.encA, INPUT_PULLUP);
  pinMode(m.encB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(m.encA), isr, RISING);  // RISING cho ổn định
  m.count = 0;
  m.lastCount = 0;
  m.rpmRaw = m.rpmFilt = 0;
  m.integral = m.lastErr = 0;
  m.lastPIDt = millis();
}

void applyPWM(Motor &m, int pwm, bool forward) {
  if (pwm < 0) pwm = 0;
  if (pwm > OUT_MAX) pwm = OUT_MAX;
  
  // Điều khiển chiều quay
  digitalWrite(m.dir, forward ? HIGH : LOW);
  
  ledcWrite(m.pwmCh, pwm);
}

// Cập nhật RPM cho tất cả motor
void updateRPMAll() {
  unsigned long now = millis();
  if (now - lastRPMt < RPM_WINDOW_MS) return;
  float dt = (now - lastRPMt) / 1000.0f;

  auto updateOne = [&](Motor &m) {
    long c;
    noInterrupts(); c = m.count; interrupts();
    long d = c - m.lastCount;
    // Giữ nguyên dấu của d để RPM có thể âm/dương
    float rpmRaw = (d / (float)ENCODER_CPR) * (60.0f / dt);
    m.rpmRaw = rpmRaw;
    m.rpmFilt = RPM_ALPHA * rpmRaw + (1 - RPM_ALPHA) * m.rpmFilt;
    m.lastCount = c;
  };

  updateOne(m1);
  updateOne(m2);
  updateOne(m3);
  lastRPMt = now;
}

// PID cho một motor - RPM có dấu, target_rpm có dấu
float stepPID(Motor &m, float target_rpm) {
  unsigned long now = millis();
  float dt = (now - m.lastPIDt) / 1000.0f;
  if (dt <= 0) dt = 1e-3;

  // Sai số có dấu: target - feedback (cả 2 có thể âm/dương)
  float err = target_rpm - m.rpmFilt;

  // Vùng chết sai số nhỏ - giữ feedforward PWM
  if (fabs(err) < MIN_ERR_FOR_DRIVE) {
    m.integral *= 0.95; // Giảm dần integral
    m.lastErr = err;
    m.lastPIDt = now;
    // Giữ feedforward PWM (có dấu theo target)
    float feedforward = target_rpm * BASE_PWM_PER_RPM;
    float out = constrain(fabs(feedforward), OUT_MIN, OUT_MAX);
    return (target_rpm >= 0) ? out : -out;
  }

  // Reset integral nếu overshoots (RPM vượt quá nhiều)
  if (fabs(target_rpm - m.rpmFilt) > 20) {
    m.integral *= 0.5; // Giảm integral 50%
  }

  m.integral += err * dt;
  // Integral limit
  if (m.integral > 150) m.integral = 150;
  if (m.integral < -150) m.integral = -150;

  float deriv = (err - m.lastErr) / dt;
  
  // Feedforward + PID correction (cả 2 đều có dấu)
  float feedforward = target_rpm * BASE_PWM_PER_RPM;
  float pidCorrection = Kp * err + Ki * m.integral + Kd * deriv;
  float out = feedforward + pidCorrection;

  // Lấy độ lớn và clamp
  float absPWM = fabs(out);
  if (absPWM > OUT_MAX) absPWM = OUT_MAX;
  if (absPWM < OUT_MIN) absPWM = OUT_MIN;
  
  // Trả về PWM có dấu
  float signedPWM = (out >= 0) ? absPWM : -absPWM;
  m.out = absPWM;
  m.lastErr = err;
  m.lastPIDt = now;
  return signedPWM;
}

//================= Initialization ================
void init3Motors() {
  setupMotor(m1);
  setupMotor(m2);
  setupMotor(m3);

  setupEncoder(m1, enc1ISR);
  setupEncoder(m2, enc2ISR);
  setupEncoder(m3, enc3ISR);

  lastRPMt = millis();
  Serial.println("[3_motor] Init done");
}

//================= Main update ===================
void update3Motors() {
  updateRPMAll();

  float pwm1 = stepPID(m1, m1_target_rpm);
  float pwm2 = stepPID(m2, m2_target_rpm);
  float pwm3 = stepPID(m3, m3_target_rpm);

  // Áp dụng PWM và chiều quay (PWM có dấu)
  applyPWM(m1, (int)fabs(pwm1), pwm1 >= 0);
  applyPWM(m2, (int)fabs(pwm2), pwm2 >= 0);
  applyPWM(m3, (int)fabs(pwm3), pwm3 >= 0);

  // In trạng thái mỗi 500 ms
  static unsigned long lp = 0;
  if (millis() - lp > 500) {
    lp = millis();
    Serial.print("M1 target="); Serial.print(m1_target_rpm, 1); Serial.print(" RPM="); Serial.print(m1.rpmFilt, 1); Serial.print(" PWM="); Serial.print((int)fabs(pwm1));
    Serial.print(" | M2 target="); Serial.print(m2_target_rpm, 1); Serial.print(" RPM="); Serial.print(m2.rpmFilt, 1); Serial.print(" PWM="); Serial.print((int)fabs(pwm2));
    Serial.print(" | M3 target="); Serial.print(m3_target_rpm, 1); Serial.print(" RPM="); Serial.print(m3.rpmFilt, 1); Serial.print(" PWM="); Serial.println((int)fabs(pwm3));
  }
}

//================= Omni Robot Control ============

// Chuyển đổi vận tốc (cm/s) sang RPM
// RPM = (vận_tốc_cm/s * 60) / (chu_vi_bánh_cm)
// Chu vi bánh = 2 * PI * bán_kính = 2 * PI * 5cm = 31.42 cm
float velocityToRPM(float velocity_cm_s) {
  const float wheel_circumference = 2.0f * PI * WHEEL_RADIUS_CM; // 31.42 cm
  float rpm = (velocity_cm_s * 60.0f) / wheel_circumference;
  return rpm;
}

// Đặt tốc độ mục tiêu cho một motor (RPM, có thể âm)
void setMotorTargetRPM(Motor &m, float rpm) {
  if (&m == &m1) {
    m1_target_rpm = rpm;
  } else if (&m == &m2) {
    m2_target_rpm = rpm;
  } else if (&m == &m3) {
    m3_target_rpm = rpm;
  }
}

//================= Sensor Fusion Control ========
// Điều khiển robot với vận tốc từ sensor fusion (vx, vy: cm/s, omega: rad/s)
void omniSetVelocity(float vx, float vy, float omega) {
  // Chuyển vận tốc tuyến tính sang RPM
  float vx_rpm = velocityToRPM(vx);
  float vy_rpm = velocityToRPM(vy);
  
  // Chuyển vận tốc góc sang RPM bánh xe
  // omega (rad/s) * ROBOT_RADIUS_CM = vận tốc tiếp tuyến (cm/s)
  float omega_rpm = velocityToRPM(omega * ROBOT_RADIUS_CM);
  
  // Omni kinematics với rotation compensation
  // M1 (90°): Vx + omega_contribution
  // M2 (210°): -0.5*Vx - 0.866*Vy + omega_contribution
  // M3 (330°): -0.5*Vx + 0.866*Vy + omega_contribution
  
  m1_target_rpm = vx_rpm + omega_rpm;
  m2_target_rpm = -0.5f * vx_rpm - 0.866f * vy_rpm + omega_rpm;
  m3_target_rpm = -0.5f * vx_rpm + 0.866f * vy_rpm + omega_rpm;
  
  Serial.printf("[OMNI FUSION] Vx=%.1f Vy=%.1f ω=%.2f -> M1=%.1f M2=%.1f M3=%.1f RPM\n",
                vx, vy, omega, m1_target_rpm, m2_target_rpm, m3_target_rpm);
}

// Bù trượt từ sensor fusion - KHÔNG ghi đè target, chỉ cộng thêm
void omniCompensateDrift(float vx, float vy, float omega, float gain) {
  // Chỉ bù khi có lệnh điều khiển đang thực thi VÀ vận tốc đủ lớn
  float totalTargetRPM = fabs(m1_target_rpm) + fabs(m2_target_rpm) + fabs(m3_target_rpm);
  if (totalTargetRPM < 30.0f) {
    return;  // Không bù khi robot đang dừng hoặc di chuyển rất chậm
  }
  
  // Chuyển vận tốc sang RPM với gain nhỏ
  float vx_rpm = velocityToRPM(vx) * gain;
  float vy_rpm = velocityToRPM(vy) * gain;
  float omega_rpm = velocityToRPM(omega * ROBOT_RADIUS_CM) * gain;
  
  // Tính compensation theo kinematics
  float comp_m1 = vx_rpm + omega_rpm;
  float comp_m2 = -0.5f * vx_rpm - 0.866f * vy_rpm + omega_rpm;
  float comp_m3 = -0.5f * vx_rpm + 0.866f * vy_rpm + omega_rpm;
  
  // Cộng compensation vào target hiện tại (không ghi đè)
  m1_target_rpm += comp_m1;
  m2_target_rpm += comp_m2;
  m3_target_rpm += comp_m3;
  
  // Giới hạn để tránh quá tải
  m1_target_rpm = constrain(m1_target_rpm, -200, 200);
  m2_target_rpm = constrain(m2_target_rpm, -200, 200);
  m3_target_rpm = constrain(m3_target_rpm, -200, 200);
}

//================= 5 hàm điều khiển cơ bản ======
// Cấu hình robot omni 3 bánh (tam giác đều):
// - M1: Phía trước (góc 90°)
// - M2: Phía sau-trái (góc 210°)
// - M3: Phía sau-phải (góc 330°)
//
// Công thức omni kinematics:
// M1 = Vx * sin(90°) + Vy * cos(90°) = Vx * 1 + Vy * 0 = Vx
// M2 = Vx * sin(210°) + Vy * cos(210°) = Vx * (-0.5) + Vy * (-0.866)
// M3 = Vx * sin(330°) + Vy * cos(330°) = Vx * (-0.5) + Vy * (0.866)

// TIẾN (Forward) - Vx dương, Vy = 0
void omniForward(float speed_cm_s) {
  float rpm = velocityToRPM(speed_cm_s);
  m1_target_rpm = rpm;             // M1 tiến (100%)
  m2_target_rpm = -0.5f * rpm;     // M2 lùi (sin 30° = 0.5)
  m3_target_rpm = -0.5f * rpm;     // M3 lùi (sin 30° = 0.5)
  Serial.printf("[OMNI] Forward %.1f cm/s -> M1=%.1f, M2=%.1f, M3=%.1f RPM\n", 
                speed_cm_s, m1_target_rpm, m2_target_rpm, m3_target_rpm);
}

// LÙI (Backward) - Vx âm, Vy = 0
void omniBackward(float speed_cm_s) {
  float rpm = velocityToRPM(speed_cm_s);
  m1_target_rpm = -rpm;            // M1 lùi (100%)
  m2_target_rpm = 0.5f * rpm;      // M2 tiến (sin 30° = 0.5)
  m3_target_rpm = 0.5f * rpm;      // M3 tiến (sin 30° = 0.5)
  Serial.printf("[OMNI] Backward %.1f cm/s -> M1=%.1f, M2=%.1f, M3=%.1f RPM\n", 
                speed_cm_s, m1_target_rpm, m2_target_rpm, m3_target_rpm);
}

// TRÁI (Strafe Left) - Vx = 0, Vy âm
void omniStrafeLeft(float speed_cm_s) {
  float rpm = velocityToRPM(speed_cm_s);
  m1_target_rpm = 0;              // M1 đứng yên
  m2_target_rpm = 0.866f * rpm;   // M2 quay (sqrt(3)/2 ≈ 0.866)
  m3_target_rpm = -0.866f * rpm;  // M3 quay ngược
  Serial.printf("[OMNI] Strafe Left %.1f cm/s -> M1=%.1f, M2=%.1f, M3=%.1f RPM\n", 
                speed_cm_s, m1_target_rpm, m2_target_rpm, m3_target_rpm);
}

// PHẢI (Strafe Right) - Vx = 0, Vy dương
void omniStrafeRight(float speed_cm_s) {
  float rpm = velocityToRPM(speed_cm_s);
  m1_target_rpm = 0;              // M1 đứng yên
  m2_target_rpm = -0.866f * rpm;  // M2 quay ngược
  m3_target_rpm = 0.866f * rpm;   // M3 quay (sqrt(3)/2 ≈ 0.866)
  Serial.printf("[OMNI] Strafe Right %.1f cm/s -> M1=%.1f, M2=%.1f, M3=%.1f RPM\n", 
                speed_cm_s, m1_target_rpm, m2_target_rpm, m3_target_rpm);
}

// XOAY TRÒN (Rotate)
void omniRotate(int dir, float rpm) {
  // dir=1: quay thuận chiều kim đồng hồ (clockwise)
  // dir=0: quay ngược chiều kim đồng hồ (counter-clockwise)
  // Tất cả động cơ quay cùng chiều để tạo mô-men xoay quanh tâm robot
  if (dir == 1) {
    digitalWrite(m1.dir, LOW);
    digitalWrite(m2.dir, LOW);
    digitalWrite(m3.dir, LOW);
    m1_target_rpm = rpm;
    m2_target_rpm = rpm;
    m3_target_rpm = rpm;
    // Serial.printf("[OMNI] Rotate CW %.1f RPM -> All motors = %.1f\n", rpm, rpm);
  } else {
    digitalWrite(m1.dir, HIGH);
    digitalWrite(m2.dir, HIGH);
    digitalWrite(m3.dir, HIGH);
    m1_target_rpm = rpm;
    m2_target_rpm = rpm;
    m3_target_rpm = rpm;
    // Serial.printf("[OMNI] Rotate CCW %.1f RPM -> All motors = %.1f\n", rpm, -rpm);
  }
}

// DỪNG (Stop)
void omniStop() {
  m1_target_rpm = 0;
  m2_target_rpm = 0;
  m3_target_rpm = 0;
  Serial.println("[OMNI] Stop - All motors RPM=0");
}
