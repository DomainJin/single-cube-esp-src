# 📌 ESP32 GPIO PINOUT - CỤM VẬT LÝ CHO LAYOUT PCB

## ✅ TỔNG QUAN

**ESP32 NodeMCU-32S: 22/34 chân khả dụng**

### 🔧 Sơ đồ điều khiển Motor/Xilanh (2 chân):
```
ESP32 DIR → L298N IN1
ESP32 DIR → NOT gate (74HC04) → L298N IN2  
ESP32 EN  → L298N EN (PWM 5kHz)

DIR=HIGH: IN1=HIGH, IN2=LOW → Tiến/Lên
DIR=LOW:  IN1=LOW, IN2=HIGH → Lùi/Xuống
```

---

## 📍 CỤM BÊN TRÁI (LEFT SIDE) - 11 GPIO

### 🔵 BLOCK ENCODERS (GPIO 12-14, 25-27)
| GPIO | Chức năng | Loại | Ghi chú |
|------|-----------|------|---------|
| **12** | Motor 1 Encoder A | INPUT | ⚠️ Strapping pin (boot mode) |
| **13** | Motor 1 Encoder B | INPUT | - |
| **14** | Motor 2 Encoder A | INPUT | - |
| **25** | Motor 2 Encoder B | INPUT | - |
| **26** | Motor 3 Encoder A | INPUT | - |
| **27** | Motor 3 Encoder B | INPUT | - |

**Yêu cầu phần cứng:**
- Pull-up 4.7kΩ mỗi chân encoder (ESP32 internal ~45kΩ yếu)
- RC filter: 1kΩ + 47nF (cutoff ~3.4kHz) mỗi chân
- Level shifter 5V→3.3V: Zener clamp (1kΩ + MM3Z3V3) hoặc TXS0108E
- Bypass capacitor 10µF gần ESP32

---

### 🟠 BLOCK A4L RELAYS (GPIO 32-33, 23)
| GPIO | Chức năng | Loại | Ghi chú |
|------|-----------|------|---------|
| **32** | A4L Next Button | OUTPUT | Active LOW (relay đảo) |
| **33** | A4L Sync Mode | OUTPUT | Active LOW |
| **23** | A4L HDMI Mode | OUTPUT | Active LOW (bên phải) |

**Yêu cầu phần cứng:**
- Relay module active LOW (kích khi GPIO=LOW)
- Pull-up resistor trên relay input nếu cần

---

### 🟢 BLOCK IR SENSORS (GPIO 34-35-36)
| GPIO | Chức năng | Loại | Ghi chú |
|------|-----------|------|---------|
| **34** | IR Sensor 1 (ADC1_CH6) | INPUT-ONLY | 12-bit ADC |
| **35** | IR Sensor 2 (ADC1_CH7) | INPUT-ONLY | 12-bit ADC |
| **36** | IR Sensor 3 (ADC1_CH0) | INPUT-ONLY | Tùy chọn nếu cần sensor thứ 3 |

**Yêu cầu phần cứng:**
- IR sensor analog output 0-3.3V
- Median filter trong code (noise rejection)

---

## 📍 CỤM BÊN PHẢI (RIGHT SIDE) - 11 GPIO

### 🔴 BLOCK MOTORS (GPIO 2,4,5,16,17,18)
| GPIO | Chức năng | Loại | PWM | Ghi chú |
|------|-----------|------|-----|---------|
| **2** | Motor 1 EN | PWM | CH0 | ⚠️ Strapping pin (LOW boot) |
| **4** | Motor 1 DIR | OUTPUT | - | DIR → IN1, NOT → IN2 |
| **5** | Motor 2 EN | PWM | CH1 | ⚠️ Strapping pin (HIGH boot) |
| **16** | Motor 2 DIR | OUTPUT | - | - |
| **17** | Motor 3 EN | PWM | CH2 | - |
| **18** | Motor 3 DIR | OUTPUT | - | - |

**Yêu cầu phần cứng:**
- L298N driver module (6-12V motor supply)
- NOT gate 74HC04 hoặc transistor inverter cho mỗi DIR → IN2
- PWM 5kHz, 8-bit resolution (0-255)
- Voltage drop compensation: PWM 60-255 effective range

---

### 🟣 BLOCK XILANH (GPIO 19,21)
| GPIO | Chức năng | Loại | PWM | Ghi chú |
|------|-----------|------|-----|---------|
| **19** | Xilanh EN | PWM | CH3 | - |
| **21** | Xilanh DIR | OUTPUT | - | HIGH=Lên, LOW=Xuống |

**Yêu cầu phần cứng:**
- L298N driver (tương tự motor)
- NOT gate cho DIR → IN2
- PWM 5kHz, 8-bit resolution

---

### 🟡 BLOCK I2C (GPIO 15,22)
| GPIO | Chức năng | Loại | Ghi chú |
|------|-----------|------|---------|
| **15** | I2C SDA | I2C | ⚠️ Strapping pin (LOW boot) |
| **22** | I2C SCL | I2C | - |

**Yêu cầu phần cứng:**
- Pull-up 4.7kΩ trên SDA và SCL (nếu chưa có trên module I2C)
- `Wire.begin(15, 22);` trong code
- Clock speed: 100kHz standard mode

---

### 🔵 BLOCK MISC (GPIO 0)
| GPIO | Chức năng | Loại | Ghi chú |
|------|-----------|------|---------|
| **0** | LED WS2812 Data | OUTPUT | ⚠️ Strapping pin (boot mode) |

**Yêu cầu phần cứng:**
- WS2812 LED strip (3.5-5V data, level shifter nếu cần)
- 100-470Ω resistor giữa GPIO 0 và LED data pin
- 1000µF capacitor trên nguồn LED

---

## 🎯 BẢNG TỔNG HỢP THEO CHỨC NĂNG

| **Chức năng** | **GPIO** | **Số chân** | **PWM** | **Note** |
|--------------|----------|-------------|---------|----------|
| **Motor 1** | EN=2, DIR=4 | 2 | CH0 | 2-pin control |
| **Motor 1 Encoder** | A=12, B=13 | 2 | - | 4.7kΩ pull-up + filter |
| **Motor 2** | EN=5, DIR=16 | 2 | CH1 | 2-pin control |
| **Motor 2 Encoder** | A=14, B=25 | 2 | - | 4.7kΩ pull-up + filter |
| **Motor 3** | EN=17, DIR=18 | 2 | CH2 | 2-pin control |
| **Motor 3 Encoder** | A=26, B=27 | 2 | - | 4.7kΩ pull-up + filter |
| **Xilanh** | EN=19, DIR=21 | 2 | CH3 | 2-pin control |
| **A4L Relay** | 32, 33, 23 | 3 | - | Active LOW |
| **IR Sensors** | 34, 35, (36) | 2-3 | - | Input-only GPIO |
| **I2C** | SDA=15, SCL=22 | 2 | - | 4.7kΩ pull-up |
| **LED** | 0 | 1 | - | WS2812 data |
| **TOTAL** | - | **20-21** | 4 PWM | 1-2 chân còn trống |

---

## ⚠️ LƯU Ý QUAN TRỌNG

### 🔸 Strapping Pins (Boot Mode):
| GPIO | Boot Mode | Sử dụng hiện tại | An toàn? |
|------|-----------|------------------|----------|
| **0** | LOW = Boot | LED WS2812 | ✅ OK (output, LOW sau boot) |
| **2** | LOW = Boot | Motor 1 EN | ✅ OK (PWM=0 khi boot) |
| **5** | HIGH = Boot | Motor 2 EN | ✅ OK (PWM=0 khi boot) |
| **12** | Boot voltage | Motor 1 Enc A | ⚠️ Cẩn thận (nếu HIGH khi boot có thể lỗi flash) |
| **15** | LOW = Boot | I2C SDA | ✅ OK (output, LOW sau boot) |

**Khuyến nghị:** Không kéo GPIO 12 HIGH khi reset/power-on

---

### 🔸 Input-Only Pins (Không thể OUTPUT):
- **GPIO 34, 35, 36, 37, 38, 39**: Chỉ dùng cho INPUT (ADC, encoder, sensor)
- Không được gán cho motor control, relay, LED

---

### 🔸 Reserved Pins (Không bao giờ dùng):
- **GPIO 6-11**: Flash SPI (xung đột với bộ nhớ)
- **GPIO 1, 3**: UART TX/RX (Serial Monitor)

---

## 🔧 DANH SÁCH LINH KIỆN PHẦN CỨNG

### Encoder Filter Circuit (6 bộ cho 6 chân encoder):
```
Encoder 5V → [4.7kΩ pull-up] → [470Ω series] → [1kΩ + 47nF RC filter] → ESP32 GPIO
                                                 [MM3Z3V3 zener to GND]
```

**Linh kiện cho 1 encoder (2 chân A,B):**
- 2× Resistor 4.7kΩ (pull-up)
- 2× Resistor 1kΩ (RC filter)
- 2× Capacitor 47nF ceramic (RC filter)
- 2× Zener diode MM3Z3V3 3.3V (clamp protection)
- 1× Capacitor 10µF electrolytic (bypass cho encoder power)

**Tổng cho 3 encoder:**
- 6× 4.7kΩ, 6× 1kΩ, 6× 47nF, 6× MM3Z3V3, 3× 10µF

---

### Motor/Xilanh Driver:
- 4× L298N module (3 motor + 1 xilanh)
- 4× 74HC04 hex inverter IC (hoặc 8× NPN transistor 2N2222 cho DIR inverter)
- 12V power supply (motor/xilanh)
- Heatsink cho L298N

---

### I2C & LED:
- 2× 4.7kΩ resistor (I2C pull-up)
- 1× 470Ω resistor (LED data series)
- 1× 1000µF capacitor (LED power smoothing)
- WS2812 LED strip

---

## 🛠️ CẤU HÌNH CODE

### Header Files Updated:
- ✅ [motor.h](include/motor.h) - 2-pin control (EN+DIR)
- ✅ [xilanh.h](include/xilanh.h) - 2-pin control (EN+DIR)
- ✅ [IR.h](include/IR.h) - GPIO 34,35,36
- ✅ [a4l.h](include/a4l.h) - GPIO 32,33,23
- ✅ [led.h](include/led.h) - GPIO 0
- ✅ [i2c_config.h](include/i2c_config.h) - GPIO 15,22

### Source Files Updated:
- ✅ [motor.cpp](src/motor.cpp) - Chỉ dùng pin_dir (không còn pin_in1/in2)
- ✅ [xilanh.cpp](src/xilanh.cpp) - Điều khiển EN+DIR
- ✅ [main.h](include/main.h) - Include xilanh.h và i2c_config.h

---

## 📐 SƠ ĐỒ KẾT NỐI L298N (Motor/Xilanh)

```
┌─────────────┐              ┌──────────────────┐
│  ESP32      │              │   L298N Driver   │
│             │              │                  │
│  GPIO EN ───┼─────────────→│ ENA (PWM)        │
│             │              │                  │
│  GPIO DIR ──┼─────┬───────→│ IN1              │
│             │     │        │                  │
└─────────────┘     │        │                  │
                    │        │                  │
              ┌─────┴────┐   │                  │
              │ NOT gate │   │                  │
              │ (74HC04) ├──→│ IN2              │
              └──────────┘   │                  │
                             │                  │
                             │  OUT1 ─────┬─────→ Motor/Xilanh
                             │            │
                             │  OUT2 ─────┘
                             └──────────────────┘
```

**Ưu điểm:**
- Chỉ cần 2 GPIO/motor (tiết kiệm 1 GPIO)
- Tự động đảo logic DIR → IN2 (không lo sai hướng)
- Đơn giản hóa PCB layout (ít trace)

**Nhược điểm:**
- Cần thêm 1 NOT gate IC (74HC04) hoặc transistor inverter
- Không thể brake bằng IN1=IN2=HIGH (dùng EN=255 để brake)

---

## 🔍 KIỂM TRA SAU KHI LẮP RÁP

1. **Test GPIO OUTPUT (motor, xilanh, relay):**
   ```cpp
   pinMode(GPIO_X, OUTPUT);
   digitalWrite(GPIO_X, HIGH);
   delay(1000);
   digitalWrite(GPIO_X, LOW);
   ```

2. **Test Encoder INPUT:**
   ```cpp
   pinMode(ENC_A, INPUT_PULLUP);
   Serial.println(digitalRead(ENC_A)); // Phải đọc được 0 hoặc 1
   ```

3. **Test PWM:**
   ```cpp
   ledcSetup(0, 5000, 8);
   ledcAttachPin(MOTOR_EN, 0);
   ledcWrite(0, 128); // 50% duty cycle
   ```

4. **Test I2C:**
   ```cpp
   Wire.begin(15, 22);
   Wire.beginTransmission(0x68); // MPU6050 address
   byte error = Wire.endTransmission();
   if(error == 0) Serial.println("I2C OK");
   ```

---

## 📊 GIỚI HẠN PHẦN CỨNG

- **PWM Channels**: 16 channels available, dùng 4 (motor×3 + xilanh×1)
- **ADC1**: 8 channels (GPIO 32-39), dùng 2-3 (IR sensors)
- **ADC2**: 10 channels (GPIO 0,2,4,12-15,25-27), KHÔNG DÙNG được khi WiFi ON
- **GPIO Total Usable**: 22/34 pins
- **GPIO Used**: 20-21 pins (1-2 pins còn trống)

---

**✅ HOÀN THÀNH! Tất cả GPIO đã được sắp xếp theo cụm vật lý để dễ layout PCB.**
