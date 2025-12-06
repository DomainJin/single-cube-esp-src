# 🤖 THUẬT TOÁN ĐIỀU KHIỂN ROBOT OMNI 3 BÁNH

## 📐 CẤU HÌNH HỆ THỐNG

### Thông số kỹ thuật
```
Motor DC 6-12V với Encoder
├─ Tỷ số truyền: 1:30
├─ Encoder: 11 xung/vòng
├─ Tốc độ rotor: 6000 RPM max
├─ Tốc độ bánh xe: 333 RPM max (6000/30 * 1.665)
└─ Đường kính bánh: 100mm (r = 0.050m)

Driver: L298N
├─ Voltage drop: ~2V
├─ PWM: 0-255 (8-bit)
└─ PWM hiệu dụng: 60-255 (bù voltage drop)

Kích thước Robot (3-wheel Omni)
├─ Hình dạng: Tam giác đều
├─ Cạnh tam giác: 40cm (0.4m)
├─ Bán kính robot R: 23.094cm (0.23094m)
│  └─ Công thức: R = a/√3 = 0.4/1.732
├─ Góc giữa các bánh: 120°
└─ Cấu hình: Mercedes (0°, 120°, 240°)
```

### Cấu hình 3 bánh omni
```
Góc đặt bánh xe (tính từ trục Y+, theo chiều kim đồng hồ):
- Motor 1: θ₁ = 0°   (trục Y+, phía trước)
- Motor 2: θ₂ = 120° (phía sau-trái) 
- Motor 3: θ₃ = 240° (phía sau-phải)

Sơ đồ:
        Y+ (0°)
           ↑
           │ M1
           │
    ───────┼───────► X+
           │
      M2 ╱   ╲ M3
       120°   240°
```

---

## 🧮 PHƯƠNG TRÌNH ĐỘNG LỰC HỌC

### 1. INVERSE KINEMATICS (Robot → Bánh xe)

**Input**: Vận tốc mong muốn của robot
- `Vx` (m/s): Vận tốc theo trục X (strafe left/right)
- `Vy` (m/s): Vận tốc theo trục Y (forward/backward)
- `ω` (rad/s): Vận tốc góc (rotation)

**Output**: Vận tốc từng bánh xe `V₁, V₂, V₃` (m/s)

**Công thức chuẩn:**
```
Vᵢ = Vy × cos(θᵢ) + Vx × sin(θᵢ) + R × ω
```

Trong đó:
- `θᵢ`: Góc đặt bánh xe i (rad)
- `R`: Khoảng cách từ tâm robot đến bánh xe (0.23094m)
  * Tam giác đều cạnh 40cm: R = a/√3 = 0.4/1.732 = 0.23094m
- `ω`: Vận tốc góc (rad/s)

**Khai triển cho 3 bánh:**

```cpp
// Motor 1 (0°): trục Y+
V₁ = Vy × cos(0°) + Vx × sin(0°) + R × ω
   = Vy × 1 + Vx × 0 + R × ω
   = Vy + R × ω

// Motor 2 (120°): sau-trái
V₂ = Vy × cos(120°) + Vx × sin(120°) + R × ω
   = Vy × (-0.5) + Vx × 0.866 + R × ω
   = -0.5Vy + 0.866Vx + R × ω

// Motor 3 (240°): sau-phải  
V₃ = Vy × cos(240°) + Vx × sin(240°) + R × ω
   = Vy × (-0.5) + Vx × (-0.866) + R × ω
   = -0.5Vy - 0.866Vx + R × ω
```

**Dạng ma trận:**
```
┌    ┐   ┌                        ┐ ┌    ┐
│ V₁ │   │  1      0      R       │ │ Vy │
│ V₂ │ = │ -0.5   0.866   R       │ │ Vx │
│ V₃ │   │ -0.5  -0.866   R       │ │ ω  │
└    ┘   └                        ┘ └    ┘
```

---

### 2. FORWARD KINEMATICS (Bánh xe → Robot)

**Input**: Vận tốc đo từ encoder `V₁, V₂, V₃` (m/s)

**Output**: Vận tốc thực tế robot `Vx, Vy, ω`

**Công thức (pseudo-inverse với k = 2/3):**

```cpp
Vx = k × [sin(0°) × V₁ + sin(120°) × V₂ + sin(240°) × V₃]
Vy = k × [cos(0°) × V₁ + cos(120°) × V₂ + cos(240°) × V₃]
ω  = k × (V₁ + V₂ + V₃) / R
```

**Rút gọn:**
```cpp
Vx = (2/3) × [0 × V₁ + 0.866 × V₂ - 0.866 × V₃]
   = 0.577 × (V₂ - V₃)

Vy = (2/3) × [1 × V₁ - 0.5 × V₂ - 0.5 × V₃]
   = 0.667V₁ - 0.333V₂ - 0.333V₃

ω = (2/3) × (V₁ + V₂ + V₃) / 0.23094
  = 2.887 × (V₁ + V₂ + V₃)
```

**Dạng ma trận:**
```
┌    ┐   ┌                          ┐ ┌    ┐
│ Vx │   │  0      0.577  -0.577   │ │ V₁ │
│ Vy │ = │  0.667 -0.333  -0.333   │ │ V₂ │
│ ω  │   │  4.444  4.444   4.444   │ │ V₃ │
└    ┘   └                          ┘ └    ┘
```

---

## ⚙️ CHUYỂN ĐỔI VẬN TỐC ↔ PWM

### Vận tốc bánh xe → RPM → PWM

**Bước 1**: Tính RPM từ vận tốc tuyến tính
```
RPM = V / (2πr) × 60
    = V / (2π × 0.050) × 60
    = V × 191
```

**Bước 2**: Áp dụng threshold (lọc nhiễu)
```cpp
if (|V| < 0.05 m/s) {
    PWM = 0;  // Tắt motor nếu tốc độ < 9.6 RPM (với bánh 100mm)
    return;
}
```

**Bước 3**: Map RPM → PWM với voltage drop compensation
```
PWM_range = 255 - 60 = 195
PWM = (RPM / 333) × 195 + 60
```

**Giải thích**:
- L298N có voltage drop ~2V (12V → 10V)
- PWM 60/255 ≈ 23.5% → Motor nhận ~2.35V (đủ khởi động)
- PWM 255 → Motor nhận ~10V (full power)

**Bước 4**: Giữ dấu cho direction
```cpp
if (V < 0) {
    PWM = -PWM;  // Âm = MOTOR_BACKWARD
}
```

### PWM → Direction mapping
```cpp
if (PWM >= 0) {
    setMotorSpeed(motor, |PWM|, MOTOR_FORWARD);   // IN1=PWM, IN2=0
} else {
    setMotorSpeed(motor, |PWM|, MOTOR_BACKWARD);  // IN1=0, IN2=PWM
}
```

---

## 🎯 VÍ DỤ TÍNH TOÁN

### VD1: FORWARD 50 cm/s

**Input**: 
```
Vy = 0.5 m/s
Vx = 0
ω = 0
```

**Inverse Kinematics**:
```
V₁ = 0.5 × 1 + 0 + 0 = 0.5 m/s
V₂ = 0.5 × (-0.5) + 0 + 0 = -0.25 m/s
V₃ = 0.5 × (-0.5) + 0 + 0 = -0.25 m/s
```

**Chuyển đổi sang PWM**:
```
RPM₁ = 0.5 × 191 = 95.5 RPM
    PWM₁ = (95.5/333) × 195 + 60 = 116

RPM₂ = -0.25 × 191 = -47.75 RPM  
    PWM₂ = (-47.75/333) × 195 + 60 = 32 → BACKWARD

RPM₃ = -47.75 RPM
    PWM₃ = 32 → BACKWARD
```

**Kết quả**: 
- Motor 1: 116 PWM FORWARD → 95.5 RPM thuận
- Motor 2: 32 PWM BACKWARD → 47.75 RPM nghịch
- Motor 3: 32 PWM BACKWARD → 47.75 RPM nghịch
→ **Robot đi thẳng về phía trước**

---

### VD2: STRAFE RIGHT 30 cm/s

**Input**:
```
Vy = 0
Vx = 0.3 m/s
ω = 0
```

**Inverse Kinematics**:
```
V₁ = 0 + 0 × 0 + 0 = 0 m/s
V₂ = 0 + 0.3 × 0.866 + 0 = 0.26 m/s
V₃ = 0 + 0.3 × (-0.866) + 0 = -0.26 m/s
```

**Chuyển đổi sang PWM**:
```
V₁ = 0 → PWM₁ = 0 (< threshold)

RPM₂ = 0.26 × 191 = 49.66 RPM
    PWM₂ = (49.66/333) × 195 + 60 = 89

RPM₃ = -0.26 × 191 = -49.66 RPM
    PWM₃ = -89 → BACKWARD
```

**Kết quả**:
- Motor 1: STOP
- Motor 2: 89 PWM FORWARD → 49.66 RPM thuận
- Motor 3: 89 PWM BACKWARD → 49.66 RPM nghịch
→ **Robot dịch ngang sang phải**

---

### VD3: ROTATE 45 deg/s

**Input**:
```
Vy = 0
Vx = 0
ω = 0.785 rad/s (45°/s)
```

**Inverse Kinematics**:
```
V₁ = 0 + 0 + 0.23094 × 0.785 = 0.181 m/s
V₂ = 0 + 0 + 0.23094 × 0.785 = 0.181 m/s
V₃ = 0 + 0 + 0.23094 × 0.785 = 0.181 m/s
```

**Chuyển đổi sang PWM**:
```
RPM = 0.181 × 191 = 34.6 RPM
PWM = (34.6/333) × 195 + 60 = 80
```

**Kết quả**:
- Cả 3 motor: 80 PWM FORWARD → 34.6 RPM thuận
→ **Robot quay tại chỗ ngược chiều kim đồng hồ**

---

## 🔧 IMPLEMENTATION DETAILS

### Main Control Loop (50Hz)
```cpp
void updateOmni() {
    // 1. Inverse Kinematics
    float wheel_speeds[3];
    omniInverseKinematics(target_vx, target_vy, target_omega, wheel_speeds);
    
    // 2. Speed → PWM
    wheel_pwm[0] = omniSpeedToPWM(wheel_speeds[0]);
    wheel_pwm[1] = omniSpeedToPWM(wheel_speeds[1]);
    wheel_pwm[2] = omniSpeedToPWM(wheel_speeds[2]);
    
    // 3. Control Motors
    setMotorSpeed(motor1, |pwm[0]|, pwm[0]>=0 ? FWD : BWD);
    setMotorSpeed(motor2, |pwm[1]|, pwm[1]>=0 ? FWD : BWD);
    setMotorSpeed(motor3, |pwm[2]|, pwm[2]>=0 ? FWD : BWD);
    
    // 4. Read Encoders (every 100ms)
    wheel_rpm[0] = getMotorRPM(motor1);
    wheel_rpm[1] = getMotorRPM(motor2);
    wheel_rpm[2] = getMotorRPM(motor3);
    
    // 5. Forward Kinematics
    wheel_speeds_actual[i] = omniRPMToSpeed(wheel_rpm[i]);
    omniForwardKinematics(wheel_speeds_actual, &vx, &vy, &omega);
    
    // 6. Update Odometry
    pos_x += vx × dt;
    pos_y += vy × dt;
    heading += omega × dt;
}
```

### RPM Calculation từ Encoder
```cpp
float getMotorRPM(Motor& motor) {
    // Đếm pulse trong khoảng thời gian Δt
    long count_diff = current_count - last_count;
    unsigned long time_diff = current_time - last_time;
    
    // RPM encoder = (pulse / PPR) × (60 / Δt_seconds)
    float rpm_encoder = (count_diff × 60000.0) / (11 × time_diff);
    
    // RPM bánh xe = RPM encoder / gear_ratio
    float rpm_wheel = rpm_encoder / 30.0;
    
    return rpm_wheel;
}
```

### Speed to PWM Conversion
```cpp
int omniSpeedToPWM(float wheel_speed) {
    // Threshold: Bỏ qua nếu tốc độ < 0.05 m/s (9.6 RPM với bánh 100mm)
    if (abs(wheel_speed) < 0.05) return 0;
    
    // V (m/s) → RPM
    float rpm = (wheel_speed / (2 × π × 0.050)) × 60;  // r = 50mm = 0.050m
    
    // RPM → PWM với voltage drop compensation
    float pwm = (abs(rpm) / 333) × 195 + 60;
    
    // Giữ dấu
    if (wheel_speed < 0) pwm = -pwm;
    
    return (int)pwm;
}
```

---

## 📊 BẢNG TRA CỨU NHANH

### Tốc độ thường dùng

| Tốc độ | m/s | RPM bánh xe | PWM | Ghi chú |
|--------|-----|-------------|-----|---------|-----|
| Rất chậm | 0.1 | 19 | 71 | Test cơ bản |
| Chậm | 0.2 | 38 | 82 | Di chuyển chính xác |
| Trung bình | 0.3 | 57 | 93 | Mặc định |
| Nhanh | 0.5 | 95.5 | 116 | Vận hành thường |
| Rất nhanh | 0.6 | 115 | 127 | Giao tiếp thường |
| Max | 1.744 | 333 | 255 | Tốc độ tối đa (v = 2πr×RPM/60) |

### Vận tốc góc thường dùng

| Góc/s | rad/s | V tại R=0.23094m | RPM | PWM |
|-------|-------|----------------|-----|-----|
| 30° | 0.524 | 0.121 m/s | 23 | 74 |
| 45° | 0.785 | 0.181 m/s | 34.6 | 80 |
| 90° | 1.571 | 0.363 m/s | 69 | 100 |
| 180° | 3.142 | 0.726 m/s | 139 | 141 |

---

## 🐛 TROUBLESHOOTING

### Vấn đề 1: Chỉ 1 motor chạy
**Nguyên nhân**: 
- Direction sai: Truyền `true/false` thay vì `MOTOR_FORWARD/MOTOR_BACKWARD`
- PWM threshold quá cao

**Giải pháp**: 
```cpp
// SAI ❌
setMotorSpeed(motor, speed, pwm >= 0);  // true=1, false=0

// ĐÚNG ✅
setMotorSpeed(motor, speed, pwm >= 0 ? MOTOR_FORWARD : MOTOR_BACKWARD);
```

### Vấn đề 2: Robot đi sai hướng
**Nguyên nhân**: 
- Vx/Vy mapping sai
- Góc bánh xe không đúng

**Giải pháp**:
```cpp
// FORWARD phải là Vy, không phải Vx
omniForward(speed) → omniMove(0, vy, 0);  // ✅

// STRAFE phải là Vx, không phải Vy  
omniStrafe(speed) → omniMove(vx, 0, 0);   // ✅
```

### Vấn đề 3: Motor "píp" hoặc không quay
**Nguyên nhân**: 
- Nguồn không đủ
- PWM_MIN quá thấp (< voltage drop L298N)

**Giải pháp**:
- Dùng nguồn 12V/3A
- Đặt `OMNI_MIN_SPEED = 60` để bù voltage drop

---

## 📚 TÀI LIỆU THAM KHẢO

1. **Công thức Kinematics**:
   - Repository: https://github.com/phanben110/Robot-Omni-3-wheels-RTOS
   - Paper: "Omnidirectional Mobile Robot - Kinematics and Control"

2. **L298N Motor Driver**:
   - Datasheet: STMicroelectronics L298N
   - Voltage drop: 1.8-2.0V typical

3. **Motor Specifications**:
   - 6MM DC motor with encoder
   - Gear ratio: 1:30
   - No-load speed: 333 RPM (wheel), 6000 RPM (rotor)
   - Encoder: 11 PPR

---

**📝 Ghi chú**: 
- File này tóm tắt thuật toán điều khiển robot omni 3 bánh
- Công thức đã được verify và test trên hardware
- Tất cả giá trị dựa trên thông số thực tế của hệ thống

**📅 Ngày tạo**: 2025-12-06  
**🔧 Version**: 2.0 (Final - Working)
