#include "IR.h"

// Khởi tạo IR pins
void initIR() {
    // Cấu hình chân 25 và 26 làm analog output (DAC)
    // ESP32 có DAC tích hợp trên chân 25 và 26
    // Không cần cấu hình pinMode cho DAC
    
    // Cấu hình chân 35 làm analog input (ADC)

    pinMode(ANALOG_READ_PIN, INPUT);
    
    Serial.println("[IR_INIT] IR module khởi tạo thành công");
}

// Hàm xuất analog cho chân 25 (revLedOut)
void revLedOut(float voltage) {
    // Kiểm tra giá trị đầu vào
    if (voltage < 0.0) {
        voltage = 0.0;
    } else if (voltage > MAX_VOLTAGE) {
        voltage = MAX_VOLTAGE;
    }
    
    // Chuyển đổi voltage thành DAC value và xuất ra
    uint16_t dacValue = voltageToDACValue(voltage);
    dacWrite(REV_LED_PIN, dacValue);
}

// Hàm xuất analog cho chân 26 (tranLedOut)
void tranLedOut(float voltage) {
    // Kiểm tra giá trị đầu vào
    if (voltage < 0.0) {
        voltage = 0.0;
    } else if (voltage > MAX_VOLTAGE) {
        voltage = MAX_VOLTAGE;
    }
    
    // Chuyển đổi voltage thành DAC value và xuất ra
    uint16_t dacValue = voltageToDACValue(voltage);
    dacWrite(TRAN_LED_PIN, dacValue);
}

// Hàm đọc giá trị analog từ chân 35 (0-3.3V)
float analogReadVoltage() {
    // Đọc giá trị ADC (0-4095 với độ phân giải 12-bit)
    uint16_t adcValue = analogRead(ANALOG_READ_PIN);
    
    // Chuyển đổi ADC value sang voltage
    // ESP32 ADC: 0-4095 tương ứng 0-3.3V
    float voltage = adcValueToVoltage(adcValue);
    
    return voltage;
}

// Hàm đọc giá trị ADC raw (0-4095)
uint16_t analogReadRaw() {
    return analogRead(ANALOG_READ_PIN);
}

// Hàm chuyển đổi từ voltage sang DAC value (cho output)
// ESP32 DAC có độ phân giải 8-bit (0-255)
// Điện áp tham chiếu là 3.3V
uint16_t voltageToDACValue(float voltage) {
    // Công thức: DAC_VALUE = (voltage / 3.3) * 255
    uint16_t dacValue = (uint16_t)((voltage / MAX_VOLTAGE) * 255.0);
    
    // Đảm bảo giá trị trong khoảng 0-255
    if (dacValue > 255) {
        dacValue = 255;
    }
    
    return dacValue;
}

// Hàm chuyển đổi từ ADC value sang voltage (cho input)
// ESP32 ADC có độ phân giải 12-bit (0-4095)
// Điện áp tham chiếu là 3.3V
float adcValueToVoltage(uint16_t adcValue) {
    // Công thức: VOLTAGE = (adcValue / 4095) * 3.3
    float voltage = (adcValue / 4095.0) * MAX_VOLTAGE;
    
    // Đảm bảo giá trị trong khoảng 0-3.3V
    if (voltage < 0.0) {
        voltage = 0.0;
    } else if (voltage > MAX_VOLTAGE) {
        voltage = MAX_VOLTAGE;
    }
    
    return voltage;
}
