# ESP32 GPIO Usage Summary - Single Cube Project
## Board: ESP32 NodeMCU-32S (ESP-32S Kit)
**Generated:** December 6, 2025

---

## 📊 TỔNG QUAN

### ✅ **TỔNG CỘNG: 22 GPIO đang sử dụng**

| Danh mục | Số lượng GPIO | Ghi chú |
|----------|---------------|---------|
| **Hệ thống hiện tại** | 10 GPIO | I2C, LED, IR, Buttons, Xilanh |
| **Motor Control mới** | 12 GPIO | 3 Motors × 4 pins (IN1, IN2, ENC_A, ENC_B) |
| **GPIO còn trống** | ~11 GPIO | Hầu hết đã dùng hết |
| **GPIO tránh dùng** | 5 GPIO | GPIO 0, 1, 3, 6-11 (system reserved) |

---

## 🔧 HỆ THỐNG HIỆN TẠI (10 GPIO)

### I2C Communication - QMC5883L Compass
| GPIO | Chức năng | Module | Loại | File |
|------|-----------|--------|------|------|
| **16** | I2C SDA | QMC5883L | I2C | `qmc5883l.cpp` |
| **17** | I2C SCL | QMC5883L | I2C | `qmc5883l.cpp` |

**Thông số:**
- Tốc độ: 100kHz
- Địa chỉ I2C: 0x0D
- Chức năng: Đo heading/hướng (0-360°), gửi UDP: `COMPASS:heading,offset,direction`

---

### LED Control - WS2812 RGB Strip
| GPIO | Chức năng | Module | Loại | File |
|------|-----------|--------|------|------|
| **5** | WS2812 Data | LED Strip | OUTPUT | `led.h` |

**Thông số:**
- Type: WS2812B addressable LED
- Protocol: Single-wire serial
- Chức năng: Touch effect, rainbow, custom colors

---

### IR Sensors - Proximity Detection
| GPIO | Chức năng | Module | Loại | File |
|------|-----------|--------|------|------|
| **34** | IR Sensor 1 | IR Module | ADC INPUT | `IR.h` |
| **35** | IR Sensor 2 | IR Module | ADC INPUT | `IR.h` |

**Thông số:**
- ADC Resolution: 12-bit (0-4095)
- Chức năng: Swipe detection (UP/DOWN/TOUCH)
- Threshold: Dynamic, 100-count change detection
- UDP: `IR_ADC_1`, `IR_ADC_2`, `FACE_X:status`

---

### A4L Control Buttons
| GPIO | Chức năng | Module | Loại | File |
|------|-----------|--------|------|------|
| **25** | Button Next | A4L Controller | INPUT | `a4l.h` |
| **26** | Button Sync | A4L Controller | INPUT | `a4l.h` |
| **27** | Button HDMI | A4L Controller | INPUT | `a4l.h` |

**Thông số:**
- Type: Digital input with pull-up
- Chức năng: External device control via GPIO output

---

### Xilanh Control - Pneumatic Actuators
| GPIO | Chức năng | Module | Loại | File |
|------|-----------|--------|------|------|
| **2** | Xilanh Output 1 | Main | OUTPUT | `main.cpp` |
| **15** | Xilanh Output 2 | Main | OUTPUT | `main.cpp` |

**Thông số:**
- Control: HIGH/LOW digital output
- UDP Command: `XILANH:0` (STOP), `XILANH:1` (DOWN), `XILANH:2` (UP)

---

## ⚡ MOTOR CONTROL SYSTEM (12 GPIO mới thêm)

### 🎯 Cấu hình: Option 2 - 3 Motors + Encoders (L298N Driver)

---

### Motor 1 - Động cơ 1 (Cụm trái)
| GPIO | Chức năng | Loại | PWM Channel | File |
|------|-----------|------|-------------|------|
| **4** | Motor 1 IN1 | PWM OUTPUT | Channel 0 | `motor.h` |
| **13** | Motor 1 IN2 | PWM OUTPUT | Channel 1 | `motor.h` |
| **14** | Motor 1 Encoder A | INPUT | Interrupt | `motor.h` |
| **32** | Motor 1 Encoder B | INPUT + ADC | - | `motor.h` |

**Thông số:**
- Driver: L298N H-Bridge
- PWM: 5kHz, 8-bit (0-255)
- Encoder: 20 PPR (configurable)
- Current sensing: GPIO 32 có ADC

---

### Motor 2 - Động cơ 2 (Cụm giữa)
| GPIO | Chức năng | Loại | PWM Channel | File |
|------|-----------|------|-------------|------|
| **18** | Motor 2 IN1 | PWM OUTPUT | Channel 2 | `motor.h` |
| **19** | Motor 2 IN2 | PWM OUTPUT | Channel 3 | `motor.h` |
| **21** | Motor 2 Encoder A | INPUT | Interrupt | `motor.h` |
| **22** | Motor 2 Encoder B | INPUT | - | `motor.h` |

**Thông số:**
- Driver: L298N H-Bridge
- PWM: 5kHz, 8-bit (0-255)
- Encoder: 20 PPR (configurable)
- GPIO 21, 22: I2C default pins (không xung đột vì I2C dùng 16/17)

---

### Motor 3 - Động cơ 3 (Cụm phải)
| GPIO | Chức năng | Loại | PWM Channel | File |
|------|-----------|------|-------------|------|
| **23** | Motor 3 IN1 | PWM OUTPUT | Channel 4 | `motor.h` |
| **33** | Motor 3 IN2 | PWM OUTPUT + ADC | Channel 5 | `motor.h` |
| **36** | Motor 3 Encoder A | INPUT-only + ADC | Interrupt | `motor.h` |
| **39** | Motor 3 Encoder B | INPUT-only + ADC | - | `motor.h` |

**Thông số:**
- Driver: L298N H-Bridge
- PWM: 5kHz, 8-bit (0-255)
- Encoder: 20 PPR (configurable)
- Current sensing: GPIO 33 có ADC
- GPIO 36, 39: Input-only (tối ưu cho encoder)

---

## 📋 DANH SÁCH GPIO THEO THỨ TỰ

```
╔═══════╦═══════════════════════════╦═══════════╦═════════════════════════╗
║ GPIO  ║ Chức năng                 ║ Loại      ║ Module                  ║
╠═══════╬═══════════════════════════╬═══════════╬═════════════════════════╣
║   2   ║ Xilanh Output 1           ║ OUTPUT    ║ Main (Pneumatic)        ║
║   4   ║ Motor 1 IN1               ║ PWM OUT   ║ Motor Control (L298N)   ║
║   5   ║ WS2812 LED Data           ║ OUTPUT    ║ LED Strip               ║
║  13   ║ Motor 1 IN2               ║ PWM OUT   ║ Motor Control (L298N)   ║
║  14   ║ Motor 1 Encoder A         ║ INPUT     ║ Motor Control (Encoder) ║
║  15   ║ Xilanh Output 2           ║ OUTPUT    ║ Main (Pneumatic)        ║
║  16   ║ I2C SDA                   ║ I2C       ║ QMC5883L Compass        ║
║  17   ║ I2C SCL                   ║ I2C       ║ QMC5883L Compass        ║
║  18   ║ Motor 2 IN1               ║ PWM OUT   ║ Motor Control (L298N)   ║
║  19   ║ Motor 2 IN2               ║ PWM OUT   ║ Motor Control (L298N)   ║
║  21   ║ Motor 2 Encoder A         ║ INPUT     ║ Motor Control (Encoder) ║
║  22   ║ Motor 2 Encoder B         ║ INPUT     ║ Motor Control (Encoder) ║
║  23   ║ Motor 3 IN1               ║ PWM OUT   ║ Motor Control (L298N)   ║
║  25   ║ Button Next               ║ INPUT     ║ A4L Controller          ║
║  26   ║ Button Sync               ║ INPUT     ║ A4L Controller          ║
║  27   ║ Button HDMI               ║ INPUT     ║ A4L Controller          ║
║  32   ║ Motor 1 Encoder B (ADC)   ║ INPUT+ADC ║ Motor Control (Encoder) ║
║  33   ║ Motor 3 IN2 (ADC)         ║ PWM+ADC   ║ Motor Control (L298N)   ║
║  34   ║ IR Sensor 1               ║ ADC IN    ║ IR Proximity (Input)    ║
║  35   ║ IR Sensor 2               ║ ADC IN    ║ IR Proximity (Input)    ║
║  36   ║ Motor 3 Encoder A (ADC)   ║ IN+ADC    ║ Motor Control (Encoder) ║
║  39   ║ Motor 3 Encoder B (ADC)   ║ IN+ADC    ║ Motor Control (Encoder) ║
╚═══════╩═══════════════════════════╩═══════════╩═════════════════════════╝
```

---

## 🎯 PHÂN LOẠI THEO CHỨC NĂNG

### PWM Output (6 GPIO)
- **GPIO 4, 13** - Motor 1 (IN1, IN2)
- **GPIO 18, 19** - Motor 2 (IN1, IN2)
- **GPIO 23, 33** - Motor 3 (IN1, IN2)

**Cấu hình PWM:**
- Frequency: 5kHz
- Resolution: 8-bit (0-255)
- Channels: 0-5 (ESP32 có 16 channels)

---

### Digital Input - Encoders (6 GPIO)
- **GPIO 14, 32** - Motor 1 Encoder (A, B)
- **GPIO 21, 22** - Motor 2 Encoder (A, B)
- **GPIO 36, 39** - Motor 3 Encoder (A, B)

**Encoder Configuration:**
- PPR: 20 (Pulses Per Revolution)
- Debounce: 2ms
- Interrupt: RISING edge on pin A
- Direction: Determined by pin B state

---

### Digital Input - Buttons (3 GPIO)
- **GPIO 25** - Button Next
- **GPIO 26** - Button Sync
- **GPIO 27** - Button HDMI

**Configuration:**
- Pull-up: Internal
- Debounce: Software

---

### ADC Input-only (4 GPIO)
- **GPIO 34, 35** - IR Sensors
- **GPIO 36, 39** - Motor 3 Encoder (Input-only pins)

**ADC Configuration:**
- Resolution: 12-bit (0-4095)
- Voltage range: 0-3.3V
- Attenuation: 11dB (0-3.6V)

---

### Digital Output (3 GPIO)
- **GPIO 2, 15** - Xilanh Control
- **GPIO 5** - WS2812 LED Data

---

### I2C Communication (2 GPIO)
- **GPIO 16** - SDA (QMC5883L)
- **GPIO 17** - SCL (QMC5883L)

**I2C Configuration:**
- Speed: 100kHz
- Device: QMC5883L Magnetometer (0x0D)

---

## ⚠️ GPIO CÒN TRỐNG VÀ TRÁNH DÙNG

### GPIO Còn Trống (11 GPIO)
| GPIO | Đặc điểm | Khuyến nghị | Trạng thái |
|------|----------|-------------|------------|
| **12** | Strapping pin | Dự phòng | ⚠️ Tránh HIGH khi boot |
| **20** | - | Không tồn tại | ❌ Không có GPIO 20 |
| **24** | - | Không tồn tại | ❌ Không có GPIO 24 |
| **28-31** | - | Không tồn tại | ❌ Không có GPIO 28-31 |
| **37, 38** | - | Không tồn tại | ❌ Không có GPIO 37, 38 |

### GPIO Tránh Dùng (System Reserved)
| GPIO | Lý do | Trạng thái |
|------|-------|------------|
| **0** | Strapping pin (Boot mode) | ❌ Phải LOW khi boot để programming |
| **1** | TX0 (USB Serial) | ❌ Dùng cho Serial Monitor |
| **3** | RX0 (USB Serial) | ❌ Dùng cho Serial Monitor |
| **6-11** | Flash SPI | ❌ KHÔNG BAO GIỜ dùng (kết nối Flash) |
| **12** | Strapping pin | ⚠️ Flash voltage select (3.3V if LOW) |
| **15** | Strapping pin | ⚠️ Boot debug mode (đang dùng - OK) |

---

## 📊 THỐNG KÊ SỬ DỤNG

```
╔═══════════════════════════════╦═════════╦═══════════╗
║ Loại GPIO                     ║ Số GPIO ║ Tỷ lệ %   ║
╠═══════════════════════════════╬═════════╬═══════════╣
║ PWM Output                    ║    6    ║   27.3%   ║
║ Digital Input (Encoder/Button)║    9    ║   40.9%   ║
║ ADC Input-only                ║    4    ║   18.2%   ║
║ Digital Output                ║    3    ║   13.6%   ║
║ I2C                          ║    2    ║    9.1%   ║
╠═══════════════════════════════╬═════════╬═══════════╣
║ TỔNG CỘNG                    ║   22    ║  100.0%   ║
╚═══════════════════════════════╩═════════╩═══════════╝
```

**GPIO Utilization:**
- Đã dùng: 22 / 34 GPIO khả dụng = **64.7%**
- System reserved: 5 GPIO (0, 1, 3, 6-11)
- Còn trống an toàn: ~7 GPIO

---

## 🔌 SƠ ĐỒ KẾT NỐI TỔNG QUAN

```
                          ┌─────────────────────┐
                          │   ESP32 NodeMCU-32S │
                          │    (ESP-32S Kit)    │
                          └──────────┬──────────┘
                                     │
          ┌──────────────────────────┼──────────────────────────┐
          │                          │                          │
    ┌─────▼─────┐            ┌──────▼──────┐          ┌───────▼──────┐
    │  MOTOR 1  │            │   MOTOR 2   │          │   MOTOR 3    │
    │  (L298N)  │            │   (L298N)   │          │   (L298N)    │
    ├───────────┤            ├─────────────┤          ├──────────────┤
    │ IN1:  4   │            │ IN1:  18    │          │ IN1:  23     │
    │ IN2: 13   │            │ IN2:  19    │          │ IN2:  33(ADC)│
    │ ENC_A: 14 │            │ ENC_A: 21   │          │ ENC_A: 36(IN)│
    │ ENC_B: 32 │            │ ENC_B: 22   │          │ ENC_B: 39(IN)│
    └───────────┘            └─────────────┘          └──────────────┘

    ┌───────────┐            ┌─────────────┐          ┌──────────────┐
    │ QMC5883L  │            │  WS2812 LED │          │  IR Sensors  │
    │  Compass  │            │    Strip    │          │  (Proximity) │
    ├───────────┤            ├─────────────┤          ├──────────────┤
    │ SDA: 16   │            │ Data: 5     │          │ ADC1: 34     │
    │ SCL: 17   │            │             │          │ ADC2: 35     │
    └───────────┘            └─────────────┘          └──────────────┘

    ┌───────────┐            ┌─────────────┐          ┌──────────────┐
    │ A4L Ctrl  │            │   Xilanh    │          │     WiFi     │
    │  Buttons  │            │  Actuators  │          │     UDP      │
    ├───────────┤            ├─────────────┤          ├──────────────┤
    │ Next: 25  │            │ OUT1: 2     │          │ Server IP:   │
    │ Sync: 26  │            │ OUT2: 15    │          │ 192.168.0.202│
    │ HDMI: 27  │            │             │          │ Port: 1509   │
    └───────────┘            └─────────────┘          └──────────────┘
```

---

## 📝 NOTES & RECOMMENDATIONS

### ✅ Ưu điểm thiết kế hiện tại:
1. **GPIO 36, 39** (Input-only) dùng cho Motor 3 Encoder - tối ưu, không lãng phí GPIO output
2. **GPIO 32, 33** có ADC - có thể đọc current sensing cho Motor 1 và 3
3. **GPIO 21, 22** không xung đột I2C (I2C đang dùng GPIO 16/17)
4. Encoder interrupt trên RISING edge - phản ứng nhanh
5. PWM 5kHz phù hợp với L298N driver
6. UDP priority queue tránh spam và loss packet

### ⚠️ Lưu ý quan trọng:
1. **GPIO 12** là strapping pin - tránh pull HIGH khi boot
2. **GPIO 2, 15** đang dùng cho Xilanh (strapping pins - hiện tại OK vì LOW)
3. **Encoder PPR = 20** - cần thay đổi trong `motor.h` nếu encoder khác
4. **Current sensing** trên GPIO 32, 33 - cần thêm mạch chia áp nếu dùng
5. **WiFi sleep mode = OFF** để giảm latency UDP

### 🔧 Khuyến nghị mở rộng:
- **GPIO còn trống:** ~7 GPIO (hầu hết đã dùng)
- Nếu cần thêm sensor: dùng I2C bus (GPIO 16/17) để tiết kiệm GPIO
- Nếu cần thêm LED: có thể dùng WS2812 chain (1 GPIO điều khiển nhiều LED)
- Nếu cần thêm motor: đã hết GPIO output, cần dùng I2C expander hoặc shift register

---

## 📚 REFERENCE FILES

### Header Files (include/)
- `motor.h` - Motor control definitions (12 GPIO)
- `led.h` - LED control (1 GPIO)
- `IR.h` - IR sensor (2 GPIO)
- `a4l.h` - Button control (3 GPIO)
- `qmc5883l.h` - I2C compass (2 GPIO)

### Source Files (src/)
- `motor.cpp` - Motor control implementation
- `main.cpp` - Main program, Xilanh control (2 GPIO)
- `led.cpp` - LED effects
- `IR.cpp` - IR sensor processing
- `qmc5883l.cpp` - Compass communication
- `udpconfig.cpp` - UDP messaging

### Documentation
- `PIN_MAPPING.md` - Detailed pin mapping
- `MOTOR_USAGE_EXAMPLES.cpp` - Motor control examples
- `generate_schematic.py` - Schematic generator

---

## 🔄 UPDATE HISTORY

| Date | Version | Changes |
|------|---------|---------|
| 2025-12-06 | 1.0 | Initial GPIO summary - 22 GPIO in use |
| 2025-12-06 | 1.0 | Added Motor Control System (12 GPIO) |
| 2025-12-06 | 1.0 | Verified all pin assignments with actual code |

---

**Generated by:** ESP32 Single Cube Project  
**Board:** ESP32 NodeMCU-32S (ESP-32S Kit)  
**Total GPIO Used:** 22 / 34 available (64.7%)  
**Status:** ✅ All GPIO verified and documented
