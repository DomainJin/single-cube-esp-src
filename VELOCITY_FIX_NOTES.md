# 🔧 Robot Velocity Control - Bug Fixes

## Tóm tắt vấn đề
Robot không chạy với vận tốc mong đợi khi nhận lệnh di chuyển. Đã phát hiện **3 lỗi chính** trong hệ thống điều khiển:

---

## 🔴 **Vấn đề 1: OMNI_ROBOT_RADIUS quá lớn (3.5m)**

### Vị trí lỗi
- File: `include/omni.h` dòng 29
- Giá trị hiện tại: `OMNI_ROBOT_RADIUS = 3.5f` (3500cm)
- Giá trị đúng: `OMNI_ROBOT_RADIUS = 0.231f` (23.1cm)

### Tác động
- **Inverse Kinematics bị sai**: Công thức `V[i] = ... + OMNI_ROBOT_RADIUS * omega` 
- Với R = 3.5m, hệ số quay `R*omega` sẽ chiếm ưu thế → Robot **quay rất chậm hoặc không quay được**
- Tốc độ tuyến tính cũng bị reduce bởi giá trị này

### Công thức đúng
- Tam giác đều cạnh 40cm → bán kính nội tiếp: **R = a/(2√3) = 0.4/1.732 = 0.231m**
- Comment trong code ghi đúng, nhưng #define thì sai

### ✅ Sửa chữa
```cpp
#define OMNI_ROBOT_RADIUS  0.231f  // Thay từ 3.5f
```

---

## 🔴 **Vấn đề 2: OMNI_MAX_WHEEL_RPM không khớp motor specs (333 RPM → 25 RPM)**

### Vị trí lỗi
- File: `include/omni.h` dòng 34
- Giá trị hiện tại: `OMNI_MAX_WHEEL_RPM = 333.0f`
- Giá trị đúng: `OMNI_MAX_WHEEL_RPM = 25.0f` (hoặc 24.4f)

### Tác động
Hàm `omniSpeedToPWM()` ([src/omni.cpp](src/omni.cpp#L199)) sử dụng công thức:
```cpp
float rpm = (wheel_speed / (2.0f * PI * OMNI_WHEEL_RADIUS)) * 60.0f;
float pwm_float = (abs(rpm) / OMNI_MAX_WHEEL_RPM) * pwm_range + OMNI_MIN_SPEED;
```

Nếu lệnh là **15 cm/s** (ví dụ từ lệnh `MOVE:FORWARD`):
1. Tính RPM: `rpm = (0.15 / (2π × 0.05)) × 60 = 28.6 RPM`
2. Map PWM với `OMNI_MAX_WHEEL_RPM = 333`: `pwm = (28.6 / 333) × 195 + 60 = 76.7 PWM` (**quá thấp!**)
3. Map PWM với `OMNI_MAX_WHEEL_RPM = 25`: `pwm = (28.6 / 25) × 225 + 30 = 286 PWM` (**đúng!**)

### Nguồn giá trị đúng
Motor spec (trong [include/motor.h](include/motor.h#L66-L69)):
- **No-load RPM @ 24V: 469 RPM**
- Gear ratio: **1:19.2**
- **Wheel RPM = 469 / 19.2 = 24.4 RPM** ← Đây là max wheel RPM!

### ✅ Sửa chữa
```cpp
#define OMNI_MAX_WHEEL_RPM  25.0f  // Thay từ 333.0f
```

---

## 🔴 **Vấn đề 3: OMNI_MIN_SPEED = 60 PWM quá cao (dập tắt control tốc độ thấp)**

### Vị trí lỗi
- File: `include/omni.h` dòng 33
- Giá trị hiện tại: `OMNI_MIN_SPEED = 60` (bù L298N voltage drop ~2V)
- Giá trị khuyến nghị: `OMNI_MIN_SPEED = 30`

### Tác động
Với OMNI_MIN_SPEED = 60 và OMNI_MIN_THRESHOLD = 60 (trong motor.h):

Khi lệnh yêu cầu **5 cm/s**:
1. RPM = `(0.05 / (2π × 0.05)) × 60 = 9.55 RPM`
2. PWM = `(9.55 / 25) × 195 + 60 = 134 PWM`

Nhưng hàm `omniSpeedToPWM()` có check:
```cpp
if (abs(wheel_speed) < 0.05f) return 0;  // Nếu < 5 cm/s → PWM = 0
```

**→ Bất kỳ lệnh < 5 cm/s đều bị cắt thành STOP, không có control tốc độ mịn!**

Ngoài ra, `PWM_MIN_THRESHOLD = 60` trong `setMotorSpeed()` sẽ khiến:
- 1-59 PWM → Motor không chạy
- 60+ PWM → Motor chạy đột ngột

Giảm thành 30 sẽ:
- Cho phép control từ ~30 PWM trở lên (mịn hơn)
- Giảm ngưỡng "chết" từ 60 xuống 30

### ✅ Sửa chữa
```cpp
#define OMNI_MIN_SPEED      30  // Thay từ 60
```

Cập nhật `include/motor.h`:
```cpp
#define PWM_MIN_THRESHOLD  30  // Thay từ 60
```

---

## 📊 Ví dụ kiểm chứng

### Scenario: Lệnh "MOVE:FORWARD" với tốc độ mặc định 15 cm/s

#### Trước sửa chữa
```
1. omniForward(15) → target_vy = 0.15 m/s
2. updateOmni() → Inverse Kinematics:
   - wheel_speeds[0] ≈ 0.15 m/s (forward)
   
3. omniSpeedToPWM(0.15):
   - rpm = (0.15 / (2π × 0.1)) × 60 = 14.3 RPM  ❌ WHEEL_RADIUS = 0.1 (WRONG!)
   - pwm = (14.3 / 333) × 195 + 60 = 68 PWM  ❌ Quá thấp! Chạy rất chậm
   
4. setMotorSpeed(motor1, 68, FORWARD)
   - PWM = 68 đủ để vượt ngưỡng 60, motor chạy
   - Nhưng tốc độ < mong đợi vì PWM chỉ 68/255
```

#### Sau sửa chữa
```
1. omniForward(15) → target_vy = 0.15 m/s
2. updateOmni() → Inverse Kinematics:
   - wheel_speeds[0] ≈ 0.15 m/s (forward)
   
3. omniSpeedToPWM(0.15):
   - rpm = (0.15 / (2π × 0.05)) × 60 = 28.6 RPM ✅ WHEEL_RADIUS = 0.05 (CORRECT!)
   - pwm = (28.6 / 25) × 225 + 30 = 286 PWM ✅ Gần max, chạy với tốc độ gần mong đợi
   
4. setMotorSpeed(motor1, 286 → constrain = 255, FORWARD)
   - PWM = 255 (max), motor chạy với tốc độ gần max
   - Tốc độ mong đợi ≈ 15 × 25.6 / 25 ≈ 15.4 cm/s ✅
```

---

## ✅ Các thay đổi thực hiện

| File | Thay đổi |
|------|---------|
| `include/omni.h` | OMNI_WHEEL_RADIUS: 0.1 → 0.05 |
| `include/omni.h` | OMNI_ROBOT_RADIUS: 3.5 → 0.231 |
| `include/omni.h` | OMNI_MAX_WHEEL_RPM: 333 → 25 |
| `include/omni.h` | OMNI_MIN_SPEED: 60 → 30 |
| `include/motor.h` | PWM_MIN_THRESHOLD: 60 → 30 |
| `src/omni.cpp` | Comment: Updated PWM range 60-255 → 30-255 |

---

## 🧪 Kiểm tra sau sửa chữa

Bạn nên kiểm tra:

1. **Tốc độ tuyến tính**: 
   - Gửi lệnh `MOVE:FORWARD` → Robot chạy với tốc độ ≈ 15 cm/s

2. **Tốc độ quay**:
   - Gửi lệnh `ROBOT:ROTATE:45` → Robot quay 45° trong thời gian hợp lý (không chậm)

3. **Tốc độ thấp**:
   - Gửi lệnh `ROBOT:LINEAR:5,0,0` (5 cm/s) → Motor chạy mượt (không bị snap từ 0 → max)

4. **Encoder feedback**:
   - Kiểm tra log: `[ENCODER_1]` để xác nhận RPM được đọc chính xác

---

## 📚 Tài liệu tham khảo

- Motor specs: [include/motor.h](include/motor.h#L66-L69)
- Omni config: [include/omni.h](include/omni.h#L25-L34)
- Speed mapping: [src/omni.cpp](src/omni.cpp#L192-L220)
- Motor control: [src/motor.cpp](src/motor.cpp#L228-L280)
