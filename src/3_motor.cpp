#include "3_motor.h"
#include "udpconfig.h"

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

// Target RPM (user setpoint) và Ramp RPM (giá trị thực tế đưa vào PID)
float m1_target_rpm = 0.0f;
float m2_target_rpm = 0.0f;
float m3_target_rpm = 0.0f;

static float m1_ramp_rpm = 0.0f;  // soft-start/stop value fed to PID
static float m2_ramp_rpm = 0.0f;
static float m3_ramp_rpm = 0.0f;

// S-curve: per-motor acceleration state (jerk-limited profile)
static float m1_ramp_accel = 0.0f;
static float m2_ramp_accel = 0.0f;
static float m3_ramp_accel = 0.0f;

// S-curve parameters
// ACCEL_MAX: peak acceleration rate (RPM/s)
// JERK_UP:   how fast acceleration builds (RPM/s²) → soft start
// JERK_DN:   how fast acceleration falls  (RPM/s²) → soft stop
static const float SCURVE_ACCEL_MAX = 200.0f;
static const float SCURVE_JERK_UP   = 600.0f;
static const float SCURVE_JERK_DN   = 400.0f;

// Position control state
struct MotorPosCtrl {
  bool          active;
  long          target_count;
  bool          forward;
  unsigned long start_ms;
  unsigned long timeout_ms;
  float         speed_rpm;   // tốc độ tối đa của lần chạy này (cho decel zone)
};
static MotorPosCtrl pos1 = {false, 0, true, 0, 0, 0};
static MotorPosCtrl pos2 = {false, 0, true, 0, 0, 0};
static MotorPosCtrl pos3 = {false, 0, true, 0, 0, 0};

//================= Motor instances ===============
Motor m1 = {MOTOR1_EN, MOTOR1_DIR, ENC1_A, ENC1_B, M1_CH, 0, 0, 0, 0, 0, 0, 0};
Motor m2 = {MOTOR2_EN, MOTOR2_DIR, ENC2_A, ENC2_B, M2_CH, 0, 0, 0, 0, 0, 0, 0};
Motor m3 = {MOTOR3_EN, MOTOR3_DIR, ENC3_A, ENC3_B, M3_CH, 0, 0, 0, 0, 0, 0, 0};

unsigned long lastRPMt = 0;

//================= ISR — X4 quadrature (CHANGE trên cả A và B) ======
// Chiều tiến: B dẫn A (B leads A) — giống chiều quay ban đầu khi A RISING + B=HIGH.
//
// Quy tắc:
//   Kênh A thay đổi: if (a == b) → tiến (+1)  else → lùi (-1)
//   Kênh B thay đổi: if (a != b) → tiến (+1)  else → lùi (-1)
//
// (Kiểm chứng với dãy B dẫn A:  [0,0]→[0,1]→[1,1]→[1,0]→[0,0]
//   A rises ở [0,1]→[1,1]: a=1,b=1 → a==b → +1 ✓
//   B rises ở [0,0]→[0,1]: a=0,b=1 → a!=b → +1 ✓
//   A falls ở [1,1]→[1,0]: a=0,b=0 → a==b → hmm wait...
//   wait — falls state [1,1]→[1,0] means B falls, not A. A stays 1.
//   Let me redo: [0,0]→[0,1]: B rises, a=0,b=1, a!=b → +1 ✓
//                [0,1]→[1,1]: A rises, a=1,b=1, a==b → +1 ✓
//                [1,1]→[1,0]: B falls, a=1,b=0, a!=b → +1 ✓
//                [1,0]→[0,0]: A falls, a=0,b=0, a==b → +1 ✓   — tất cả đúng)

void IRAM_ATTR enc1_A_ISR() {
  bool a = digitalRead(m1.encA), b = digitalRead(m1.encB);
  if (a == b) m1.count++; else m1.count--;
}
void IRAM_ATTR enc1_B_ISR() {
  bool a = digitalRead(m1.encA), b = digitalRead(m1.encB);
  if (a != b) m1.count++; else m1.count--;
}

void IRAM_ATTR enc2_A_ISR() {
  bool a = digitalRead(m2.encA), b = digitalRead(m2.encB);
  if (a == b) m2.count++; else m2.count--;
}
void IRAM_ATTR enc2_B_ISR() {
  bool a = digitalRead(m2.encA), b = digitalRead(m2.encB);
  if (a != b) m2.count++; else m2.count--;
}

void IRAM_ATTR enc3_A_ISR() {
  bool a = digitalRead(m3.encA), b = digitalRead(m3.encB);
  if (a == b) m3.count++; else m3.count--;
}
void IRAM_ATTR enc3_B_ISR() {
  bool a = digitalRead(m3.encA), b = digitalRead(m3.encB);
  if (a != b) m3.count++; else m3.count--;
}

//================= Helper ========================
void setupMotor(Motor &m) {
  ledcSetup(m.pwmCh, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(m.en, m.pwmCh);
  pinMode(m.dir, OUTPUT);
  digitalWrite(m.dir, LOW); // Mặc định LOW (sẽ điều khiển theo target RPM)
  ledcWrite(m.pwmCh, 0);
}

void setupEncoder(Motor &m, void (*isrA)(), void (*isrB)()) {
  pinMode(m.encA, INPUT_PULLUP);
  pinMode(m.encB, INPUT_PULLUP);
  // X4 quadrature: bắt CHANGE trên cả 2 kênh để có độ phân giải tối đa
  attachInterrupt(digitalPinToInterrupt(m.encA), isrA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(m.encB), isrB, CHANGE);
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

  setupEncoder(m1, enc1_A_ISR, enc1_B_ISR);
  setupEncoder(m2, enc2_A_ISR, enc2_B_ISR);
  setupEncoder(m3, enc3_A_ISR, enc3_B_ISR);

  lastRPMt = millis();
  Serial.println("[3_motor] Init done");
}

//================= S-curve ramp =======================
// Jerk-limited velocity profile: ramp goes to target with smooth accel/decel.
// Produces true S-shape: slow start → peak speed → slow stop.
static void doSCurve(float &ramp, float &accel, float target, float dt) {
  float diff = target - ramp;
  if (fabs(diff) < 0.05f) { ramp = target; accel = 0.0f; return; }
  float sign = (diff > 0.0f) ? 1.0f : -1.0f;

  // Stopping distance based on CURRENT acceleration (not ACCEL_MAX).
  // When accel > 0: it'll be brought to 0 using JERK_DN (the decreasing branch).
  // When accel < 0: it'll be brought to 0 using JERK_UP (the increasing branch).
  // This avoids the "frozen ramp" deadlock that occurs when using a fixed decel_zone.
  float brake_jerk  = (accel >= 0.0f) ? SCURVE_JERK_DN : SCURVE_JERK_UP;
  float stopping_dist = (accel * accel) / (2.0f * brake_jerk);

  float desired_accel = (fabs(diff) > stopping_dist + 0.5f) ? sign * SCURVE_ACCEL_MAX : 0.0f;

  if (accel < desired_accel)
    accel = fminf(accel + SCURVE_JERK_UP * dt, desired_accel);
  else if (accel > desired_accel)
    accel = fmaxf(accel - SCURVE_JERK_DN * dt, desired_accel);

  ramp += accel * dt;
  if (sign > 0.0f && ramp > target) { ramp = target; accel = 0.0f; }
  if (sign < 0.0f && ramp < target) { ramp = target; accel = 0.0f; }
}

//================= Position control check ========
// Khi đạt target: dừng cứng ramp để tránh overshoot vị trí
static void checkPosCtrl(MotorPosCtrl &p, Motor &m, float &target_rpm, float &ramp_rpm, float &ramp_accel) {
  if (!p.active) return;
  long c;
  noInterrupts(); c = m.count; interrupts();
  bool done    = p.forward ? (c >= p.target_count) : (c <= p.target_count);
  bool timeout = (p.timeout_ms > 0) && (millis() - p.start_ms >= p.timeout_ms);
  if (done || timeout) {
    p.active    = false;
    target_rpm  = 0.0f;
    ramp_rpm    = 0.0f;
    ramp_accel  = 0.0f;
    if (timeout && !done)
      Serial.printf("[POS] TIMEOUT! count=%ld target=%ld - encoder khong dem duoc? Kiem tra day encoder!\n", c, p.target_count);
    else
      Serial.printf("[POS] Done! count=%ld target=%ld\n", c, p.target_count);
  } else {
    // Soft decel zone: giảm target_rpm tỉ lệ khi đến gần đích.
    // Kích thước zone = speed² / ACCEL_MAX × (CPR/60) × 2
    // → đủ thời gian để S-curve giảm tốc hoàn toàn trước khi chạm target.
    // Ví dụ 100 RPM, CPR=1008: zone = 100²/200 × 16.8 × 2 = 3360 counts = 3.3 vòng cơ học.
    long DECEL_ZONE = (long)(p.speed_rpm * p.speed_rpm / SCURVE_ACCEL_MAX
                             * ((float)ENCODER_CPR / 60.0f) * 2.0f);
    if (DECEL_ZONE < 400) DECEL_ZONE = 400;  // tối thiểu ~0.4 vòng

    long remaining = p.forward ? (p.target_count - c) : (c - p.target_count);
    if (remaining > 0 && remaining < DECEL_ZONE) {
      float frac    = (float)remaining / (float)DECEL_ZONE;
      float max_rpm = p.speed_rpm * frac + 6.0f;  // min 6 RPM: đủ torque, không bị stall
      if (p.forward  && target_rpm >  max_rpm) target_rpm =  max_rpm;
      if (!p.forward && target_rpm < -max_rpm) target_rpm = -max_rpm;
    }
  }
}

//================= Main update ===================
void update3Motors() {
  updateRPMAll();

  // ── S-Curve ramp: jerk-limited smooth velocity profile ───────────
  static unsigned long lastRampT = 0;
  unsigned long now = millis();
  float dt = (now - lastRampT) / 1000.0f;
  if (dt >= 0.005f) {
    doSCurve(m1_ramp_rpm, m1_ramp_accel, m1_target_rpm, dt);
    doSCurve(m2_ramp_rpm, m2_ramp_accel, m2_target_rpm, dt);
    doSCurve(m3_ramp_rpm, m3_ramp_accel, m3_target_rpm, dt);
    lastRampT = now;
  }

  // ── Position control (hard-stops ramp khi đến đích) ──────────────
  checkPosCtrl(pos1, m1, m1_target_rpm, m1_ramp_rpm, m1_ramp_accel);
  checkPosCtrl(pos2, m2, m2_target_rpm, m2_ramp_rpm, m2_ramp_accel);
  checkPosCtrl(pos3, m3, m3_target_rpm, m3_ramp_rpm, m3_ramp_accel);

  // ── PID dùng ramp_rpm (smooth) ───────────────────────────────────
  float pwm1 = stepPID(m1, m1_ramp_rpm);
  float pwm2 = stepPID(m2, m2_ramp_rpm);
  float pwm3 = stepPID(m3, m3_ramp_rpm);

  applyPWM(m1, (int)fabs(pwm1), pwm1 >= 0);
  applyPWM(m2, (int)fabs(pwm2), pwm2 >= 0);
  applyPWM(m3, (int)fabs(pwm3), pwm3 >= 0);

  static unsigned long lp = 0;
  if (millis() - lp > 500) {
    lp = millis();
    long c1, c2, c3;
    noInterrupts(); c1 = m1.count; c2 = m2.count; c3 = m3.count; interrupts();
    Serial.printf("M1 tgt=%.1f rmp=%.1f RPM=%.1f PWM=%d enc=%ld | M2 tgt=%.1f rmp=%.1f RPM=%.1f PWM=%d enc=%ld | M3 tgt=%.1f rmp=%.1f RPM=%.1f PWM=%d enc=%ld\n",
      m1_target_rpm, m1_ramp_rpm, m1.rpmFilt, (int)fabs(pwm1), c1,
      m2_target_rpm, m2_ramp_rpm, m2.rpmFilt, (int)fabs(pwm2), c2,
      m3_target_rpm, m3_ramp_rpm, m3.rpmFilt, (int)fabs(pwm3), c3);
    // Gửi encoder counts về server để hiển thị trên web
    char encMsg[64];
    snprintf(encMsg, sizeof(encMsg), "ENC:%ld,%ld,%ld", c1, c2, c3);
    sendUDPPacket(encMsg, UDP_PRIORITY_LOW);
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
  pos1.active = pos2.active = pos3.active = false;
  Serial.println("[OMNI] Stop - All motors RPM=0");
}

//================= Position control ==============
// Quay đúng N vòng rồi tự dừng. revs > 0 = tiến, < 0 = lùi.
void motorRunRevs(Motor &m, float revs, float rpm) {
  MotorPosCtrl *p   = (&m == &m1) ? &pos1 : (&m == &m2) ? &pos2 : &pos3;
  float        *trg = (&m == &m1) ? &m1_target_rpm : (&m == &m2) ? &m2_target_rpm : &m3_target_rpm;

  long current;
  noInterrupts(); current = m.count; interrupts();

  long delta    = (long)(revs * ENCODER_CPR);
  p->target_count = current + delta;
  p->forward      = (revs >= 0);
  p->active       = true;

  // Đặt RPM theo chiều
  *trg = p->forward ? fabs(rpm) : -fabs(rpm);

  p->speed_rpm = fabs(rpm);

  // Timeout = thời gian lý thuyết × 4 (dự phòng S-curve ramp + ma sát)
  float expected_s = fabs(revs) / fabs(rpm) * 60.0f;
  p->timeout_ms = (unsigned long)(expected_s * 4000.0f);
  if (p->timeout_ms < 3000)  p->timeout_ms = 3000;
  if (p->timeout_ms > 120000) p->timeout_ms = 120000;
  p->start_ms = millis();

  Serial.printf("[POS] Motor start: revs=%.2f rpm=%.1f current=%ld target=%ld timeout=%lus\n",
                revs, *trg, current, p->target_count, p->timeout_ms / 1000);
}

bool motorPosActive(Motor &m) {
  if (&m == &m1) return pos1.active;
  if (&m == &m2) return pos2.active;
  return pos3.active;
}

//================= Test mode =====================
struct MotorTestState {
  bool  active;
  int   phase;          // 1=forward, 2=pause, 3=backward, 4=done
  int   motorNum;       // 1/2/3
  float revs;
  float rpm;
  long  start_count;
  unsigned long phase_ms;
  bool  timed_out;      // true nếu position control timeout (encoder không đếm)
};
static MotorTestState ts = {false, 0, 0, 0, 0, 0, 0, false};

void motorStartTest(int motorNum, float revs, float rpm) {
  Motor *m = (motorNum == 1) ? &m1 : (motorNum == 2) ? &m2 : &m3;

  ts.active     = true;
  ts.phase      = 1;
  ts.motorNum   = motorNum;
  ts.revs       = revs;
  ts.rpm        = rpm;
  ts.timed_out  = false;
  noInterrupts(); ts.start_count = m->count; interrupts();

  Serial.printf("\n[TEST] ===== Motor %d Test =====\n", motorNum);
  Serial.printf("[TEST] Params: %.1f vòng @ %.1f RPM  (CPR=%d)\n", fabs(revs), rpm, ENCODER_CPR);
  Serial.printf("[TEST] Phase 1: %s %.1f vòng (S-curve)...\n", revs >= 0 ? "FORWARD" : "BACKWARD", fabs(revs));
  motorRunRevs(*m, revs, rpm);
}

void updateMotorTest() {
  if (!ts.active) return;
  Motor *m = (ts.motorNum == 1) ? &m1 : (ts.motorNum == 2) ? &m2 : &m3;

  switch (ts.phase) {
    case 1:
      if (!motorPosActive(*m)) {
        long c; noInterrupts(); c = m->count; interrupts();
        long expected = ts.start_count + (long)(ts.revs * ENCODER_CPR);
        // Nếu count gần như không thay đổi → encoder không đếm (timeout)
        if (labs(c - ts.start_count) < 5) ts.timed_out = true;
        Serial.printf("[TEST] Phase 1 DONE  count=%ld  (expected=%ld)%s\n",
                      c, expected, ts.timed_out ? "  *** ENCODER KHONG DEM ***" : "");
        Serial.println("[TEST] Phase 2: Dừng 500ms...");
        ts.phase    = 2;
        ts.phase_ms = millis();
      }
      break;

    case 2:
      if (millis() - ts.phase_ms >= 500) {
        ts.phase = 3;
        // Tính chính xác khoảng cách để về đúng vị trí ban đầu (start_count)
        // thay vì dùng -ts.revs (tránh sai lệch do inertia/overshoot ở phase 1)
        long c_now; noInterrupts(); c_now = m->count; interrupts();
        long delta_from_start = c_now - ts.start_count;
        float return_revs = -(float)delta_from_start / ENCODER_CPR;
        Serial.printf("[TEST] Phase 3: %s %.3f vòng (exact return: count %ld → %ld)...\n",
                      return_revs < 0 ? "BACKWARD" : "FORWARD",
                      fabs(return_revs), c_now, ts.start_count);
        motorRunRevs(*m, return_revs, ts.rpm);
      }
      break;

    case 3:
      if (!motorPosActive(*m)) {
        long final_count; noInterrupts(); final_count = m->count; interrupts();
        long drift   = final_count - ts.start_count;
        float drift_revs = (float)drift / ENCODER_CPR;

        Serial.println("[TEST] ===== KẾT QUẢ =====");
        Serial.printf("[TEST] Start count : %ld\n", ts.start_count);
        Serial.printf("[TEST] Final count : %ld\n", final_count);
        Serial.printf("[TEST] Drift       : %ld pulses  (%.4f vòng)\n", drift, drift_revs);
        if (ts.timed_out) {
          Serial.println("[TEST] FAIL - Encoder KHONG HOAT DONG!");
          Serial.println("[TEST] >>> Kiem tra: day noi encoder, nguon cap, chan GPIO");
          Serial.println("[TEST] >>> ENC1_A=GPIO21 ENC1_B=GPIO13 (Motor 1)");
        } else if (abs(drift) <= 5)
          Serial.println("[TEST] PASS - Encoder chinh xac!");
        else
          Serial.printf("[TEST] WARN - Sai lech %.2f%% (%.4f vong)\n",
                        fabs(drift_revs / ts.revs) * 100.0f, drift_revs);
        Serial.println("[TEST] ====================\n");

        ts.active = false;
        ts.phase  = 0;
      }
      break;
  }
}

//================= Record & Return mode ==========
// RUN: chạy tự do, ghi encoder liên tục
// STOP: soft-stop, tiếp tục ghi encoder cho đến khi dừng hẳn
// RETURN: quay ngược đúng số vòng đã ghi (tính đến khi dừng hẳn)

enum RecordPhase { REC_IDLE, REC_RUNNING, REC_STOPPING, REC_RETURNING };

struct RecordState {
  RecordPhase phase;
  int   motorNum;
  float rpm;
  long  start_count;
  long  stop_count;          // count khi motor dừng hẳn
  bool  return_after_stop;   // RETURN đã bấm trong lúc STOPPING
};
static RecordState rs = {REC_IDLE, 0, 0, 0, 0, false};

static Motor*          recMotor()  { return (rs.motorNum==1)?&m1:(rs.motorNum==2)?&m2:&m3; }
static float*          recTarget() { return (rs.motorNum==1)?&m1_target_rpm:(rs.motorNum==2)?&m2_target_rpm:&m3_target_rpm; }
static MotorPosCtrl*   recPos()    { return (rs.motorNum==1)?&pos1:(rs.motorNum==2)?&pos2:&pos3; }

// ── RUN ──────────────────────────────────────────────────────────────
void motorRecordStart(int motorNum, float rpm) {
  // Hủy bất kỳ mode nào đang chạy trên motor này
  MotorPosCtrl *p = (motorNum==1)?&pos1:(motorNum==2)?&pos2:&pos3;
  p->active = false;
  float *trg = (motorNum==1)?&m1_target_rpm:(motorNum==2)?&m2_target_rpm:&m3_target_rpm;

  Motor *m = (motorNum==1)?&m1:(motorNum==2)?&m2:&m3;
  noInterrupts(); rs.start_count = m->count; interrupts();

  rs.phase             = REC_RUNNING;
  rs.motorNum          = motorNum;
  rs.rpm               = rpm;
  rs.return_after_stop = false;
  *trg = rpm;  // ramp sẽ đưa dần lên rpm (soft start)

  Serial.printf("\n[REC] ===== Motor %d  RUN @ %.1f RPM =====\n", motorNum, rpm);
  Serial.printf("[REC] Start count = %ld\n", rs.start_count);
  Serial.println("[REC] Lệnh: MOTOR:n,STOP  |  MOTOR:n,RETURN");
}

// ── STOP (soft-stop, tiếp tục ghi encoder đến khi dừng hẳn) ─────────
void motorRecordStop(int motorNum) {
  if (rs.phase != REC_RUNNING || rs.motorNum != motorNum) {
    Serial.println("[REC] STOP: không có RUN đang chạy");
    return;
  }
  *recTarget() = 0.0f;  // ramp sẽ giảm dần về 0 (soft stop)
  rs.phase     = REC_STOPPING;
  Serial.println("[REC] STOP requested - soft stopping, vẫn ghi encoder...");
}

// ── RETURN ───────────────────────────────────────────────────────────
void motorRecordReturn(int motorNum) {
  if (rs.motorNum != motorNum && rs.phase != REC_IDLE) {
    Serial.println("[REC] RETURN: motor không khớp");
    return;
  }

  if (rs.phase == REC_RUNNING) {
    // Dừng ngay và return với count hiện tại
    motorRecordStop(motorNum);
    rs.return_after_stop = true;
    Serial.println("[REC] RETURN: sẽ quay ngược sau khi dừng hẳn...");

  } else if (rs.phase == REC_STOPPING) {
    // Đã đang dừng, đánh dấu return sau khi dừng xong
    rs.return_after_stop = true;
    Serial.println("[REC] RETURN: đã nhận, chờ motor dừng hẳn...");

  } else if (rs.phase == REC_IDLE && rs.stop_count != rs.start_count) {
    // Đã dừng hẳn, return ngay
    long  delta      = rs.stop_count - rs.start_count;
    float delta_revs = (float)delta / ENCODER_CPR;
    Serial.printf("\n[REC] RETURN: %.3f vòng @ %.1f RPM\n", fabs(delta_revs), rs.rpm);
    rs.phase = REC_RETURNING;
    motorRunRevs(*recMotor(), -delta_revs, rs.rpm);

  } else {
    Serial.println("[REC] RETURN: không có dữ liệu để return");
  }
}

// ── UPDATE (gọi từ loop) ─────────────────────────────────────────────
void updateMotorRecord() {
  if (rs.phase == REC_IDLE || rs.phase == REC_RUNNING) return;

  Motor *m = recMotor();

  if (rs.phase == REC_STOPPING) {
    // Ghi liên tục encoder cho đến khi dừng hẳn
    noInterrupts(); rs.stop_count = m->count; interrupts();

    bool stopped = (fabs(m->rpmFilt) < 2.0f);
    float *ramp  = (rs.motorNum==1)?&m1_ramp_rpm:(rs.motorNum==2)?&m2_ramp_rpm:&m3_ramp_rpm;
    stopped = stopped && (fabs(*ramp) < 2.0f);

    if (stopped) {
      long  delta      = rs.stop_count - rs.start_count;
      float delta_revs = (float)delta / ENCODER_CPR;
      Serial.printf("[REC] Dừng hẳn - count=%ld  delta=%ld pulses (%.3f vòng)\n",
                    rs.stop_count, delta, delta_revs);

      if (rs.return_after_stop) {
        rs.phase             = REC_RETURNING;
        rs.return_after_stop = false;
        Serial.printf("[REC] RETURN: quay ngược %.3f vòng @ %.1f RPM...\n",
                      fabs(delta_revs), rs.rpm);
        motorRunRevs(*m, -delta_revs, rs.rpm);
      } else {
        rs.phase = REC_IDLE;
        Serial.println("[REC] Đã dừng. Gửi MOTOR:n,RETURN để quay ngược.");
      }
    }
    return;
  }

  if (rs.phase == REC_RETURNING) {
    if (!motorPosActive(*m)) {
      long final_c;
      noInterrupts(); final_c = m->count; interrupts();
      long  drift      = final_c - rs.start_count;
      float drift_revs = (float)drift / ENCODER_CPR;

      Serial.println("\n[REC] ===== KẾT QUẢ =====");
      Serial.printf("[REC] Vị trí gốc  : %ld\n", rs.start_count);
      Serial.printf("[REC] Vị trí cuối : %ld\n", final_c);
      Serial.printf("[REC] Drift        : %ld pulses  (%.4f vòng)\n", drift, drift_revs);
      if (abs(drift) <= 5)
        Serial.println("[REC] ✓ PASS - Về đúng vị trí gốc!");
      else
        Serial.printf("[REC] ✗ WARN - Sai lệch %ld pulses (%.3f vòng)\n", drift, drift_revs);
      Serial.println("[REC] ====================\n");

      rs.phase = REC_IDLE;
    }
  }
}
