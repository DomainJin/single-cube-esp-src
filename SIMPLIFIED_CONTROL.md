# ĐÃ XÓA TẤT CẢ LOGIC ĐIỀU KHIỂN - CHỈ GIỮ UDP COMMANDS

## 🎯 THAY ĐỔI

### ✅ ĐÃ LÀM
1. **Tắt hoàn toàn PID control** - Comment `updateOmni()` trong main.cpp
2. **Xóa kinematics** - updateOmni() không còn chạy nên inverse/forward kinematics bị vô hiệu hóa
3. **Điều khiển trực tiếp PWM** - omniForward(), omniStrafe(), omniStop() giờ chỉ set PWM cố định

### 📝 CÁC HÀM ĐƯỢC ĐƠN GIẢN HÓA

#### `omniForward(speed_cm_s)` - Tiến/Lùi
```cpp
PWM = 100 (cố định ~40%)

FORWARD (speed > 0):
  Motor 1: FORWARD, PWM=100
  Motor 2: BACKWARD, PWM=100  
  Motor 3: BACKWARD, PWM=100

BACKWARD (speed < 0):
  Motor 1: BACKWARD, PWM=100
  Motor 2: FORWARD, PWM=100
  Motor 3: FORWARD, PWM=100
```

#### `omniStrafe(speed_cm_s)` - Sang trái/phải
```cpp
PWM = 100 (cố định ~40%)

RIGHT (speed > 0):
  Motor 1: OFF (PWM=0)
  Motor 2: FORWARD, PWM=100
  Motor 3: BACKWARD, PWM=100

LEFT (speed < 0):
  Motor 1: OFF (PWM=0)
  Motor 2: BACKWARD, PWM=100
  Motor 3: FORWARD, PWM=100
```

#### `omniStop()` - Dừng
```cpp
Motor 1: PWM=0
Motor 2: PWM=0
Motor 3: PWM=0
```

### 🎮 UDP COMMANDS VẪN HOẠT ĐỘNG
- `MOVE:FORWARD` → omniForward(10)
- `MOVE:BACKWARD` → omniForward(-10)
- `MOVE:LEFT` → omniStrafe(-10)
- `MOVE:RIGHT` → omniStrafe(10)
- `MOVE:STOP` → omniStop()

## 🧪 TEST NGAY

### Test Motor Direction (quan trọng nhất)

1. **Gửi `MOVE:FORWARD`** - Quan sát serial log:
   ```
   [OMNI_SIMPLE] FORWARD: M1=FWD(100), M2=BWD(100), M3=BWD(100)
   ```
   **KỲ VỌNG**: 
   - Motor 1 quay chiều FORWARD
   - Motor 2 quay chiều BACKWARD
   - Motor 3 quay chiều BACKWARD
   
   **NẾU SAI HƯỚNG**:
   - Kiểm tra encoder count tăng hay giảm
   - Có thể cần đảo DIR pin hoặc encoder wiring

2. **Gửi `MOVE:RIGHT`** - Quan sát:
   ```
   [OMNI_SIMPLE] RIGHT: M1=STOP, M2=FWD(100), M3=BWD(100)
   ```
   **KỲ VỌNG**:
   - Motor 1 đứng yên
   - Motor 2 FORWARD
   - Motor 3 BACKWARD

3. **Gửi `MOVE:STOP`** - Tất cả motor phải dừng ngay lập tức

### Debug với Serial Monitor

Log đơn giản hơn rất nhiều, chỉ cần xem:
```
[UDP_MOVE] Received direction: FORWARD
[OMNI_SIMPLE] FORWARD: M1=FWD(100), M2=BWD(100), M3=BWD(100)
```

**KHÔNG CÒN:**
- PID debug (P, I, D values)
- RPM calculations
- Target RPM vs Current RPM
- Encoder diff (vẫn có thể đọc nhưng không dùng)

## 🔧 NẾU VẪN SAI HƯỚNG

### Trường hợp 1: Motor 1 sai hướng
```cpp
// Trong motor.h dòng 67-72, thay đổi:
#define MOTOR_1_DIRECTION_INVERT 1  // Đổi từ 0 → 1
```

### Trường hợp 2: Motor 2 sai hướng  
```cpp
#define MOTOR_2_DIRECTION_INVERT 1  // Đổi từ 0 → 1
```

### Trường hợp 3: Motor 3 sai hướng
```cpp
#define MOTOR_3_DIRECTION_INVERT 0  // Đổi từ 1 → 0 (hoặc ngược lại)
```

### Trường hợp 4: Encoder đếm ngược
```cpp
// Trong motor.h dòng 46-51, thay đổi:
#define MOTOR_1_ENCODER_INVERT 1  // Đổi từ 0 → 1 (hoặc ngược lại)
```

## 🎛️ ĐIỀU CHỈNH PWM

Nếu muốn thay đổi tốc độ, sửa trong omni.cpp:

### Tăng tốc độ (nếu motor chạy quá chậm)
```cpp
int pwm = 150;  // Tăng từ 100 → 150 (~60% power)
```

### Giảm tốc độ (nếu motor chạy quá nhanh)
```cpp
int pwm = 70;   // Giảm từ 100 → 70 (~27% power)
```

### PWM khác nhau cho mỗi motor
```cpp
// Trong omniForward():
ledcWrite(motor1.pwm_channel_en, 100);
ledcWrite(motor2.pwm_channel_en, 120);  // M2 mạnh hơn
ledcWrite(motor3.pwm_channel_en, 100);
```

## ⚙️ FILE ĐÃ SỬA

1. **src/main.cpp** - Line 146
   - Comment `updateOmni()` để tắt PID/kinematics

2. **src/omni.cpp**
   - `omniForward()` - Direct PWM control, không kinematics
   - `omniStrafe()` - Direct PWM control
   - `omniStop()` - Set PWM = 0 trực tiếp
   - `updateOmni()` - Vẫn còn code nhưng không được gọi

3. **src/motor.cpp** - KHÔNG CẦN SỬA
   - PID functions vẫn còn nhưng không được gọi
   - Encoder ISR vẫn chạy (có thể dùng để debug)

## 📊 EXPECTED BEHAVIOR

### Khi khởi động
```
[OMNI_SIMPLE] STOP: All motors OFF
```

### Khi nhận FORWARD
```
[UDP_MOVE] Received direction: FORWARD
[UDP_MOVE] ⬆️ Moving FORWARD
[OMNI_SIMPLE] FORWARD: M1=FWD(100), M2=BWD(100), M3=BWD(100)
```

### Khi nhận STOP
```
[UDP_MOVE] Received direction: STOP
[UDP_MOVE] 🛑 STOP
[OMNI_SIMPLE] STOP: All motors OFF
```

## 🐛 DEBUG CHECKLIST

- [ ] Serial monitor shows `[OMNI_SIMPLE]` messages
- [ ] Motor 1 responds to direction changes
- [ ] Motor 2 responds to direction changes  
- [ ] Motor 3 responds to direction changes
- [ ] All motors stop immediately when STOP command sent
- [ ] No PID messages in log (đã tắt thành công)
- [ ] Encoder counts still incrementing (ISR vẫn hoạt động)

## 🔄 ĐỂ KHÔI PHỤC PID/KINEMATICS

Nếu muốn bật lại:
```cpp
// Trong main.cpp line 146, uncomment:
updateOmni();  // Bỏ comment này
```

---
**Ngày tạo**: 2025-12-24  
**Mục đích**: Debug cơ bản - loại bỏ tất cả complexity để test motor direction
