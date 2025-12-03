#include "IR.h"
#include "udpconfig.h"

// Khởi tạo biến statusIR toàn cục
StatusIR statusIR = {
    .Pin = {0, 0, 0, 0, 0, 0},  // Tất cả pin ban đầu = 0
    .Face = {None, None, None, None, None, None}  // Tất cả mặt ban đầu = None
};

int numOfSensor = 2;
int rawAdcValue = 0;
int thresholdValue = 2000;

// Biến lưu trạng thái trước đó để phát hiện swipe
struct SwipeDetector {
    bool pin1WasActive;
    bool pin2WasActive;
    bool pin1JustReleased;      // Pin 1 vừa được thả ra
    bool pin2JustReleased;      // Pin 2 vừa được thả ra
    unsigned long pin1ReleaseTime;  // Thời điểm Pin 1 tắt
    unsigned long pin2ReleaseTime;  // Thời điểm Pin 2 tắt
    unsigned long pin1ActiveTime;
    unsigned long pin2ActiveTime;
    unsigned long swipeTimeout;  // Thời gian tối đa giữa 2 lần chạm (ms)
    FaceStatus lastStatus;       // Lưu trạng thái trước đó để tránh gửi lặp
    bool swipeDetected;          // Cờ đánh dấu đã phát hiện swipe
};

SwipeDetector swipe1 = {false, false, false, false, 0, 0, 0, 0, 500, None, false};  // Mặt 1 (pin 1 và 2)


// Khởi tạo IR pins
void initIR() {
    // Cấu hình chân 25 và 26 làm analog output (DAC)
    // ESP32 có DAC tích hợp trên chân 25 và 26
    // Không cần cấu hình pinMode cho DAC
    
    // Cấu hình chân 35 làm analog input (ADC)

    pinMode(ANALOG_READ_PIN_1, INPUT);
    pinMode(ANALOG_READ_PIN_2, INPUT);
    
    Serial.println("[IR_INIT] IR module khởi tạo thành công");
}

// // Hàm xuất analog cho chân 25 (revLedOut)
// void revLedOut(float voltage) {
//     // Kiểm tra giá trị đầu vào
//     if (voltage < 0.0) {
//         voltage = 0.0;
//     } else if (voltage > MAX_VOLTAGE) {
//         voltage = MAX_VOLTAGE;
//     }
    
//     // Chuyển đổi voltage thành DAC value và xuất ra
//     uint16_t dacValue = voltageToDACValue(voltage);
//     dacWrite(REV_LED_PIN, dacValue);
// }

// // Hàm xuất analog cho chân 26 (tranLedOut)
// void tranLedOut(float voltage) {
//     // Kiểm tra giá trị đầu vào
//     if (voltage < 0.0) {
//         voltage = 0.0;
//     } else if (voltage > MAX_VOLTAGE) {
//         voltage = MAX_VOLTAGE;
//     }
    
//     // Chuyển đổi voltage thành DAC value và xuất ra
//     uint16_t dacValue = voltageToDACValue(voltage);
//     dacWrite(TRAN_LED_PIN, dacValue);
// }

// Hàm đọc giá trị analog từ chân 35 (0-3.3V)
float analogReadVoltage(int pin) {
    // Đọc giá trị ADC (0-4095 với độ phân giải 12-bit)
    uint16_t adcValue = analogRead(pin);
    
    // Chuyển đổi ADC value sang voltage
    // ESP32 ADC: 0-4095 tương ứng 0-3.3V
    float voltage = adcValueToVoltage(adcValue);
    
    return voltage;
}

// Hàm đọc giá trị ADC raw (0-4095)
uint16_t analogReadRaw(int pin) {
    return analogRead(pin);
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

void handleIRModule() {
    unsigned long currentTime = millis();
    
    // Đọc giá trị từ 2 pin (Pin 1 và Pin 2 tạo thành Mặt 1)
    float inputVoltage_1 = analogReadVoltage(ANALOG_READ_PIN_1);
    uint16_t rawValue_1 = analogReadRaw(ANALOG_READ_PIN_1);
    sendIRADCRaw(1, rawValue_1);
    sendIRThreshold(1, thresholdValue);
    
    float inputVoltage_2 = analogReadVoltage(ANALOG_READ_PIN_2);
    uint16_t rawValue_2 = analogReadRaw(ANALOG_READ_PIN_2);
    sendIRADCRaw(2, rawValue_2);
    sendIRThreshold(2, thresholdValue);
    
    // Cập nhật trạng thái Pin
    bool pin1Active = (rawValue_1 > thresholdValue);
    bool pin2Active = (rawValue_2 > thresholdValue);
    
    statusIR.Pin._1 = pin1Active ? 1 : 0;
    statusIR.Pin._2 = pin2Active ? 1 : 0;
    
    // Biến lưu trạng thái hiện tại
    FaceStatus currentStatus = None;
    bool shouldSend = false;
    
    // ===== PHÁT HIỆN SWIPE UP: Pin 1 active -> tắt -> Pin 2 active =====
    
    // Phát hiện Pin 1 vừa được kích hoạt
    if (pin1Active && !swipe1.pin1WasActive) {
        swipe1.pin1ActiveTime = currentTime;
        swipe1.pin1WasActive = true;
        swipe1.pin1JustReleased = false;
        Serial.println("[SWIPE] Pin 1 pressed");
    }
    
    // Phát hiện Pin 1 vừa được thả ra
    if (!pin1Active && swipe1.pin1WasActive) {
        swipe1.pin1ReleaseTime = currentTime;
        swipe1.pin1WasActive = false;
        swipe1.pin1JustReleased = true;
        Serial.println("[SWIPE] Pin 1 released");
    }
    
    // Phát hiện Pin 2 vừa được kích hoạt
    if (pin2Active && !swipe1.pin2WasActive) {
        swipe1.pin2ActiveTime = currentTime;
        swipe1.pin2WasActive = true;
        swipe1.pin2JustReleased = false;
        
        // Kiểm tra swipe UP: Pin 1 vừa thả ra -> Pin 2 active trong timeout
        if (swipe1.pin1JustReleased && 
            (currentTime - swipe1.pin1ReleaseTime) < swipe1.swipeTimeout &&
            !swipe1.swipeDetected) {
            currentStatus = UP;
            swipe1.swipeDetected = true;
            Serial.println("[SWIPE] Detected UP (Pin 1 -> release -> Pin 2)");
        } else {
            Serial.println("[SWIPE] Pin 2 pressed");
        }
    }
    
    // Phát hiện Pin 2 vừa được thả ra
    if (!pin2Active && swipe1.pin2WasActive) {
        swipe1.pin2ReleaseTime = currentTime;
        swipe1.pin2WasActive = false;
        swipe1.pin2JustReleased = true;
        Serial.println("[SWIPE] Pin 2 released");
    }
    
    // ===== PHÁT HIỆN SWIPE DOWN: Pin 2 active -> tắt -> Pin 1 active =====
    
    // Kiểm tra swipe DOWN: Pin 2 vừa thả ra -> Pin 1 active trong timeout
    if (pin1Active && !swipe1.pin1WasActive && 
        swipe1.pin2JustReleased && 
        (currentTime - swipe1.pin2ReleaseTime) < swipe1.swipeTimeout &&
        !swipe1.swipeDetected) {
        currentStatus = DOWN;
        swipe1.swipeDetected = true;
        Serial.println("[SWIPE] Detected DOWN (Pin 2 -> release -> Pin 1)");
    }
    
    // ===== XỬ LÝ TRẠNG THÁI =====
    
    if (pin1Active || pin2Active) {
        // Có ít nhất 1 pin active
        if (currentStatus == None && !swipe1.swipeDetected) {
            // Nếu chưa phát hiện swipe thì là TOUCH
            currentStatus = TOUCH;
        }
        
        statusIR.Face._1 = currentStatus;
        
        // Chỉ gửi nếu trạng thái thay đổi
        if (currentStatus != swipe1.lastStatus) {
            shouldSend = true;
            swipe1.lastStatus = currentStatus;
        }
        
    } else {
        // Không có pin nào active
        currentStatus = None;
        statusIR.Face._1 = None;
        
        // Chỉ gửi nếu trạng thái thay đổi
        if (swipe1.lastStatus != None) {
            shouldSend = true;
            swipe1.lastStatus = None;
        }
        
        // Reset trạng thái swipe khi không có pin nào active
        // Giữ trạng thái JustReleased trong timeout để phát hiện swipe
        if (swipe1.pin1JustReleased && 
            (currentTime - swipe1.pin1ReleaseTime) > swipe1.swipeTimeout) {
            swipe1.pin1JustReleased = false;
        }
        if (swipe1.pin2JustReleased && 
            (currentTime - swipe1.pin2ReleaseTime) > swipe1.swipeTimeout) {
            swipe1.pin2JustReleased = false;
        }
        
        swipe1.swipeDetected = false;
    }
    
    // Gửi trạng thái nếu có thay đổi
    if (shouldSend) {
        const char* statusStr = "None";
        switch(currentStatus) {
            case UP: statusStr = "UP"; break;
            case DOWN: statusStr = "DOWN"; break;
            case TOUCH: statusStr = "TOUCH"; break;
            case None: statusStr = "None"; break;
        }
        sendStatusFace(1, statusStr);
    }
}