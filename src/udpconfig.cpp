#include "main.h"
#include "IR.h"
#include "a4l.h"

// ===== UDP SEND CONTROL =====
SemaphoreHandle_t udpMutex = NULL;  // Mutex để đồng bộ gửi UDP
unsigned long lastUDPSendTime = 0;   // Thời gian gửi UDP lần cuối
const unsigned long UDP_THROTTLE_MS = 2;  // Khoảng cách tối thiểu giữa 2 lần gửi (2ms - giảm latency)

// ===== RECALIBRATION STATE =====
static unsigned long recalibStartTime = 0;
static int recalibStep = 0;  // 0: idle, 1: sent D, 2: sent A, 3: waiting 10s, 4: done

// Hàng đợi ưu tiên cho các message
struct UDPMessage {
    char message[128];
    UDPPriority priority;
    unsigned long timestamp;
};

#define UDP_QUEUE_SIZE 50  // ✅ Tăng từ 20 lên 50 để tránh mất heartbeat
UDPMessage udpQueue[UDP_QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;

// ===== UDP TOUCH CONFIGURATION =====
// SERVER_IP và SERVER_PORT lấy từ main.h
int TOUCH_SERVER_PORT = 0;     // Port được tính theo IP (lastOctet * 100)
int LOCAL_TOUCH_PORT = 8001;   // Giá trị mặc định, sẽ được cập nhật

// ===== GLOBAL UDP TOUCH OBJECTS =====
WiFiUDP touch_udp;
IPAddress touch_server_address;

// ===== TOUCH VARIABLES =====
bool touchActive = false;
unsigned long touchDuration = 0;
int r,g,b;

// ===== DYNAMIC PORT CALCULATION =====
void calculatePortsFromLocalIP() {
    // Lấy local IP address
    IPAddress localIP = WiFi.localIP();
    
    // Lấy 8 bit cuối (octet thứ 4) của IP
    uint8_t lastOctet = localIP[3];
    
    // Tính TOUCH_SERVER_PORT = lastOctet * 100
    // Ví dụ: IP 192.168.0.43 → lastOctet = 43 → TOUCH_SERVER_PORT = 4300
    TOUCH_SERVER_PORT = lastOctet * 100;
    LOCAL_TOUCH_PORT = lastOctet * 100;
    
    Serial.printf("[UDP_PORT_CALC] Local IP: %s\n", localIP.toString().c_str());
    Serial.printf("[UDP_PORT_CALC] Last octet: %d\n", lastOctet);
    Serial.printf("[UDP_PORT_CALC] Calculated TOUCH_SERVER_PORT: %d\n", TOUCH_SERVER_PORT);
    Serial.printf("[UDP_PORT_CALC] Calculated LOCAL_TOUCH_PORT: %d\n", LOCAL_TOUCH_PORT);
}

// ===== UDP TOUCH INITIALIZATION =====
bool initUDPTouch() {
    Serial.println("[UDP_TOUCH] Khởi tạo UDP Touch module...");
    
    // Tạo mutex cho UDP
    if (udpMutex == NULL) {
        udpMutex = xSemaphoreCreateMutex();
        if (udpMutex == NULL) {
            Serial.println("[UDP_TOUCH] Lỗi: Không thể tạo mutex!");
            return false;
        }
    }
    
    // Kiểm tra WiFi trước khi khởi tạo UDP
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[UDP_TOUCH] Lỗi: WiFi chưa kết nối!");
        return false;
    }
    
    // Tính toán port dựa trên local IP
    calculatePortsFromLocalIP();
    
    // Khởi tạo UDP với port được tính toán
    if (!touch_udp.begin(LOCAL_TOUCH_PORT)) {
        Serial.printf("[UDP_TOUCH] Lỗi: Không thể khởi tạo UDP trên port %d!\n", LOCAL_TOUCH_PORT);
        return false;
    }
    
    // Thiết lập địa chỉ Touch Server từ main.h
    touch_server_address.fromString(SERVER_IP);
    
    Serial.printf("[UDP_TOUCH] Local UDP Port: %d\n", LOCAL_TOUCH_PORT);
    Serial.printf("[UDP_TOUCH] Touch Server: %s:%d (calculated from IP)\n", SERVER_IP, TOUCH_SERVER_PORT);
    Serial.println("[UDP_TOUCH] UDP Touch module sẵn sàng!");
    
    return true;
}

// ===== UDP QUEUE MANAGEMENT =====
bool enqueueUDPMessage(const char* message, UDPPriority priority) {
    if (queueCount >= UDP_QUEUE_SIZE) {
        // Hàng đợi đầy, kiểm tra có thể thay thế message ưu tiên thấp hơn không
        for (int i = 0; i < UDP_QUEUE_SIZE; i++) {
            int idx = (queueHead + i) % UDP_QUEUE_SIZE;
            if (udpQueue[idx].priority < priority) {
                // Thay thế message có ưu tiên thấp hơn
                strncpy(udpQueue[idx].message, message, sizeof(udpQueue[idx].message) - 1);
                udpQueue[idx].message[sizeof(udpQueue[idx].message) - 1] = '\0';
                udpQueue[idx].priority = priority;
                udpQueue[idx].timestamp = millis();
                return true;
            }
        }
        // Serial.println("[UDP_QUEUE] Hàng đợi đầy, bỏ qua message!");
        return false;
    }
    
    // Thêm vào hàng đợi
    strncpy(udpQueue[queueTail].message, message, sizeof(udpQueue[queueTail].message) - 1);
    udpQueue[queueTail].message[sizeof(udpQueue[queueTail].message) - 1] = '\0';
    udpQueue[queueTail].priority = priority;
    udpQueue[queueTail].timestamp = millis();
    
    queueTail = (queueTail + 1) % UDP_QUEUE_SIZE;
    queueCount++;
    return true;
}

int findHighestPriorityMessage() {
    if (queueCount == 0) return -1;
    
    int highestIdx = queueHead;
    UDPPriority highestPriority = udpQueue[queueHead].priority;
    
    for (int i = 1; i < queueCount; i++) {
        int idx = (queueHead + i) % UDP_QUEUE_SIZE;
        if (udpQueue[idx].priority > highestPriority) {
            highestPriority = udpQueue[idx].priority;
            highestIdx = idx;
        }
    }
    
    return highestIdx;
}

bool dequeueUDPMessage(char* message, size_t maxLen) {
    if (queueCount == 0) return false;
    
    // Tìm message có ưu tiên cao nhất
    int idx = findHighestPriorityMessage();
    if (idx == -1) return false;
    
    // Copy message
    strncpy(message, udpQueue[idx].message, maxLen - 1);
    message[maxLen - 1] = '\0';
    
    // Xóa message khỏi queue bằng cách di chuyển các phần tử
    if (idx == queueHead) {
        queueHead = (queueHead + 1) % UDP_QUEUE_SIZE;
    } else {
        // Di chuyển các phần tử để lấp chỗ trống
        int current = idx;
        while (current != queueTail) {
            int next = (current + 1) % UDP_QUEUE_SIZE;
            if (next == queueTail) break;
            udpQueue[current] = udpQueue[next];
            current = next;
        }
        if (queueTail > 0) {
            queueTail--;
        } else {
            queueTail = UDP_QUEUE_SIZE - 1;
        }
    }
    
    queueCount--;
    return true;
}

// ===== CORE UDP SEND FUNCTION =====
bool sendUDPPacket(const char* message, UDPPriority priority, bool immediate = false) {
    if (!isUDPTouchReady()) {
        return false;
    }
    
    if (message == nullptr || strlen(message) == 0) {
        return false;
    }
    
    // Nếu không immediate, thêm vào queue
    if (!immediate) {
        return enqueueUDPMessage(message, priority);
    }
    
    // Gửi ngay lập tức với mutex (timeout ngắn để tránh block)
    if (xSemaphoreTake(udpMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // Throttling - bỏ qua nếu gửi quá nhanh (không dùng delay blocking)
        unsigned long now = millis();
        if (now - lastUDPSendTime < UDP_THROTTLE_MS) {
            xSemaphoreGive(udpMutex);
            return false;  // Bỏ qua message này
        }
        
        // Gửi packet
        touch_udp.beginPacket(touch_server_address, TOUCH_SERVER_PORT);
        touch_udp.write((uint8_t*)message, strlen(message));
        touch_udp.endPacket();
        
        lastUDPSendTime = millis();
        xSemaphoreGive(udpMutex);
        return true;
    }
    
    return false;
}

// Process queued messages (gọi trong loop)
void processUDPQueue() {
    if (queueCount == 0) return;
    
    char message[128];
    if (dequeueUDPMessage(message, sizeof(message))) {
        sendUDPPacket(message, UDP_PRIORITY_NORMAL, true);
    }
}

// ===== UDP TOUCH FUNCTIONS =====
void sendTouchValue(const char* touchMessage) {
    sendUDPPacket(touchMessage, UDP_PRIORITY_NORMAL);
}

void sendTouchValueInt(int touchValue) {
    // Tạo message từ số nguyên
    char message[32];
    snprintf(message, sizeof(message), "Raw Value: %d", touchValue);
    
    // Gọi hàm chính
    sendTouchValue(message);
}

// Hàm gửi giá trị ADC raw qua UDP
void sendADCValue(uint16_t adcRaw, float adcVoltage) {
    char adcMessage[64];
    snprintf(adcMessage, sizeof(adcMessage), "ADC_RAW:%d,VOLTAGE:%.3f", adcRaw, adcVoltage);
    sendUDPPacket(adcMessage, UDP_PRIORITY_LOW);
}

// Hàm gửi chỉ ADC raw (đơn giản hơn)
void sendADCRaw(uint16_t adcRaw) {
    char adcMessage[32];
    snprintf(adcMessage, sizeof(adcMessage), "ADC:%d", adcRaw);
    sendUDPPacket(adcMessage, UDP_PRIORITY_LOW);
}

// Hàm gửi chỉ voltage (đơn giản hơn)
void sendADCVoltage(float voltage) {
    char voltageMessage[32];
    snprintf(voltageMessage, sizeof(voltageMessage), "VOLTAGE:%.3f", voltage);
    sendUDPPacket(voltageMessage, UDP_PRIORITY_LOW);
}

// ===== UDP TOUCH UTILITY FUNCTIONS =====
bool isUDPTouchReady() {
    return (WiFi.status() == WL_CONNECTED);
}

// ===== UDP RECEIVE FUNCTIONS =====
int availableUDPData() {
    if (!isUDPTouchReady()) {
        return 0;
    }
    return touch_udp.parsePacket();
}

int receiveUDPData(char* buffer, int bufferSize) {
    if (!isUDPTouchReady()) {
        Serial.println("[UDP_RECEIVE] Lỗi: WiFi chưa kết nối!");
        return -1;
    }
    
    // ✅ KHÔNG gọi parsePacket() nữa - đã gọi ở availableUDPData() rồi
    // Chỉ đọc dữ liệu từ packet hiện tại
    int bytesRead = touch_udp.read(buffer, bufferSize - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Null terminate
        
        // In thông tin về packet nhận được
        IPAddress remoteIP = touch_udp.remoteIP();
        int remotePort = touch_udp.remotePort();
        
        Serial.printf("[UDP_RECEIVE] Nhận %d bytes từ %s:%d\n", 
                     bytesRead, remoteIP.toString().c_str(), remotePort);
        Serial.printf("[UDP_RECEIVE] Dữ liệu: %s\n", buffer);
    }
    
    return bytesRead;
}

void handleUDPReceive() {
    if (!isUDPTouchReady()) {
        return;
    }
    
    // ✅ Chỉ xử lý touch effect mỗi 50ms để giảm load
    static unsigned long lastTouchUpdate = 0;
    if (millis() - lastTouchUpdate > 50) {
        lastTouchUpdate = millis();
        touchActive = isTouchActive();
        touchDuration = getTouchDuration();
        applyColorWithBrightness(touchActive, r, g, b);
    }
    
    int packetSize = touch_udp.parsePacket();
    
    if (packetSize > 0) {
        // ✅ Chỉ log khi có packet quan trọng
        // Serial.printf("[DEBUG] Có packet size: %d bytes\n", packetSize);
        
        char buffer[256];
        int bytesRead = touch_udp.read(buffer, sizeof(buffer) - 1);
        
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            
            IPAddress remoteIP = touch_udp.remoteIP();
            int remotePort = touch_udp.remotePort();
            
            // ✅ Chỉ log command quan trọng, không log mọi packet
            // Serial.printf("[UDP_RECEIVE] Nhận %d bytes từ %s:%d\n", 
            //              bytesRead, remoteIP.toString().c_str(), remotePort);
            // Serial.printf("[UDP_RECEIVE] Dữ liệu: %s\n", buffer);
            
            String data = String(buffer);
            data.trim();
            
            // ✅ Xử lý lệnh THRESHOLD
            if (data.startsWith("THRESHOLD:")) {
                int colonPos = data.indexOf(':');
                String thresholdStr = data.substring(colonPos + 1);
                thresholdStr.trim();
                
                int thresholdValue = thresholdStr.toInt();
                
                // Format thành 5 chữ số với leading zeros
                char uartCommand[16];
                snprintf(uartCommand, sizeof(uartCommand), "T%05dX", thresholdValue);
                
                // Gửi qua UART
                sendUARTCommand(String(uartCommand));
                
                Serial.printf("[UDP_THRESHOLD] Nhận threshold: %d -> Gửi UART: %s\n", 
                             thresholdValue, uartCommand);
            }
            
            // ✅ Xử lý lệnh XILANH
            else if (data.startsWith("XILANH:")) {
                int colonPos = data.indexOf(':');
                String valueStr = data.substring(colonPos + 1);
                valueStr.trim();
                
                int xiLanhValue = valueStr.toInt();
                
                if (xiLanhValue == 1) {
                    digitalWrite(2, HIGH);
                    digitalWrite(15, LOW);
                    Serial.println("[UDP_XILANH] IO15 -> HIGH (Xi lanh DOWN)");
                } else if (xiLanhValue == 2) {
                    digitalWrite(2, LOW);
                    digitalWrite(15, HIGH);
                    Serial.println("[UDP_XILANH] IO15 -> LOW (Xi lanh UP)");
                } else if (xiLanhValue == 0) {
                    digitalWrite(2, LOW);
                    digitalWrite(15, LOW);
                    Serial.println("[UDP_XILANH] IO15 -> LOW (Xi lanh STOP)");
                }
                
                else {
                    Serial.printf("[UDP_XILANH] Giá trị không hợp lệ: %d (chỉ chấp nhận 0 hoặc 1)\n", xiLanhValue);
                }
            }

            else if (data.startsWith("IRtransmitOut:")) {
                int colonPos = data.indexOf(':');
                String valueStr = data.substring(colonPos + 1);
                valueStr.trim();
                
                float VoltageValue = valueStr.toFloat();
                
                // tranLedOut(VoltageValue);
                Serial.printf("[UDP_IR] Chân TRAN LED xuất voltage: %.2f V\n", VoltageValue);
                
            }
            else if (data.startsWith("IRRecieveOut:")) {
                int colonPos = data.indexOf(':');
                String valueStr = data.substring(colonPos + 1);
                valueStr.trim();
                
                float VoltageValue = valueStr.toFloat();
                
                // revLedOut(VoltageValue);
                Serial.printf("[UDP_IR] Chân REV LED xuất voltage: %.2f V\n", VoltageValue);
                
            }
            
            // ✅ Xử lý lệnh RECALIB
            else if (data.equals("RECALIB")) {
                Serial.println("[UDP_RECALIB] Bắt đầu quá trình tái hiệu chuẩn...");
                
                // Reset recalib process
                recalibStep = 1;
                recalibStartTime = millis();
                
                // Bước 1: Gửi "D" ngay lập tức
                sendUARTCommand("D");
                Serial.println("[UDP_RECALIB] Bước 1: Gửi lệnh D");
                
                // Note: Các bước tiếp theo sẽ được xử lý trong processRecalibration()
                // Cần gọi processRecalibration() trong loop()
            }
            
            // ✅ Xử lý lệnh LED CONTROL
            else if (data.startsWith("LEDCTRL:")) {
                // Format: LEDCTRL:ALL,255,128,64 hoặc LEDCTRL:5,255,0,0
                int colonPos = data.indexOf(':');
                String params = data.substring(colonPos + 1);
                
                int commaPos1 = params.indexOf(',');
                int commaPos2 = params.indexOf(',', commaPos1 + 1);
                int commaPos3 = params.indexOf(',', commaPos2 + 1);
                
                if (commaPos1 > 0 && commaPos2 > 0 && commaPos3 > 0) {
                    String target = params.substring(0, commaPos1);
                    r = params.substring(commaPos1 + 1, commaPos2).toInt();
                    g = params.substring(commaPos2 + 1, commaPos3).toInt();
                    b = params.substring(commaPos3 + 1).toInt();
                    
                    if (target.equals("ALL")) {
                        setAllLEDs(r, g, b);
                        Serial.printf("[UDP_LEDCTRL] Đặt tất cả LED: RGB(%d,%d,%d)\n", r, g, b);
                    } else {
                        int ledIndex = target.toInt();
                        setLEDRange(ledIndex, ledIndex, r, g, b);
                        Serial.printf("[UDP_LEDCTRL] Đặt LED[%d]: RGB(%d,%d,%d)\n", ledIndex, r, g, b);
                    }
                }
            }
            
            // ✅ Xử lý lệnh LED ON/OFF
            else if (data.startsWith("LED:")) {
                int colonPos = data.indexOf(':');
                String valueStr = data.substring(colonPos + 1);
                valueStr.trim();
                
                int ledValue = valueStr.toInt();
                
                if (ledValue == 1) {
                    enableLEDEffect(true);
                    Serial.println("[UDP_LED] LED Effect -> BẬT");
                } else if (ledValue == 0) {
                    enableLEDEffect(false);
                    Serial.println("[UDP_LED] LED Effect -> TẮT");
                }
            }
            
            // ✅ Xử lý lệnh DIRECTION
            else if (data.startsWith("DIR:")) {
                int colonPos = data.indexOf(':');
                String valueStr = data.substring(colonPos + 1);
                valueStr.trim();
                
                int direction = valueStr.toInt();
                
                if (direction == 1) {
                    setLEDDirection(1);
                    Serial.println("[UDP_LED] Direction -> UP (1)");
                } else if (direction == 0) {
                    setLEDDirection(-1);
                    Serial.println("[UDP_LED] Direction -> DOWN (-1)");
                }
            }
            
            // ✅ Xử lý lệnh CONFIG MODE
            else if (data.startsWith("CONFIG:")) {
                int colonPos = data.indexOf(':');
                String valueStr = data.substring(colonPos + 1);
                valueStr.trim();
                
                int configValue = valueStr.toInt();
                Serial.printf("[UDP_LED] Config Mode -> %s\n", configValue ? "BẬT" : "TẮT");
                
                // Có thể thêm logic xử lý config mode ở đây
                if (configValue == 1) {
                    // Chế độ config - có thể disable touch effect
                    Serial.println("[UDP_LED] Vào chế độ cấu hình LED");
                } else {
                    // Chế độ bình thường - enable touch effect
                    Serial.println("[UDP_LED] Về chế độ bình thường");
                }
            }
            
            // ✅ Xử lý lệnh RAINBOW EFFECT
            else if (data.equals("RAINBOW:START")) {
                Serial.println("[UDP_LED] Bắt đầu hiệu ứng Rainbow");
                
                // Tạo hiệu ứng rainbow đơn giản
                for (int i = 0; i < 256; i += 85) {
                    r = (i < 85) ? 255 - i * 3 : (i < 170) ? 0 : (i - 170) * 3;
                    g = (i < 85) ? i * 3 : (i < 170) ? 255 - (i - 85) * 3 : 0;
                    b = (i < 85) ? 0 : (i < 170) ? (i - 85) * 3 : 255 - (i - 170) * 3;
                    
                    setAllLEDs(r, g, b);
                    delay(50);
                }
                
                Serial.println("[UDP_LED] Hoàn thành hiệu ứng Rainbow");
            }
            
            // ✅ Xử lý lệnh RGB đơn giản (format: "255 128 64")
            else if (data.indexOf(' ') > 0) {
                // Phân tích RGB từ format "r g b"
                int space1 = data.indexOf(' ');
                int space2 = data.indexOf(' ', space1 + 1);
                
                if (space1 > 0 && space2 > 0) {
                    r = data.substring(0, space1).toInt();
                    g = data.substring(space1 + 1, space2).toInt();
                    b = data.substring(space2 + 1).toInt();
                    
                    // Áp dụng màu với touch effect
                    applyColorWithBrightness(true, r, g, b);
                    Serial.printf("[UDP_LED] Áp dụng màu RGB: (%d,%d,%d)\n", r, g, b);
                }
            }
            else if (data.startsWith("A4L:")) {
                int colonPos = data.indexOf(':');
                String valueStr = data.substring(colonPos + 1);
                valueStr.trim();
                
                int a4l = valueStr.toInt();
                if (a4l == 0) {
                    a4lHDMIModeEnable();
                } else if (a4l == 1) {
                    a4lSyncModeEnable();
                } else if (a4l == 2) {
                    a4lNext();
                }
                
                else {
                    Serial.printf("[UDP_A4L] Giá trị không hợp lệ: %d (chỉ chấp nhận 0 hoặc 1)\n", a4l);
                }
            }
        }
    } else {
        // Debug message mỗi 30 giây để giảm spam
        static unsigned long lastDebug = 0;
        if (millis() - lastDebug > 30000) {
            lastDebug = millis();
            Serial.println("[DEBUG] Đang chờ UDP packets...");
        }
    }
}

// ===== IR ADC UDP FUNCTIONS =====
// Hàm gửi giá trị IR ADC raw qua UDP (chân 35)
void sendIRADCValue(uint16_t adcRaw, float adcVoltage) {
    char irAdcMessage[64];
    snprintf(irAdcMessage, sizeof(irAdcMessage), "IR_ADC_RAW:%d,IR_VOLTAGE:%.3f", adcRaw, adcVoltage);
    sendUDPPacket(irAdcMessage, UDP_PRIORITY_LOW);
}

// Hàm gửi chỉ IR ADC raw (đơn giản hơn)
void sendIRADCRaw(int index, uint16_t adcRaw) {
    char irAdcMessage[32];
    snprintf(irAdcMessage, sizeof(irAdcMessage), "IR_ADC_%d:,%d", index, adcRaw);
    sendUDPPacket(irAdcMessage, UDP_PRIORITY_LOW);
}

// Hàm gửi chỉ IR voltage (đơn giản hơn)
void sendIRVoltage(float voltage) {
    char irVoltageMessage[32];
    snprintf(irVoltageMessage, sizeof(irVoltageMessage), "IR_VOLTAGE:%.3f", voltage);
    sendUDPPacket(irVoltageMessage, UDP_PRIORITY_LOW);
}

// Hàm gửi IR receive data (chân 35 đọc tín hiệu phản hồi)
void sendIRReceiveData(uint16_t adcRaw, float voltage) {
    char irReceiveMessage[64];
    snprintf(irReceiveMessage, sizeof(irReceiveMessage), "IR_RECEIVE:RAW=%d,VOLT=%.3f", adcRaw, voltage);
    sendUDPPacket(irReceiveMessage, UDP_PRIORITY_LOW);
}

// Hàm gửi chỉ IR ADC raw (đơn giản hơn)
void sendIRThreshold(int index, uint16_t threshold) {
    char irAdcMessage[32];
    snprintf(irAdcMessage, sizeof(irAdcMessage), "IR_THR_%d:,%d", index, threshold);
    sendUDPPacket(irAdcMessage, UDP_PRIORITY_LOW);
}

// Hàm gửi trạng thái mặt qua UDP
void sendStatusFace(int faceNumber, const char* status) {
    if (faceNumber < 1 || faceNumber > 6 || status == nullptr) {
        return;
    }
    
    char faceMessage[32];
    snprintf(faceMessage, sizeof(faceMessage), "FACE_%d:%s", faceNumber, status);
    sendUDPPacket(faceMessage, UDP_PRIORITY_HIGH);  // Ưu tiên cao cho face status
}

// Hàm gửi heading compass qua UDP
void sendCompassHeading(float heading, const char* direction) {
    if (direction == nullptr) {
        return;
    }
    
    // Offset = declination angle (góc lệch từ)
    float offset = 0.5;  // Vietnam ~0.5°
    
    char compassMessage[64];
    snprintf(compassMessage, sizeof(compassMessage), "COMPASS:%.1f,%.1f,%s", heading, offset, direction);
    sendUDPPacket(compassMessage, UDP_PRIORITY_NORMAL);  // Ưu tiên bình thường
}

// Hàm gửi dữ liệu thô magnetometer qua UDP
void sendCompassRaw(int16_t mx, int16_t my, int16_t mz) {
    char magMessage[64];
    snprintf(magMessage, sizeof(magMessage), "MAG:%d,%d,%d", mx, my, mz);
    sendUDPPacket(magMessage, UDP_PRIORITY_LOW);  // Ưu tiên thấp (raw data)
}

// ===== RECALIBRATION NON-BLOCKING HANDLER =====
void processRecalibration() {
    if (recalibStep == 0) {
        return;  // Không có recalibration đang chạy
    }
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - recalibStartTime;
    
    switch (recalibStep) {
        case 1:
            // Bước 1 đã gửi "D", chờ 100ms rồi gửi "A"
            if (elapsed >= 100) {
                sendUARTCommand("A");
                Serial.println("[UDP_RECALIB] Bước 2: Gửi lệnh A - Chờ 10 giây...");
                recalibStep = 2;
                recalibStartTime = currentTime;  // Reset timer
            }
            break;
            
        case 2:
            // Đang chờ 10 giây
            if (elapsed >= 10000) {
                sendUARTCommand("E");
                Serial.println("[UDP_RECALIB] Bước 3: Gửi lệnh E - Hoàn thành tái hiệu chuẩn!");
                recalibStep = 0;  // Done
            } else {
                // In progress mỗi 2 giây
                static unsigned long lastProgressPrint = 0;
                if (currentTime - lastProgressPrint > 2000) {
                    lastProgressPrint = currentTime;
                    int remaining = (10000 - elapsed) / 1000;
                    Serial.printf("[UDP_RECALIB] Đang chờ... còn %d giây\n", remaining);
                }
            }
            break;
    }
}

// Hàm gửi tốc độ motor qua UDP
void sendSpeed(int16_t s1, int16_t s2, int16_t s3) {
    char speedMessage[64];
    snprintf(speedMessage, sizeof(speedMessage), "SPEED:%d,%d,%d", s1, s2, s3);
    Serial.printf("[UDP_SPEED] Sending: %s\n", speedMessage);  // DEBUG
    sendUDPPacket(speedMessage, UDP_PRIORITY_NORMAL);  // ✅ Ưu tiên NORMAL (giống COMPASS)
}