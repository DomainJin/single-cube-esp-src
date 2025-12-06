# 🤖 ĐỘNG LỰC HỌC ROBOT OMNI 3 BÁNH

## 📐 CẤU HÌNH ROBOT

### Thông số vật lý
```
- Bánh xe: 3 bánh omni wheel
- Đường kính bánh: 37mm (bán kính r = 0.0185m)
- Khoảng cách tâm robot đến bánh: R = 0.15m
- Tỷ số truyền: 1:30 (motor → bánh xe)
- Encoder: 11 xung/vòng
- Motor max: 6000 RPM (rotor) → 200 RPM (bánh xe)
```

### Góc đặt bánh xe
```
Motor 1: θ₁ = 0° (bên phải robot)
Motor 2: θ₂ = 120° (sau-trái robot) 
Motor 3: θ₃ = 240° (sau-phải robot)
```

```
        Front (0°)
           ↑
           |
    120° ╱   ╲ 240°
       ╱  💠  ╲
     M2       M3
```

---

## 🧮 PHƯƠNG TRÌNH ĐỘNG LỰC HỌC

### 1. INVERSE KINEMATICS (Vận tốc robot → Vận tốc bánh xe)

**Input**: Vận tốc mong muốn của robot
- `vₓ` (m/s): Vận tốc theo trục X (forward/backward)
- `vᵧ` (m/s): Vận tốc theo trục Y (left/right strafe)
- `ω` (rad/s): Vận tốc góc (rotation)

**Output**: Vận tốc của từng bánh xe `v₁, v₂, v₃` (m/s)

**Công thức**:
```
vᵢ = sin(θᵢ) × vₓ - cos(θᵢ) × vᵧ + R × ω
```

**Giải thích**:
- `sin(θᵢ) × vₓ`: Thành phần chuyển động thẳng theo phương X
- `cos(θᵢ) × vᵧ`: Thành phần chuyển động ngang theo phương Y
- `R × ω`: Thành phần quay tại chỗ

**Áp dụng cho 3 bánh**:
```cpp
v₁ = sin(0°) × vₓ - cos(0°) × vᵧ + R × ω
   = 0 × vₓ - 1 × vᵧ + R × ω
   = -vᵧ + R × ω

v₂ = sin(120°) × vₓ - cos(120°) × vᵧ + R × ω
   = 0.866 × vₓ - (-0.5) × vᵧ + R × ω
   = 0.866vₓ + 0.5vᵧ + R × ω

v₃ = sin(240°) × vₓ - cos(240°) × vᵧ + R × ω
   = -0.866 × vₓ - (-0.5) × vᵧ + R × ω
   = -0.866vₓ + 0.5vᵧ + R × ω
```

---

### 2. FORWARD KINEMATICS (Vận tốc bánh xe → Vận tốc robot)

**Input**: Vận tốc đo được từ encoder của từng bánh `v₁, v₂, v₃` (m/s)

**Output**: Vận tốc thực tế của robot `vₓ, vᵧ, ω`

**Công thức** (pseudo-inverse với k = 2/3):
```
vₓ = (2/3) × [sin(0°) × v₁ + sin(120°) × v₂ + sin(240°) × v₃]
vᵧ = (2/3) × [-cos(0°) × v₁ - cos(120°) × v₂ - cos(240°) × v₃]
ω = (2/3) × (v₁ + v₂ + v₃) / R
```

**Rút gọn**:
```cpp
vₓ = (2/3) × [0.866v₂ - 0.866v₃]
   = 0.577 × (v₂ - v₃)

vᵧ = (2/3) × [-v₁ + 0.5v₂ + 0.5v₃]
   = -0.667v₁ + 0.333v₂ + 0.333v₃

ω = (2/3) × (v₁ + v₂ + v₃) / R
```

---

## 🎯 VÍ DỤ TÍNH TOÁN

### VD1: FORWARD (Di chuyển thẳng)
**Input**: vₓ = 0.5 m/s, vᵧ = 0, ω = 0

**Inverse Kinematics**:
```
v₁ = 0 × 0.5 - 1 × 0 + 0 = 0 m/s        → Motor 1: STOP
v₂ = 0.866 × 0.5 + 0.5 × 0 + 0 = 0.433 m/s  → Motor 2: CW (224 RPM)
v₃ = -0.866 × 0.5 + 0.5 × 0 + 0 = -0.433 m/s → Motor 3: CCW (224 RPM)
```

**Kết quả**: Motor 2 và 3 quay ngược chiều nhau → Robot đi thẳng

---

### VD2: STRAFE LEFT (Dịch ngang trái)
**Input**: vₓ = 0, vᵧ = -0.5 m/s, ω = 0

**Inverse Kinematics**:
```
v₁ = 0 - 1 × (-0.5) + 0 = 0.5 m/s     → Motor 1: CW (258 RPM)
v₂ = 0 + 0.5 × (-0.5) + 0 = -0.25 m/s → Motor 2: CCW (129 RPM)
v₃ = 0 + 0.5 × (-0.5) + 0 = -0.25 m/s → Motor 3: CCW (129 RPM)
```

**Kết quả**: Motor 1 quay nhanh, motor 2+3 quay ngược → Robot dịch trái

---

### VD3: ROTATE (Quay tại chỗ)
**Input**: vₓ = 0, vᵧ = 0, ω = 1.0 rad/s

**Inverse Kinematics**:
```
v₁ = 0 - 0 + 0.15 × 1.0 = 0.15 m/s  → Motor 1: CW (77 RPM)
v₂ = 0 + 0 + 0.15 × 1.0 = 0.15 m/s  → Motor 2: CW (77 RPM)
v₃ = 0 + 0 + 0.15 × 1.0 = 0.15 m/s  → Motor 3: CW (77 RPM)
```

**Kết quả**: Cả 3 motor quay cùng chiều → Robot quay tại chỗ

---

## ⚙️ CHUYỂN ĐỔI VẬN TỐC ↔ PWM

### Vận tốc → RPM → PWM

**Bước 1**: Vận tốc tuyến tính (m/s) → RPM bánh xe
```
RPM = v / (2πr) × 60
    = v / (2π × 0.0185) × 60
    = v × 516.4
```

**Bước 2**: RPM → PWM (mapping với bù voltage drop L298N)
```
PWM_range = 255 - 60 = 195
PWM = (RPM / 333) × 195 + 60
```

**Ví dụ**: v = 0.433 m/s
```
RPM = 0.433 × 516.4 = 224 RPM
PWM = (224 / 333) × 195 + 60 = 191
```

### PWM → RPM → Vận tốc

**Bước 1**: PWM → RPM
```
RPM = (PWM - 60) / 195 × 333
```

**Bước 2**: RPM → Vận tốc (m/s)
```
v = RPM × (2πr) / 60
  = RPM × (2π × 0.0185) / 60
  = RPM × 0.001936
```

---

## 🔧 ĐIỀU CHỈNH GÓC BÁNH XE

Nếu robot không di chuyển đúng hướng, cần điều chỉnh góc `θ₁, θ₂, θ₃`.

### Phương pháp test:
1. **Test FORWARD**: Quan sát motor nào quay
2. **Test LEFT**: Quan sát motor nào quay
3. **Test ROTATE**: Cả 3 motor phải quay cùng chiều

### Bảng góc phổ biến:

| Cấu hình | Motor 1 | Motor 2 | Motor 3 | Đặc điểm |
|----------|---------|---------|---------|----------|
| **Mercedes** | 0° | 120° | 240° | Đối xứng 120° |
| **Y-shape** | 90° | 210° | 330° | 1 trước, 2 sau |
| **Triangle** | 30° | 150° | 270° | Tam giác đều lệch |

### Cách xác định góc đúng:
1. Đặt robot lên bàn
2. Nhìn từ trên xuống
3. Chọn 1 motor làm mốc (ví dụ: motor ở bên phải = 0°)
4. Đo góc theo chiều kim đồng hồ

```
          0° (North)
            ↑
            |
270° ← ─ ─ ⊕ ─ ─ → 90°
            |
            ↓
          180°
```

---

## 🐛 TROUBLESHOOTING

### Vấn đề 1: Chỉ có 1 motor chạy
**Nguyên nhân**: 
- Góc bánh xe sai
- PWM_MIN quá cao (các motor khác bị filter)

**Giải pháp**:
1. Giảm `OMNI_MIN_SPEED` từ 80 xuống 60
2. Thử các cấu hình góc khác nhau
3. Test từng hướng và ghi lại motor nào chạy

### Vấn đề 2: Robot đi sai hướng
**Nguyên nhân**: 
- Góc bánh xe không khớp với thực tế
- Motor nối ngược dây

**Giải pháp**:
1. Đổi chiều motor bằng cách đảo IN1 ↔ IN2
2. Điều chỉnh góc theo bảng trên
3. Test và tinh chỉnh

### Vấn đề 3: Motor "píp" hoặc không quay
**Nguyên nhân**: 
- Nguồn không đủ (< 2A cho 3 motor)
- PWM quá thấp (< voltage drop L298N)

**Giải pháp**:
1. Dùng nguồn 12V/3A
2. Tăng `OMNI_MIN_SPEED` lên 80-100
3. Thêm capacitor 1000µF gần motor

---

## 📊 DEBUG LOG ANALYSIS

### Đọc Serial Monitor:
```
[OMNI] Target: vx=0.500 vy=0.000 omega=0.000
[OMNI] Wheel speeds (m/s): [0.000, 0.433, -0.433]
[OMNI] PWM: [80, 191, -191]
```

**Phân tích**:
- Motor 1: PWM = 80 (min threshold) → Có thể không quay
- Motor 2: PWM = 191 (mạnh) → Quay CW
- Motor 3: PWM = -191 (mạnh) → Quay CCW

**Kết luận**: Cần giảm PWM_MIN hoặc tăng tốc độ lệnh

---

## 🎓 TÀI LIỆU THAM KHẢO

1. **Omnidirectional Mobile Robot - Kinematics and Control**
   - IEEE Paper on 3-wheel omni robot
   
2. **L298N Motor Driver**
   - Voltage drop: ~1.8-2.0V
   - Max current: 2A per channel
   
3. **Motor Specifications**
   - 6MM motor encoder
   - Gear ratio: 1:30
   - No-load speed: 333 RPM (wheel)
   - Encoder: 11 PPR

---

## 💡 TIPS & TRICKS

1. **Bắt đầu với tốc độ thấp**: 20-30 cm/s để test
2. **Test từng motor riêng lẻ**: Kiểm tra chiều quay
3. **Dùng Serial Monitor**: Quan sát PWM và RPM realtime
4. **Calibrate encoder**: Đếm xung khi quay 1 vòng bánh xe
5. **Đo góc chính xác**: Dùng thước đo góc hoặc protractor

---

**📝 Ghi chú**: File này được tạo tự động bởi GitHub Copilot
**📅 Ngày tạo**: 2025-12-06
**🔧 Phiên bản**: 1.0
