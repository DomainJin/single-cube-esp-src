# Hướng dẫn sử dụng điều khiển Robot Omni 3 bánh

## Cấu hình Robot

- **Bố cục**: 3 bánh xe tạo thành tam giác đều
- **Khoảng cách từ tâm đến bánh**: 35cm (đến biên ngoài)
- **Bánh xe**:
  - Đường kính: 10cm (bán kính 5cm)
  - Độ dày: 3.5cm
- **Hệ thống điều khiển**: PID với feedforward (Kp=3.5, Ki=0.8, Kd=0.2)

## Bố trí Motor

```
        M1 (Front)
         ⬆️
         |
         |
    M2 ⬋   ⬊ M3
(Back-Left) (Back-Right)
```

- **Motor 1**: Phía trước (góc 90°)
- **Motor 2**: Phía sau-trái (góc 210°)
- **Motor 3**: Phía sau-phải (góc 330°)

## 5 Hàm Điều Khiển Cơ Bản

### 1. Tiến (Forward)
```cpp
omniForward(20.0);  // Tiến với tốc độ 20 cm/s
```
- M1 quay tiến (100% tốc độ)
- M2 và M3 quay lùi (50% tốc độ mỗi motor)

### 2. Lùi (Backward)
```cpp
omniBackward(20.0);  // Lùi với tốc độ 20 cm/s
```
- M1 quay lùi (100% tốc độ)
- M2 và M3 quay tiến (50% tốc độ mỗi motor)

### 3. Di chuyển sang Trái (Strafe Left)
```cpp
omniStrafeLeft(15.0);  // Di chuyển sang trái với tốc độ 15 cm/s
```
- M1 đứng yên
- M2 quay (86.6% tốc độ)
- M3 quay ngược chiều (86.6% tốc độ)

### 4. Di chuyển sang Phải (Strafe Right)
```cpp
omniStrafeRight(15.0);  // Di chuyển sang phải với tốc độ 15 cm/s
```
- M1 đứng yên
- M2 quay ngược (86.6% tốc độ)
- M3 quay (86.6% tốc độ)

### 5. Dừng (Stop)
```cpp
omniStop();  // Dừng tất cả motor
```
- Tất cả motor về RPM = 0
- PID vẫn hoạt động để duy trì trạng thái dừng

## Ví dụ sử dụng trong main.cpp

### Ví dụ 1: Di chuyển đơn giản
```cpp
void loop() {
    // Tiến trong 2 giây
    omniForward(20.0);
    delay(2000);
    
    // Dừng 1 giây
    omniStop();
    delay(1000);
    
    // Lùi trong 2 giây
    omniBackward(20.0);
    delay(2000);
    
    // Dừng 1 giây
    omniStop();
    delay(1000);
    
    // PID tự động cập nhật
    update3Motors();
}
```

### Ví dụ 2: Di chuyển sang hai bên
```cpp
void loop() {
    // Sang trái 2 giây
    omniStrafeLeft(15.0);
    delay(2000);
    
    // Dừng 1 giây
    omniStop();
    delay(1000);
    
    // Sang phải 2 giây
    omniStrafeRight(15.0);
    delay(2000);
    
    // Dừng 1 giây
    omniStop();
    delay(1000);
    
    // PID tự động cập nhật
    update3Motors();
}
```

### Ví dụ 3: Điều khiển từ UDP
```cpp
// Trong handleUDPReceive():
if (data == "FORWARD") {
    omniForward(20.0);
} else if (data == "BACKWARD") {
    omniBackward(20.0);
} else if (data == "LEFT") {
    omniStrafeLeft(15.0);
} else if (data == "RIGHT") {
    omniStrafeRight(15.0);
} else if (data == "STOP") {
    omniStop();
}
```

## Công thức Kinematics

### Chuyển đổi vận tốc sang RPM:
```
RPM = (vận_tốc_cm/s × 60) / (chu_vi_bánh_cm)
Chu vi bánh = 2 × π × 5cm = 31.42 cm
```

### Ma trận Kinematics (Omni-3):
```
M1 = Vx × sin(90°) + Vy × cos(90°) = Vx
M2 = Vx × sin(210°) + Vy × cos(210°) = -0.5×Vx - 0.866×Vy
M3 = Vx × sin(330°) + Vy × cos(330°) = -0.5×Vx + 0.866×Vy
```

Trong đó:
- **Vx**: Vận tốc tiến/lùi (dương = tiến)
- **Vy**: Vận tốc trái/phải (dương = phải)
- **0.866 ≈ √3/2**

## Tốc độ khuyến nghị

| Chế độ | Tốc độ (cm/s) | RPM tương ứng | Ghi chú |
|--------|---------------|---------------|---------|
| Thấp   | 10 cm/s       | ~19 RPM       | An toàn, chính xác |
| Trung bình | 20 cm/s   | ~38 RPM       | Cân bằng tốt |
| Cao    | 30 cm/s       | ~57 RPM       | Nhanh, cần kiểm tra |
| Max    | 40 cm/s       | ~76 RPM       | Giới hạn motor |

**Lưu ý**: Motor 38RD249009 có tốc độ no-load ~17 RPM (sau hộp số). Không nên vượt quá 80-90 RPM để đảm bảo mô-men xoắn đủ.

## Giám sát Realtime

### Serial Monitor Output (mỗi 500ms):
```
M1 target=38.2 RPM=37.8 PWM=58 | M2 target=-19.1 RPM=-18.9 PWM=29 | M3 target=-19.1 RPM=-19.2 PWM=30
```

### UDP Output (mỗi 500ms):
```cpp
sendSpeed((int16_t)m1.rpmFilt, (int16_t)m2.rpmFilt, (int16_t)m3.rpmFilt);
```

## Debug & Tuning

### Kiểm tra PID hoạt động:
1. Gọi `omniForward(20.0)`
2. Quan sát Serial: target RPM vs actual RPM
3. Sai số < 2 RPM là tốt

### Nếu motor không đạt target:
- Kiểm tra encoder đếm đúng không
- Tăng Kp (hiện tại 3.5)
- Kiểm tra PWM có đủ không (OUT_MIN=25, OUT_MAX=255)
- Kiểm tra nguồn điện có đủ không

### Nếu motor dao động:
- Giảm Kp
- Tăng MIN_ERR_FOR_DRIVE (hiện tại 1.5 RPM)
- Giảm RPM_ALPHA (làm mượt hơn)

## API Functions

```cpp
// Khởi tạo (gọi 1 lần trong setup())
void init3Motors();

// Cập nhật PID (gọi trong loop())
void update3Motors();

// 5 hàm điều khiển
void omniForward(float speed_cm_s);
void omniBackward(float speed_cm_s);
void omniStrafeLeft(float speed_cm_s);
void omniStrafeRight(float speed_cm_s);
void omniStop();

// Helper functions
float velocityToRPM(float velocity_cm_s);
void setMotorTargetRPM(Motor &m, float rpm);
```

## Lưu ý quan trọng

1. **Luôn gọi `update3Motors()` trong loop()** - PID cần cập nhật liên tục (mỗi ~10ms)
2. **Không dùng delay() dài** - Sẽ làm PID không cập nhật kịp
3. **Kiểm tra encoder** - Nếu không đếm xung, PID sẽ cấp full PWM
4. **Nguồn điện ổn định** - PID cần PWM chính xác
5. **Hiệu chỉnh chiều quay** - Nếu robot đi sai hướng, kiểm tra DIR pin

## Tích hợp với UDP Control (TODO)

Để điều khiển từ UDP, thêm vào `udpconfig.cpp`:

```cpp
// Trong handleUDPReceive():
else if (data.startsWith("MOVE:")) {
    String direction = data.substring(5);
    direction.trim();
    
    if (direction == "FORWARD") {
        omniForward(20.0);
    } else if (direction == "BACKWARD") {
        omniBackward(20.0);
    } else if (direction == "LEFT") {
        omniStrafeLeft(15.0);
    } else if (direction == "RIGHT") {
        omniStrafeRight(15.0);
    } else if (direction == "STOP") {
        omniStop();
    }
}
```

Nhớ thêm `#include "3_motor.h"` vào đầu file `udpconfig.cpp`.
