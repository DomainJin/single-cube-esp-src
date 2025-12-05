# ESP32 Pin Mapping - Single Cube Project
## Board: ESP32 NodeMCU-32S (ESP-32S Kit)

## 📌 CHÂN ĐANG SỬ DỤNG (Hiện tại)

### I2C (QMC5883L Compass)
| Chân Board | GPIO | Chức năng | Module | Ghi chú |
|------------|------|-----------|--------|---------|
| Pin 12 | GPIO 32 | I2C SDA | QMC5883L | ⚠️ CẦN KIỂM TRA - hiện code dùng GPIO 16 |
| Pin 16 | GPIO 27 | I2C SCL | QMC5883L | ⚠️ CẦN KIỂM TRA - hiện code dùng GPIO 17 |

### LED WS2812
| Chân Board | GPIO | Chức năng | Module | Ghi chú |
|------------|------|-----------|--------|---------|
| Pin 10 | GPIO 34 | LED_PIN | WS2812 | ⚠️ XUNG ĐỘT - GPIO 34 chỉ INPUT, không xuất được! |

### IR/ADC Sensors
| Chân Board | GPIO | Chức năng | Module | Ghi chú |
|------------|------|-----------|--------|---------|
| Pin 10 | GPIO 34 | ANALOG_READ_PIN_1 | IR Sensor | ✅ OK - ADC Input |
| Pin 8 | GPIO 39 | ANALOG_READ_PIN_2 | IR Sensor | ⚠️ CẦN KIỂM TRA - hiện code dùng GPIO 35 |

### A4L Control Buttons
| Chân Board | GPIO | Chức năng | Module | Ghi chú |
|------------|------|-----------|--------|---------|
| Pin 25 | GPIO 16 | Button_Next_Pin | A4L | ⚠️ CẦN KIỂM TRA - hiện code dùng GPIO 25 |
| Pin 24 | GPIO 4 | Button_SyncMode | A4L | ⚠️ CẦN KIỂM TRA - hiện code dùng GPIO 26 |
| Pin 16 | GPIO 27 | Button_HDMI_Mode | A4L | ✅ OK |

### Xilanh Control
| Chân Board | GPIO | Chức năng | Module | Ghi chú |
|------------|------|-----------|--------|---------|
| Pin 40 | GPIO 3 | Xilanh Output 1 | Main | ⚠️ XUNG ĐỘT - GPIO 3 là RX0! |
| Pin 29 | GPIO 10 | Xilanh Output 2 | Main | ⚠️ XUNG ĐỘT - GPIO 10 nối Flash! |

### WiFi (Internal)
| Chân | Chức năng | Module | Ghi chú |
|------|-----------|--------|---------|
| N/A | WiFi | Built-in | Không dùng GPIO |

---

## 📊 TỔNG HỢP CHÂN ĐÃ SỬ DỤNG

| GPIO | Chức năng | Module | Loại | Trạng thái |
|------|-----------|--------|------|------------|
| **2** | Xilanh 1 | Main | OUTPUT | LOW |
| **5** | WS2812 Data | LED | OUTPUT | Active |
| **15** | Xilanh 2 | Main | OUTPUT | LOW |
| **16** | I2C SDA | QMC5883L | I2C | Active |
| **17** | I2C SCL | QMC5883L | I2C | Active |
| **25** | Button Next | A4L | OUTPUT | Active |
| **26** | Button Sync | A4L | OUTPUT | Active |
| **27** | Button HDMI | A4L | OUTPUT | Active |
| **34** | ADC IR 1 | IR Sensor | INPUT (ADC) | Active |
| **35** | ADC IR 2 | IR Sensor | INPUT (ADC) | Active |

**Tổng cộng: 10 chân GPIO đang sử dụng**

---

## ✅ CHÂN CÒN TRỐNG (Available GPIO)

### Digital I/O Pins (Safe to use - Có thể INPUT/OUTPUT)
| GPIO | Đặc điểm | Khuyến nghị sử dụng | Trạng thái |
|------|----------|---------------------|------------|
| **4** | ✅ Safe | 🎯 ENCODER_1A hoặc MOTOR PWM | ✅ Ưu tiên encoder |
| **12** | ⚠️ Strapping pin | MOTOR_3_PWM_A (tránh HIGH khi boot) | ⚠️ Dùng cẩn thận |
| **13** | ✅ Safe | 🎯 ENCODER_1B hoặc MOTOR_3_PWM_A | ✅ Ưu tiên motor |
| **14** | ✅ Safe | 🎯 ENCODER_2A hoặc MOTOR_3_PWM_B | ✅ Ưu tiên motor |
| **18** | ✅ Safe | 🎯 ENCODER_2B hoặc ENCODER_1B | ✅ Ưu tiên encoder |
| **19** | ✅ Safe | 🎯 ENCODER_3A hoặc ENCODER_2A | ✅ Ưu tiên encoder |
| **21** | ✅ I2C SDA default | 🎯 MOTOR_2_PWM_A (không xung đột I2C) | ✅ Ưu tiên motor |
| **22** | ✅ I2C SCL default | 🎯 MOTOR_2_PWM_B (không xung đột I2C) | ✅ Ưu tiên motor |
| **23** | ✅ Safe | 🎯 ENCODER_3B | ✅ Ưu tiên encoder |
| **32** | ✅ Safe, có ADC | 🎯 MOTOR_1_PWM_A (ADC current sensing) | ✅ Ưu tiên motor |
| **33** | ✅ Safe, có ADC | 🎯 MOTOR_1_PWM_B (ADC current sensing) | ✅ Ưu tiên motor |

### Input Only Pins (Chỉ đọc INPUT - Không xuất OUTPUT)
| GPIO | Đặc điểm | Khuyến nghị sử dụng | Trạng thái |
|------|----------|---------------------|------------|
| **36** (VP) | Input only, có ADC | 🎯 ENCODER_3A (tối ưu) | ✅ Dùng cho encoder |
| **39** (VN) | Input only, có ADC | 🎯 ENCODER_3B (tối ưu) | ✅ Dùng cho encoder |

**Tổng số GPIO còn trống: 13 chân** (11 I/O + 2 Input-only)

---

## 🎯 ĐỀ XUẤT PINOUT CHO MỞ RỘNG

### 1. Nếu cần thêm I2C device (MPU6050, OLED, etc.)
```cpp
// Dùng chung bus I2C với QMC5883L
#define I2C_SDA  16  // Đang dùng cho QMC5883L
#define I2C_SCL  17  // Đang dùng cho QMC5883L
// Hoặc dùng I2C thứ 2
#define I2C2_SDA 21  // I2C default
#define I2C2_SCL 22  // I2C default
```

### 2. ✅ 6 CHÂN ENCODER INPUT (Đọc tín hiệu encoder - 3 encoder x 2 chân)
```cpp
// Encoder 1
#define ENCODER_1A  4   // GPIO 4  - Safe, Digital Input
#define ENCODER_1B  13  // GPIO 13 - Safe, Digital Input

// Encoder 2  
#define ENCODER_2A  14  // GPIO 14 - Safe, Digital Input
#define ENCODER_2B  18  // GPIO 18 - Safe, Digital Input

// Encoder 3
#define ENCODER_3A  19  // GPIO 19 - Safe, Digital Input
#define ENCODER_3B  23  // GPIO 23 - Safe, Digital Input
```

**Đặc điểm:**
- ✅ Tất cả chân đều hỗ trợ INPUT với pull-up/pull-down
- ✅ Không xung đột với hệ thống hiện tại
- ✅ Không phải strapping pins quan trọng
- 💡 Có thể dùng interrupt để đếm xung encoder chính xác

### 3. ⚡ 6 CHÂN PWM OUTPUT (Điều khiển motor - 3 motor x 2 chân)

**Option 1 - PWM chia sẻ chân với Encoder (nếu không dùng đồng thời):**
```cpp
// Motor 1 PWM
#define MOTOR_1_PWM_A  4   // GPIO 4  - Safe (trùng ENCODER_1A)
#define MOTOR_1_PWM_B  13  // GPIO 13 - Safe (trùng ENCODER_1B)

// Motor 2 PWM
#define MOTOR_2_PWM_A  14  // GPIO 14 - Safe (trùng ENCODER_2A)
#define MOTOR_2_PWM_B  18  // GPIO 18 - Safe (trùng ENCODER_2B)

// Motor 3 PWM
#define MOTOR_3_PWM_A  19  // GPIO 19 - Safe (trùng ENCODER_3A)
#define MOTOR_3_PWM_B  23  // GPIO 23 - Safe (trùng ENCODER_3B)
```

**Option 2 - PWM độc lập (khuyến nghị - không trùng chân):**
```cpp
// Motor 1 PWM
#define MOTOR_1_PWM_A  32  // GPIO 32 - Safe, có ADC
#define MOTOR_1_PWM_B  33  // GPIO 33 - Safe, có ADC

// Motor 2 PWM
#define MOTOR_2_PWM_A  21  // GPIO 21 - Safe (I2C SDA default)
#define MOTOR_2_PWM_B  22  // GPIO 22 - Safe (I2C SCL default)

// Motor 3 PWM
#define MOTOR_3_PWM_A  12  // GPIO 12 - OK (strapping pin, tránh HIGH khi boot)
#define MOTOR_3_PWM_B  36  // GPIO 36 - Input only, có thể đọc PWM feedback
```

**Đặc điểm Option 2:**
- ✅ GPIO 32, 33 có ADC - có thể đọc current sensing
- ✅ GPIO 21, 22 an toàn, không xung đột I2C (đang dùng GPIO 16/17)
- ⚠️ GPIO 12 là strapping pin - giữ LOW khi boot
- ⚠️ GPIO 36 chỉ INPUT - phù hợp cho encoder feedback, không điều khiển được motor

**Khuyến nghị cuối cùng - PWM hoàn toàn độc lập:**
```cpp
// 6 chân PWM OUTPUT an toàn nhất (không trùng encoder)
#define MOTOR_1_PWM_A  32  // GPIO 32 - Safe, có ADC
#define MOTOR_1_PWM_B  33  // GPIO 33 - Safe, có ADC
#define MOTOR_2_PWM_A  21  // GPIO 21 - Safe
#define MOTOR_2_PWM_B  22  // GPIO 22 - Safe
#define MOTOR_3_PWM_A  12  // GPIO 12 - OK (strapping, giữ LOW khi boot)
#define MOTOR_3_PWM_B  13  // GPIO 13 - Safe (nếu không dùng cho encoder)
```

### 4. 🎯 KẾT HỢP ENCODER + PWM (Khuyến nghị sử dụng)

**Cấu hình A - Encoder và PWM hoàn toàn tách biệt:**
```cpp
// 6 ENCODER INPUTS (GPIO: 4, 13, 14, 18, 19, 23)
#define ENCODER_1A  4
#define ENCODER_1B  13
#define ENCODER_2A  14
#define ENCODER_2B  18
#define ENCODER_3A  19
#define ENCODER_3B  23

// 6 PWM OUTPUTS (GPIO: 32, 33, 21, 22, 12, 36)
// ⚠️ Lưu ý: GPIO 36 chỉ INPUT - thay bằng GPIO khác nếu cần OUTPUT
#define MOTOR_1_PWM_A  32  // Safe, có ADC
#define MOTOR_1_PWM_B  33  // Safe, có ADC
#define MOTOR_2_PWM_A  21  // Safe
#define MOTOR_2_PWM_B  22  // Safe
#define MOTOR_3_PWM_A  12  // OK (strapping)
#define MOTOR_3_PWM_B  0   // ❌ GPIO 0 là strapping - TRÁNH!
```

**Cấu hình B - Encoder riêng, PWM ưu tiên an toàn (Khuyến nghị):**
```cpp
// 6 ENCODER INPUTS
#define ENCODER_1A  4   // GPIO 4
#define ENCODER_1B  18  // GPIO 18
#define ENCODER_2A  19  // GPIO 19
#define ENCODER_2B  23  // GPIO 23
#define ENCODER_3A  36  // GPIO 36 - Input only, OK cho encoder
#define ENCODER_3B  39  // GPIO 39 - Input only, OK cho encoder

// 6 PWM OUTPUTS (không trùng encoder)
#define MOTOR_1_PWM_A  32  // GPIO 32 - Safe, có ADC
#define MOTOR_1_PWM_B  33  // GPIO 33 - Safe, có ADC
#define MOTOR_2_PWM_A  21  // GPIO 21 - Safe
#define MOTOR_2_PWM_B  22  // GPIO 22 - Safe
#define MOTOR_3_PWM_A  13  // GPIO 13 - Safe
#define MOTOR_3_PWM_B  14  // GPIO 14 - Safe
```

**✅ Cấu hình B là tối ưu nhất:**
- Encoder dùng GPIO input-only (36, 39) - không lãng phí GPIO output
- PWM dùng 6 GPIO output an toàn nhất (32, 33, 21, 22, 13, 14)
- Không có xung đột, không có strapping pin quan trọng

---

## ⚠️ LƯU Ý QUAN TRỌNG

### Các chân không nên dùng:
| Chân | Lý do |
|------|-------|
| GPIO 0 | Strapping pin (Boot mode) - phải LOW khi boot để programming |
| GPIO 1 | TX0 (USB Serial) - dùng cho Serial Monitor |
| GPIO 3 | RX0 (USB Serial) - dùng cho Serial Monitor |
| GPIO 6-11 | Connected to SPI Flash - KHÔNG BAO GIỜ dùng |
| GPIO 12 | Strapping pin - tránh pull HIGH khi boot |

### Strapping Pins cần chú ý:
- **GPIO 0**: Phải LOW khi boot để vào programming mode
- **GPIO 2**: Phải LOW khi boot (đang dùng - OK)
- **GPIO 12**: Flash voltage select (3.3V if LOW)
- **GPIO 15**: Boot debug mode (nên LOW)

---

## 📊 TỔNG KẾT

### 🔧 Hiện tại (GPIO đang sử dụng):
| Hệ thống | GPIO đã dùng | Số lượng |
|----------|--------------|----------|
| **I2C** | 16, 17 | 2 chân |
| **LED WS2812** | 5 | 1 chân |
| **IR Sensors** | 34, 35 | 2 chân |
| **A4L Buttons** | 25, 26, 27 | 3 chân |
| **Xilanh Control** | 2, 15 | 2 chân |
| **Tổng cộng** | | **10 GPIO** |

### ✅ Mở rộng được (GPIO còn trống):
| Chức năng | GPIO khuyến nghị | Số lượng | Ghi chú |
|-----------|------------------|----------|---------|
| **3 Motor L298N** | 4, 13, 14, 18, 19, 23, 32, 33, 21, 22, 36, 39 | 12 chân | 3 động cơ x 4 chân (IN1, IN2, ENC_A, ENC_B) |
| **I2C thứ 2** | ❌ Không khả dụng | - | GPIO 21, 22 dùng cho Motor 2 Encoder |
| **Dự phòng** | 12 (strapping pin) | 1 chân | Dùng cẩn thận |
| **Tổng còn trống** | | **13 GPIO** | Đã phân bổ 12 cho motor, còn 1 dự phòng |

### 🎯 CẤU HÌNH ĐỀ XUẤT CUỐI CÙNG (Encoder + Motor L298N):
**Mỗi động cơ 4 chân: IN1, IN2, ENCODER_A, ENCODER_B**

```cpp
// ============================================
// ĐỘNG CƠ 1 - Cụm chân bên TRÁI board (Pin 24-21)
// ============================================
// Vị trí: Pin 24, 23, 22, 21 - GPIO 4, 0, 2, 15
#define MOTOR_1_IN1       4   // GPIO 4  - Pin 24 (OUTPUT PWM)
#define MOTOR_1_IN2       13  // GPIO 13 - Pin 20 (OUTPUT PWM)
#define MOTOR_1_ENCODER_A 14  // GPIO 14 - Pin 17 (INPUT)
#define MOTOR_1_ENCODER_B 32  // GPIO 32 - Pin 12 (INPUT, có ADC)

// ============================================
// ĐỘNG CƠ 2 - Cụm chân GIỮA board (Pin 35-38)
// ============================================
// Vị trí: Pin 35, 34, 33, 32 - GPIO 18, 5, 21, 19
#define MOTOR_2_IN1       18  // GPIO 18 - Pin 35 (OUTPUT PWM)
#define MOTOR_2_IN2       19  // GPIO 19 - Pin 38 (OUTPUT PWM)
#define MOTOR_2_ENCODER_A 21  // GPIO 21 - Pin 42 (INPUT)
#define MOTOR_2_ENCODER_B 22  // GPIO 22 - Pin 39 (INPUT)

// ============================================
// ĐỘNG CƠ 3 - Cụm chân bên PHẢI board (Pin 23-25)
// ============================================
// Vị trí: Pin 25, 24, 23, 22 - GPIO 16, 4, 0, 2
#define MOTOR_3_IN1       23  // GPIO 23 - Pin 23 (OUTPUT PWM)
#define MOTOR_3_IN2       33  // GPIO 33 - Pin 33 (OUTPUT PWM, có ADC)
#define MOTOR_3_ENCODER_A 36  // GPIO 36 - Pin 5  (INPUT-only, có ADC)
#define MOTOR_3_ENCODER_B 39  // GPIO 39 - Pin 8  (INPUT-only, có ADC)
```

**📍 Sơ đồ vị trí vật lý trên board (theo pinout ESP32 NodeMCU-32S):**

```
ĐỘNG CƠ 1 (Góc trái-dưới):          ĐỘNG CƠ 2 (Giữa board):             ĐỘNG CƠ 3 (Góc phải-trên):
Pin 24 → GPIO 4  → IN1              Pin 35 → GPIO 18 → IN1              Pin 23 → GPIO 23 → IN1
Pin 20 → GPIO 13 → IN2              Pin 38 → GPIO 19 → IN2              Pin 33 → GPIO 33 → IN2
Pin 17 → GPIO 14 → ENC_A            Pin 42 → GPIO 21 → ENC_A            Pin 5  → GPIO 36 → ENC_A
Pin 12 → GPIO 32 → ENC_B            Pin 39 → GPIO 22 → ENC_B            Pin 8  → GPIO 39 → ENC_B
```

**✅ Ưu điểm cấu hình này:**
- 🎯 **Tối ưu đi dây**: Mỗi động cơ dùng cụm chân gần nhau trên board
- ✅ **Encoder A/B** dùng GPIO input-only (36, 39) cho động cơ 3 - không lãng phí GPIO output
- ✅ **GPIO 32, 33** có ADC - có thể đọc current sensing cho động cơ 1 và 3
- ✅ **GPIO 21, 22** không xung đột I2C (đang dùng GPIO 16/17)
- ✅ Không có strapping pin quan trọng (GPIO 0, 12, 15 đã tránh)
- ✅ Tất cả chân đều an toàn khi boot
- 🔧 **Dễ troubleshoot**: Mỗi động cơ có nhóm chân riêng biệt

### 🔥 BẢNG TỔNG HỢP GPIO ĐẦY ĐỦ

| GPIO | Hiện tại | Mở rộng được | Ưu tiên |
|------|----------|--------------|---------|
| 0 | ❌ Tránh | Boot strapping | N/A |
| 1 | ❌ Tránh | TX0 Serial | N/A |
| 2 | ✅ Xilanh 1 | Đang dùng | HIGH |
| 3 | ❌ Tránh | RX0 Serial | N/A |
| 4 | 🟢 Trống | 🎯 MOTOR_1_IN1 | HIGH |
| 5 | ✅ LED WS2812 | Đang dùng | HIGH |
| 6-11 | ❌ Tránh | Flash SPI | N/A |
| 12 | 🟡 Trống | Strapping (dự phòng) | LOW |
| 13 | 🟢 Trống | 🎯 MOTOR_1_IN2 | HIGH |
| 14 | 🟢 Trống | 🎯 MOTOR_1_ENCODER_A | HIGH |
| 15 | ✅ Xilanh 2 | Đang dùng | HIGH |
| 16 | ✅ I2C SDA | Đang dùng | HIGH |
| 17 | ✅ I2C SCL | Đang dùng | HIGH |
| 18 | 🟢 Trống | 🎯 MOTOR_2_IN1 | HIGH |
| 19 | 🟢 Trống | 🎯 MOTOR_2_IN2 | HIGH |
| 21 | 🟢 Trống | 🎯 MOTOR_2_ENCODER_A | HIGH |
| 22 | 🟢 Trống | 🎯 MOTOR_2_ENCODER_B | HIGH |
| 23 | 🟢 Trống | 🎯 MOTOR_3_IN1 | HIGH |
| 25 | ✅ Button Next | Đang dùng | HIGH |
| 26 | ✅ Button Sync | Đang dùng | HIGH |
| 27 | ✅ Button HDMI | Đang dùng | HIGH |
| 32 | 🟢 Trống | 🎯 MOTOR_1_ENCODER_B (ADC) | HIGH |
| 33 | 🟢 Trống | 🎯 MOTOR_3_IN2 (ADC) | HIGH |
| 34 | ✅ IR Sensor 1 | Đang dùng (Input-only) | HIGH |
| 35 | ✅ IR Sensor 2 | Đang dùng (Input-only) | HIGH |
| 36 | 🟢 Trống | 🎯 MOTOR_3_ENCODER_A (Input-only) | HIGH |
| 39 | 🟢 Trống | 🎯 MOTOR_3_ENCODER_B (Input-only) | HIGH |

**Legend:**
- ✅ Đang dùng (10 GPIO)
- 🟢 Trống - khuyến nghị dùng (11 GPIO)
- 🟡 Trống - dùng cẩn thận (1 GPIO)
- ❌ Tránh dùng (system reserved)

### 📝 Lưu ý quan trọng:
- ⚠️ **Tránh GPIO 0, 1, 3, 6-11** (system reserved, boot strapping, Flash SPI)
- ✅ **GPIO Input-only (34, 35, 36, 39)** - chỉ đọc, không xuất tín hiệu
- ✅ **GPIO 32, 33** có ADC - có thể đọc analog (current sensing cho motor)
- ⚠️ **GPIO 12** là strapping pin - tránh pull HIGH khi boot
- ✅ **I2C đang dùng GPIO 16/17** - không xung đột với GPIO 21/22 (I2C default)
