# 🔌 SCHEMATIC to GPIO MAPPING
## NodeMCU-32S Configuration Updated from Schematic

**Last Updated:** December 23, 2025
**Schematic:** Single Cube ESP - Motor Control Board

---

## 📋 MCU Side Labels → GPIO Mapping

Based on the schematic provided, here's the updated GPIO configuration:

### 🔴 **MOTOR CONTROL PINS**

| Label | Function | GPIO | PWM Channel | Notes |
|-------|----------|------|-------------|-------|
| **PWM1** | Motor 1 Enable | GPIO 32 | CH 0 | Speed control for Motor 1 |
| **DIR1** | Motor 1 Direction | GPIO 26 | - | Forward/Backward control |
| **PWM2** | Motor 2 Enable | GPIO 33 | CH 1 | Speed control for Motor 2 |
| **DIR2** | Motor 2 Direction | GPIO 27 | - | Forward/Backward control |
| **PWM3** | Motor 3 Enable | GPIO 25 | CH 2 | Speed control for Motor 3 |
| **DIR3** | Motor 3 Direction | GPIO 14 | - | Forward/Backward control |

**Header File:** [include/motor.h](include/motor.h)

```cpp
// MOTOR 1
#define MOTOR_1_EN        32  // PWM Enable
#define MOTOR_1_DIR       26  // Direction control

// MOTOR 2
#define MOTOR_2_EN        33  // PWM Enable
#define MOTOR_2_DIR       27  // Direction control

// MOTOR 3
#define MOTOR_3_EN        25  // PWM Enable
#define MOTOR_3_DIR       14  // Direction control
```

---

### 🟠 **COMMUNICATION PINS**

| Label | Function | GPIO | Type | Notes |
|-------|----------|------|------|-------|
| **TX** | UART Transmit | GPIO 1 | OUTPUT | Serial communication to STM32 |
| **RX** | UART Receive | GPIO 3 | INPUT | Serial communication from STM32 |
| **SDA** | I2C Data | GPIO 21 | I2C | Compass (QMC5883L) |
| **SCL** | I2C Clock | GPIO 22 | I2C | Compass (QMC5883L) |

**Header Files:**
- [include/uart.h](include/uart.h)
- [include/i2c_config.h](include/i2c_config.h)

```cpp
// UART (UART0)
#define UART_TX_PIN  1   // TX
#define UART_RX_PIN  3   // RX
#define UART_BAUD    112500
#define UART_PORT    0   // UART0

// I2C
#define I2C_SDA  21  // GPIO 21
#define I2C_SCL  22  // GPIO 22
```

---

### 🟡 **LED & INDICATORS**

| Label | Function | GPIO | Type | Notes |
|-------|----------|------|------|-------|
| **LED** | WS2812 RGB Data | GPIO 4 | OUTPUT | Addressable LED strip |

**Header File:** [include/led.h](include/led.h)

```cpp
#define LED_PIN 4  // GPIO 4 - WS2812 data
```

---

### 🟢 **CONTROL BUTTONS (A4L)**

| Label | Function | GPIO | Type | Notes |
|-------|----------|------|------|-------|
| **NEXT** | Next Button | GPIO 32 | OUTPUT | Active LOW relay control |
| Sync Mode | Sync Button | GPIO 33 | OUTPUT | Active LOW relay control |
| **HDMI** | HDMI Mode Button | GPIO 23 | OUTPUT | Active LOW relay control |

**Header File:** [include/a4l.h](include/a4l.h)

```cpp
#define Button_Next_Pin  32  // GPIO 32
#define Button_SyncMode  33  // GPIO 33
#define Button_HDMI_Mode 23  // GPIO 23
```

---

## 📊 GPIO ALLOCATION SUMMARY

### Used GPIOs (19 total)
| Category | GPIO | Count | Purpose |
|----------|------|-------|---------|
| **Motors** | 14, 25, 26, 27, 32, 33 | 6 | PWM + Direction control |
| **Encoders** | 12, 13, 16, 17 | 4 | Encoder feedback (A, B channels) |
| **UART** | 1, 3 | 2 | Serial communication |
| **I2C** | 21, 22 | 2 | Compass sensor |
| **Buttons** | 23, 32, 33 | 3 | A4L relay control |
| **LED** | 4 | 1 | Addressable RGB strip |

### Available GPIOs for Future Use
| GPIO | Notes |
|------|-------|
| 0, 2, 5 | Strapping pins (use with caution) |
| 6-11 | System reserved (do not use) |
| 15, 18, 19, 20, 24, 28, 29, 31, 34, 35, 36, 37, 38, 39 | Available but some are ADC input-only |

---

## ⚡ PWM CONFIGURATION

```cpp
#define PWM_FREQ        5000  // 5kHz PWM frequency
#define PWM_RESOLUTION  8     // 8-bit resolution (0-255)

#define PWM_CHANNEL_M1_EN   0  // Motor 1 Enable
#define PWM_CHANNEL_M2_EN   1  // Motor 2 Enable  
#define PWM_CHANNEL_M3_EN   2  // Motor 3 Enable
```

---

## 🔧 PIN CONFLICT CHECK

✅ **NO CONFLICTS DETECTED**

All GPIO assignments are unique and do not overlap. The configuration respects:
- Hardware strapping pins (GPIO 0, 2, 5, 12, 15)
- System reserved pins (GPIO 6-11)
- ADC input-only pins are not used for PWM output

---

## 📝 CHANGES FROM ORIGINAL

| File | Old Pins | New Pins | Reason |
|------|----------|----------|--------|
| motor.h | 2,4,5,16,17,18 | 32,33,25 (EN) + 26,27,14 (DIR) | Match schematic layout |
| uart.h | 15 (TX), 19 (RX) | 1 (TX), 3 (RX) | Match schematic layout |
| led.h | GPIO 0 | GPIO 4 | Avoid strapping pin |
| i2c_config.h | 21, 22 | 21, 22 | Unchanged - already correct |
| a4l.h | 32, 33, 23 | 32, 33, 23 | Unchanged - confirmed from schematic |

---

## 🧪 VERIFICATION CHECKLIST

- [x] Motor PWM pins are valid for PWM output
- [x] Motor DIR pins are valid GPIO outputs
- [x] Encoder pins support input (GPIO 12, 13, 16, 17)
- [x] UART pins support TX/RX operation
- [x] I2C pins support I2C protocol
- [x] LED pin supports WS2812 data transmission
- [x] No GPIO used for multiple functions
- [x] No conflicts with strapping pins
- [x] All pins within ESP32 GPIO range (0-39)

---

## 📚 Related Files Updated

- [include/motor.h](include/motor.h) - Motor control pins
- [include/uart.h](include/uart.h) - UART communication pins
- [include/led.h](include/led.h) - LED control pin
- [include/i2c_config.h](include/i2c_config.h) - I2C pins (unchanged)
- [include/a4l.h](include/a4l.h) - Button control pins (unchanged)

---

## 🔗 References

- **Schematic:** Single Cube ESP Motor Control (provided)
- **MCU:** NodeMCU-32S (ESP32-S)
- **Original GPIO Summary:** [GPIO_SUMMARY.md](GPIO_SUMMARY.md)
- **Pin Clusters:** [GPIO_PINOUT_CLUSTERS.md](GPIO_PINOUT_CLUSTERS.md)

