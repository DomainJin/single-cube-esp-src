# ESP32 Pin Mapping - Single Cube Project

## 📌 CHÂN ĐANG SỬ DỤNG

### I2C (MPU6050)
| Chân | Chức năng | Ghi chú |
|------|-----------|---------|
| GPIO 27 | I2C SDA | MPU6050 accelerometer |
| GPIO 12 | I2C SCL | MPU6050 accelerometer |

### LED WS2812
| Chân | Chức năng | Ghi chú |
|------|-----------|---------|
| GPIO 5 | LED_PIN | WS2812 Data (150 LEDs) |

### IR/ADC Sensors
| Chân | Chức năng | Ghi chú |
|------|-----------|---------|
| GPIO 34 | ANALOG_READ_PIN_1 | ADC Input 1 (chỉ input) |
| GPIO 35 | ANALOG_READ_PIN_2 | ADC Input 2 (chỉ input) |

### A4L Control Buttons
| Chân | Chức năng | Ghi chú |
|------|-----------|---------|
| GPIO 25 | Button_Next_Pin | A4L Next button |
| GPIO 26 | Button_SyncMode | A4L Sync Mode |
| GPIO 27 | Button_HDMI_Mode | A4L HDMI Mode ⚠️ TRÙNG I2C SDA! |

### Xilanh Control
| Chân | Chức năng | Ghi chú |
|------|-----------|---------|
| GPIO 2 | Xilanh Output 1 | LOW (đang dùng trong main) |
| GPIO 15 | Xilanh Output 2 | LOW (đang dùng trong main) |

### WiFi (Internal)
| Chân | Chức năng | Ghi chú |
|------|-----------|---------|
| N/A | WiFi | Built-in, không dùng GPIO |

---

## ✅ CHÂN CÒN TRỐNG (Available)

### Digital I/O Pins
| Chân | Đặc điểm | Khuyến nghị sử dụng |
|------|----------|---------------------|
| **GPIO 4** | ✅ Safe | **Encoder 1A** |
| **GPIO 13** | ✅ Safe | **Encoder 1B** |
| **GPIO 14** | ✅ Safe | **Encoder 2A** |
| **GPIO 16** | ✅ Safe (RX2) | **Encoder 2B** |
| **GPIO 17** | ✅ Safe (TX2) | **Encoder 3A** |
| **GPIO 18** | ✅ Safe | **Encoder 3B** |
| **GPIO 19** | ✅ Safe | **L298 IN1** (Motor 1) |
| **GPIO 21** | ⚠️ I2C SDA default | **L298 IN2** (Motor 1) |
| **GPIO 22** | ⚠️ I2C SCL default | **L298 IN3** (Motor 2) |
| **GPIO 23** | ✅ Safe | **L298 IN4** (Motor 2) |
| **GPIO 32** | ✅ Safe, có ADC | **L298 IN5** (Motor 3) |
| **GPIO 33** | ✅ Safe, có ADC | **L298 IN6** (Motor 3) |

### Input Only Pins (chỉ đọc, không xuất tín hiệu)
| Chân | Đặc điểm | Ghi chú |
|------|----------|---------|
| GPIO 36 (VP) | Input only | Có thể dùng cho Encoder (chỉ đọc) |
| GPIO 39 (VN) | Input only | Có thể dùng cho Encoder (chỉ đọc) |

---

## 🎯 ĐỀ XUẤT PINOUT CHO YÊU CẦU CỦA BẠN

### 1. I2C (Cần 2 chân)
```
✅ SDA: GPIO 21 (I2C default)
✅ SCL: GPIO 22 (I2C default)
```
**Lưu ý**: Hiện tại GPIO 27 đang bị trùng giữa I2C SDA và Button_HDMI_Mode!

### 2. Encoder Inputs (Cần 6 chân input)
```
✅ Encoder 1A: GPIO 4
✅ Encoder 1B: GPIO 13
✅ Encoder 2A: GPIO 14
✅ Encoder 2B: GPIO 16
✅ Encoder 3A: GPIO 17
✅ Encoder 3B: GPIO 18
```

### 3. L298 Motor Driver (Cần 6 chân output)
```
✅ Motor 1 IN1: GPIO 19
✅ Motor 1 IN2: GPIO 23
✅ Motor 2 IN3: GPIO 32
✅ Motor 2 IN4: GPIO 33
✅ Motor 3 IN5: GPIO 2  (hoặc GPIO 25 nếu bỏ A4L)
✅ Motor 3 IN6: GPIO 15 (hoặc GPIO 26 nếu bỏ A4L)
```

---

## ⚠️ XUNG ĐỘT PHÁT HIỆN

### Vấn đề 1: GPIO 27 bị trùng
- **MPU6050 I2C SDA**: GPIO 27
- **A4L Button_HDMI_Mode**: GPIO 27

**Giải pháp**:
1. Chuyển I2C sang GPIO 21/22 (default pins)
2. Hoặc chuyển Button_HDMI_Mode sang chân khác

### Vấn đề 2: GPIO 12 có thể gây vấn đề boot
- GPIO 12 là strapping pin, mức LOW khi boot sẽ chọn flash voltage
- Nên tránh hoặc đảm bảo không kéo LOW khi khởi động

---

## 📝 KHUYẾN NGHỊ

### Nếu giữ nguyên MPU6050 ở GPIO 27/12:
```cpp
// Encoder
#define ENCODER_1A  4
#define ENCODER_1B  13
#define ENCODER_2A  14
#define ENCODER_2B  16
#define ENCODER_3A  17
#define ENCODER_3B  18

// L298 Motor Driver
#define MOTOR_1_IN1 19
#define MOTOR_1_IN2 23
#define MOTOR_2_IN3 32
#define MOTOR_2_IN4 33
#define MOTOR_3_IN5 25  // Thay A4L Next
#define MOTOR_3_IN6 26  // Thay A4L Sync
```

### Nếu chuyển MPU6050 sang I2C default (KHUYẾN NGHỊ):
```cpp
// I2C cho MPU6050
#define MPU_SDA 21  // I2C default
#define MPU_SCL 22  // I2C default

// Encoder (6 chân)
#define ENCODER_1A  4
#define ENCODER_1B  13
#define ENCODER_2A  14
#define ENCODER_2B  16
#define ENCODER_3A  17
#define ENCODER_3B  18

// L298 Motor Driver (6 chân)
#define MOTOR_1_IN1 19
#define MOTOR_1_IN2 23
#define MOTOR_2_IN3 25  // Có thể giữ A4L nếu không cần
#define MOTOR_2_IN4 26  // Có thể giữ A4L nếu không cần
#define MOTOR_3_IN5 27  // Giải phóng từ A4L
#define MOTOR_3_IN6 32
```

---

## 🔧 CÁC CHÂN KHÔNG NÊN DÙNG

| Chân | Lý do |
|------|-------|
| GPIO 0 | Strapping pin (Boot mode) |
| GPIO 1 | TX0 (USB Serial) |
| GPIO 3 | RX0 (USB Serial) |
| GPIO 6-11 | Connected to SPI Flash |
| GPIO 34-39 | Input only, không dùng cho output |

---

## 📊 TỔNG KẾT

- ✅ **Đủ chân** cho tất cả yêu cầu: 2 I2C + 6 Encoder + 6 L298
- ⚠️ Cần giải quyết xung đột GPIO 27 (MPU6050 SDA vs A4L)
- 💡 Khuyến nghị chuyển MPU6050 sang GPIO 21/22 (I2C default)
- 🎯 Còn dư GPIO 32, 33 có thể dùng cho PWM hoặc ADC
