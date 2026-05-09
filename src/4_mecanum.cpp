#include "4_mecanum.h"
#include "udpconfig.h"

//================= Motor 4 instance ====================
// Giống hệt m1/m2/m3: đầy đủ encoder + PID
Motor m4 = {MOTOR4_EN, MOTOR4_DIR, ENC4_A, ENC4_B, M4_CH, 0, 0, 0, 0, 0, 0, 0};

//================= Target / ramp RPM motor 4 ===========
static float m4_target_rpm = 0.0f;
static float m4_ramp_rpm   = 0.0f;
static float m4_ramp_accel = 0.0f;

// Direction-reversal delay for M4 (mirrors RevDelay in 3_motor.cpp)
static struct { bool active; unsigned long apply_at_ms; float pending; } m4_rev = {false, 0, 0.0f};

//================= S-curve constants (giống 3_motor) ====
static const float M4_ACCEL_MAX = 200.0f;
static const float M4_JERK_UP   = 600.0f;
static const float M4_JERK_DN   = 400.0f;

//================= ISR — X4 quadrature (giống 3_motor) =
// Kênh A thay đổi: if (a == b) → tiến (+1)  else → lùi (-1)
// Kênh B thay đổi: if (a != b) → tiến (+1)  else → lùi (-1)
void IRAM_ATTR enc4_A_ISR() {
  bool a = digitalRead(m4.encA), b = digitalRead(m4.encB);
  if (a == b) m4.count++; else m4.count--;
}
void IRAM_ATTR enc4_B_ISR() {
  bool a = digitalRead(m4.encA), b = digitalRead(m4.encB);
  if (a != b) m4.count++; else m4.count--;
}

//================= S-curve ramp (bản cục bộ cho m4) ====
// doSCurve trong 3_motor.cpp là static nên không thể gọi từ đây.
static void doSCurve4(float &ramp, float &accel, float target, float dt) {
  float diff = target - ramp;
  if (fabsf(diff) < 0.05f) { ramp = target; accel = 0.0f; return; }
  float sign = (diff > 0.0f) ? 1.0f : -1.0f;

  float brake_jerk   = (accel >= 0.0f) ? M4_JERK_DN : M4_JERK_UP;
  float stopping_dist = (accel * accel) / (2.0f * brake_jerk);
  float desired_accel = (fabsf(diff) > stopping_dist + 0.5f) ? sign * M4_ACCEL_MAX : 0.0f;

  if (accel < desired_accel)
    accel = fminf(accel + M4_JERK_UP * dt, desired_accel);
  else if (accel > desired_accel)
    accel = fmaxf(accel - M4_JERK_DN * dt, desired_accel);

  ramp += accel * dt;
  if (sign > 0.0f && ramp > target) { ramp = target; accel = 0.0f; }
  if (sign < 0.0f && ramp < target) { ramp = target; accel = 0.0f; }
}

//================= RPM update cục bộ cho m4 ============
// updateRPMAll() trong 3_motor.cpp là static cho m1/m2/m3.
// m4 tự duy trì cửa sổ riêng với cùng thông số RPM_WINDOW_MS / RPM_ALPHA / ENCODER_CPR.
static unsigned long lastRPMt4 = 0;

static void updateRPM4() {
  unsigned long now = millis();
  if (now - lastRPMt4 < RPM_WINDOW_MS) return;
  float dt = (now - lastRPMt4) / 1000.0f;

  long c;
  noInterrupts(); c = m4.count; interrupts();
  long d = c - m4.lastCount;
  float rpmRaw = (d / (float)ENCODER_CPR) * (60.0f / dt);
  m4.rpmRaw  = rpmRaw;
  m4.rpmFilt = RPM_ALPHA * rpmRaw + (1.0f - RPM_ALPHA) * m4.rpmFilt;
  m4.lastCount = c;

  lastRPMt4 = now;
}

//================= Position control for M4 ============
struct M4PosCtrl {
  bool          active;
  long          target_count;
  bool          forward;
  unsigned long start_ms;
  unsigned long timeout_ms;
  float         speed_rpm;
  // Cascade outer PID state
  float         pos_integral;
  float         pos_last_err;
  unsigned long pos_last_t;
  float         vel_cmd;
};
static M4PosCtrl m4pos = {false, 0, true, 0, 0, 0.0f, 0.0f, 0.0f, 0, 0.0f};

static void checkPosCtrl4() {
  if (!m4pos.active) return;
  long c;
  noInterrupts(); c = m4.count; interrupts();
  bool done    = labs(c - m4pos.target_count) <= POS_TOLERANCE
               || (m4pos.forward ? (c >= m4pos.target_count) : (c <= m4pos.target_count));
  bool timeout = (m4pos.timeout_ms > 0) && (millis() - m4pos.start_ms >= m4pos.timeout_ms);
  if (done || timeout) {
    m4pos.active  = false;
    m4pos.vel_cmd = 0.0f;
    m4_target_rpm = 0.0f;
    // S-curve handoff: bắt đầu từ tốc độ thực tế, giảm mượt về 0
    m4_ramp_rpm   = m4.rpmFilt;
    m4_ramp_accel = 0.0f;
    // Reset inner PID để không có integral kick khi S-curve tiếp quản
    m4.integral   = 0.0f;
    m4.lastErr    = 0.0f;
    if (timeout && !done)
      Serial.printf("[POS-M4] TIMEOUT! count=%ld target=%ld - kiem tra day encoder!\n", c, m4pos.target_count);
    else
      Serial.printf("[POS-M4] Done! count=%ld target=%ld (err=%ld)\n", c, m4pos.target_count, c - m4pos.target_count);
  } else {
    // Outer position PID → outputs velocity command (RPM)
    unsigned long now = millis();
    float dt = (now - m4pos.pos_last_t) / 1000.0f;
    if (dt <= 0.0f) dt = 1e-3f;

    float err = (float)(m4pos.target_count - c);

    // Deadband: zero vel_cmd within tolerance to prevent oscillation
    if (fabsf(err) <= (float)POS_TOLERANCE) {
      m4pos.vel_cmd    = 0.0f;
      m4pos.pos_last_t = now;
      return;
    }

    // Anti-windup: reset integral when error changes sign
    if ((err > 0.0f && m4pos.pos_last_err < 0.0f) || (err < 0.0f && m4pos.pos_last_err > 0.0f))
      m4pos.pos_integral = 0.0f;

    m4pos.pos_integral += err * dt;
    if (m4pos.pos_integral >  3000.0f) m4pos.pos_integral =  3000.0f;
    if (m4pos.pos_integral < -3000.0f) m4pos.pos_integral = -3000.0f;

    float deriv = (err - m4pos.pos_last_err) / dt;
    float vel   = Kp_pos * err + Ki_pos * m4pos.pos_integral + Kd_pos * deriv;

    if (vel >  m4pos.speed_rpm) vel =  m4pos.speed_rpm;
    if (vel < -m4pos.speed_rpm) vel = -m4pos.speed_rpm;

    // Snap to zero if vel_cmd too small — hand off to S-curve for smooth decel
    if (fabsf(vel) < MIN_VEL_CMD) {
      if (fabsf(m4pos.vel_cmd) >= MIN_VEL_CMD) {
        // First snap this run: seed S-curve from actual speed so it decels smoothly
        m4_ramp_rpm   = m4.rpmFilt;
        m4_ramp_accel = 0.0f;
      }
      vel = 0.0f;
    }

    m4pos.vel_cmd      = vel;
    m4pos.pos_last_err = err;
    m4pos.pos_last_t   = now;
  }
}

void motorRunRevs4(float revs, float rpm) {
  long current;
  noInterrupts(); current = m4.count; interrupts();
  long delta         = (long)(revs * ENCODER_CPR);
  m4pos.target_count = current + delta;
  m4pos.forward      = (revs >= 0);
  m4pos.active       = true;
  m4pos.speed_rpm    = fabsf(rpm);
  m4_target_rpm      = m4pos.forward ? fabsf(rpm) : -fabsf(rpm);

  float expected_s   = fabsf(revs) / fabsf(rpm) * 60.0f;
  m4pos.timeout_ms   = (unsigned long)(expected_s * 4000.0f);
  if (m4pos.timeout_ms < 3000)   m4pos.timeout_ms = 3000;
  if (m4pos.timeout_ms > 120000) m4pos.timeout_ms = 120000;
  m4pos.start_ms = millis();

  // Init outer PID state (set last_err = initial error to avoid derivative spike)
  m4pos.pos_integral   = 0.0f;
  m4pos.pos_last_err   = (float)(m4pos.target_count - current);
  m4pos.pos_last_t     = millis();
  m4pos.vel_cmd        = 0.0f;

  Serial.printf("[POS-M4] start: revs=%.2f rpm=%.1f current=%ld target=%ld timeout=%lus\n",
                revs, m4_target_rpm, current, m4pos.target_count, m4pos.timeout_ms / 1000);
}

bool motorPosActive4() { return m4pos.active; }

void mecanumSetM4TargetRPM(float rpm) {
  m4pos.active = false;
  bool dir_change = (rpm > 5.0f && m4.rpmFilt < -5.0f)
                 || (rpm < -5.0f && m4.rpmFilt >  5.0f);
  if (dir_change && !m4_rev.active) {
    m4_target_rpm      = 0.0f;
    m4_rev.active      = true;
    m4_rev.apply_at_ms = millis() + DIR_CHANGE_DELAY_MS;
    m4_rev.pending     = rpm;
    Serial.printf("[DIR-M4] Doi chieu %.1f→%.1f RPM, cho %lums\n", m4_ramp_rpm, rpm, DIR_CHANGE_DELAY_MS);
  } else if (m4_rev.active) {
    m4_rev.pending = rpm;
  } else {
    m4_target_rpm = rpm;
  }
}

//================= Test mode for M4 ===================
struct M4TestState {
  bool          active;
  int           phase;        // 1=forward, 2=pause, 3=backward, 4=done
  float         revs;
  float         rpm;
  long          start_count;
  unsigned long phase_ms;
  bool          timed_out;
};
static M4TestState ts4 = {false, 0, 0.0f, 0.0f, 0, 0, false};

void motorStartTest4(float revs, float rpm) {
  ts4.active     = true;
  ts4.phase      = 1;
  ts4.revs       = revs;
  ts4.rpm        = rpm;
  ts4.timed_out  = false;
  noInterrupts(); ts4.start_count = m4.count; interrupts();

  Serial.printf("\n[TEST-M4] ===== Motor 4 Test =====\n");
  Serial.printf("[TEST-M4] Params: %.1f vong @ %.1f RPM  (CPR=%d)\n", fabsf(revs), rpm, ENCODER_CPR);
  Serial.printf("[TEST-M4] Phase 1: %s %.1f vong (S-curve)...\n",
                revs >= 0 ? "FORWARD" : "BACKWARD", fabsf(revs));
  motorRunRevs4(revs, rpm);
}

static void updateMotorTest4() {
  if (!ts4.active) return;

  switch (ts4.phase) {
    case 1:
      if (!motorPosActive4()) {
        long c; noInterrupts(); c = m4.count; interrupts();
        long expected = ts4.start_count + (long)(ts4.revs * ENCODER_CPR);
        if (labs(c - ts4.start_count) < 5) ts4.timed_out = true;
        Serial.printf("[TEST-M4] Phase 1 DONE  count=%ld  (expected=%ld)%s\n",
                      c, expected, ts4.timed_out ? "  *** ENCODER KHONG DEM ***" : "");
        Serial.println("[TEST-M4] Phase 2: Dung 500ms...");
        ts4.phase    = 2;
        ts4.phase_ms = millis();
      }
      break;

    case 2:
      if (millis() - ts4.phase_ms >= 500) {
        ts4.phase = 3;
        long c_now; noInterrupts(); c_now = m4.count; interrupts();
        long  delta_from_start = c_now - ts4.start_count;
        float return_revs      = -(float)delta_from_start / ENCODER_CPR;
        Serial.printf("[TEST-M4] Phase 3: %s %.3f vong (exact return: count %ld -> %ld)...\n",
                      return_revs < 0 ? "BACKWARD" : "FORWARD",
                      fabsf(return_revs), c_now, ts4.start_count);
        motorRunRevs4(return_revs, ts4.rpm);
      }
      break;

    case 3:
      if (!motorPosActive4()) {
        long final_c; noInterrupts(); final_c = m4.count; interrupts();
        long  drift      = final_c - ts4.start_count;
        float drift_revs = (float)drift / ENCODER_CPR;

        Serial.println("[TEST-M4] ===== KET QUA =====");
        Serial.printf("[TEST-M4] Start count : %ld\n", ts4.start_count);
        Serial.printf("[TEST-M4] Final count : %ld\n", final_c);
        Serial.printf("[TEST-M4] Drift       : %ld pulses  (%.4f vong)\n", drift, drift_revs);
        if (ts4.timed_out) {
          Serial.println("[TEST-M4] FAIL - Encoder KHONG HOAT DONG!");
          Serial.println("[TEST-M4] >>> Kiem tra: ENC4_A=GPIO32  ENC4_B=GPIO33");
        } else if (abs(drift) <= 5)
          Serial.println("[TEST-M4] PASS - Encoder chinh xac!");
        else
          Serial.printf("[TEST-M4] WARN - Sai lech %.2f%% (%.4f vong)\n",
                        fabsf(drift_revs / ts4.revs) * 100.0f, drift_revs);
        Serial.println("[TEST-M4] ====================\n");

        ts4.active = false;
        ts4.phase  = 0;
      }
      break;
  }
}

//================= Initialization ======================
void init4Mecanum() {
  // Motor 4
  setupMotor(m4);
  setupEncoder(m4, enc4_A_ISR, enc4_B_ISR);

  lastRPMt4 = millis();
  Serial.println("[4_mecanum] Init done");
}

//================= Main update =========================
void update4Mecanum() {
  updateRPM4();

  // S-curve ramp
  static unsigned long lastRampT4 = 0;
  unsigned long now = millis();
  float dt = (now - lastRampT4) / 1000.0f;
  if (dt >= 0.005f) {
    doSCurve4(m4_ramp_rpm, m4_ramp_accel, m4_target_rpm, dt);
    lastRampT4 = now;
  }

  // Position control (decel zone + hard stop khi đến đích)
  checkPosCtrl4();

  // Test mode update
  updateMotorTest4();

  // Position cascade XOR S-curve: vel_cmd != 0 → outer PID drives, else → S-curve smooth decel
  float vc4  = (m4pos.active && fabsf(m4pos.vel_cmd) > 0.0f) ? m4pos.vel_cmd : m4_ramp_rpm;
  float pwm4 = stepPID(m4, vc4);
  applyPWM(m4, (int)fabsf(pwm4), pwm4 >= 0.0f);

  // Debug log mỗi 500ms
  static unsigned long lp4 = 0;
  if (millis() - lp4 > 500) {
    lp4 = millis();
    long c4; noInterrupts(); c4 = m4.count; interrupts();

    // Log tốc độ (RPM thực tế) và vị trí (encoder count)
    Serial.printf("[RPM ] M4=%6.1f  (RPM)\n", m4.rpmFilt);
    Serial.printf("[POS ] M4=%7ld  (counts)\n", c4);

    char encMsg[32];
    snprintf(encMsg, sizeof(encMsg), "ENC4:%ld", c4);
    sendUDPPacket(encMsg, UDP_PRIORITY_LOW);
  }
}

//================= Helper: set cả 4 motor ===============
// Mapping vật lý: M1=FL, M2=FR, M3=RL, M4=RR
// Chỉnh lại nếu dây nối thực tế khác.
static void setAll4(float fl, float fr, float rl, float rr) {
  setMotorTargetRPM(m1, fl);
  setMotorTargetRPM(m2, fr);
  setMotorTargetRPM(m3, rl);
  mecanumSetM4TargetRPM(rr);
}

//================= Mecanum Kinematics ==================
// Bánh mecanum rollers 45°, frame robot: x=tiến, y=trái (CCW dương).
//
// FL = vx − vy − (HALF_L+HALF_W)·ω
// FR = vx + vy + (HALF_L+HALF_W)·ω
// RL = vx + vy − (HALF_L+HALF_W)·ω
// RR = vx − vy + (HALF_L+HALF_W)·ω
void mecanumSetVelocity(float vx, float vy, float omega) {
  float vx_rpm    = velocityToRPM(vx);
  float vy_rpm    = velocityToRPM(vy);
  float omega_rpm = velocityToRPM(omega * (MECANUM_HALF_L + MECANUM_HALF_W));

  float fl = vx_rpm - vy_rpm - omega_rpm;
  float fr = vx_rpm + vy_rpm + omega_rpm;
  float rl = vx_rpm + vy_rpm - omega_rpm;
  float rr = vx_rpm - vy_rpm + omega_rpm;

  setAll4(fl, fr, rl, rr);
  // Serial.printf("[MECANUM] Vx=%.1f Vy=%.1f ω=%.2f -> FL=%.1f FR=%.1f RL=%.1f RR=%.1f RPM\n",
  //               vx, vy, omega, fl, fr, rl, rr);
}

//================= 5 hàm điều khiển cơ bản ============

// TIẾN — tất cả bánh quay cùng chiều
void mecanumForward(float speed_cm_s) {
  float rpm = velocityToRPM(speed_cm_s);
  setAll4(rpm, rpm, rpm, rpm);
  // Serial.printf("[MECANUM] Forward %.1f cm/s -> all = %.1f RPM\n", speed_cm_s, rpm);
}

// LÙI — tất cả bánh quay ngược chiều
void mecanumBackward(float speed_cm_s) {
  float rpm = velocityToRPM(speed_cm_s);
  setAll4(-rpm, -rpm, -rpm, -rpm);
  // Serial.printf("[MECANUM] Backward %.1f cm/s -> all = -%.1f RPM\n", speed_cm_s, rpm);
}

// TRÁI (strafe) — FL=−, FR=+, RL=+, RR=−
void mecanumStrafeLeft(float speed_cm_s) {
  float rpm = velocityToRPM(speed_cm_s);
  setAll4(-rpm, rpm, rpm, -rpm);
  // Serial.printf("[MECANUM] Strafe Left %.1f cm/s -> FL=-%.1f FR=%.1f RL=%.1f RR=-%.1f\n",
  //               speed_cm_s, rpm, rpm, rpm, rpm);
}

// PHẢI (strafe) — FL=+, FR=−, RL=−, RR=+
void mecanumStrafeRight(float speed_cm_s) {
  float rpm = velocityToRPM(speed_cm_s);
  setAll4(rpm, -rpm, -rpm, rpm);
  // Serial.printf("[MECANUM] Strafe Right %.1f cm/s -> FL=%.1f FR=-%.1f RL=-%.1f RR=%.1f\n",
  //               speed_cm_s, rpm, rpm, rpm, rpm);
}

// XOAY TRÒN — CW: FL=+,FR=−,RL=+,RR=−  |  CCW: ngược lại
void mecanumRotate(int dir, float rpm) {
  if (dir == 1) {
    setAll4(rpm, -rpm, rpm, -rpm);
  } else {
    setAll4(-rpm, rpm, -rpm, rpm);
  }
  // Serial.printf("[MECANUM] Rotate %s %.1f RPM\n", dir == 1 ? "CW" : "CCW", rpm);
}

// DỪNG
void mecanumStop() {
  setAll4(0.0f, 0.0f, 0.0f, 0.0f);
  m4pos.active  = false;
  m4pos.vel_cmd = 0.0f;
  m4_target_rpm = 0.0f;
  m4_ramp_rpm   = 0.0f;
  m4_ramp_accel = 0.0f;
  // Serial.println("[MECANUM] Stop - All motors RPM=0");
}
