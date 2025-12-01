#include "main.h"
#include "IR.h"

// ===== UDP TOUCH CONFIGURATION =====
const char* TOUCH_SERVER_IP = "192.168.0.159";
// const char* TOUCH_SERVER_IP = "192.168.1.22";
// Port sẽ được tính tự động từ local IP
int TOUCH_SERVER_PORT = 7043;  // Giá trị mặc định, sẽ được cập nhật
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
    
    // Tính port = lastOctet * 100
    // Ví dụ: IP 192.168.0.43 → lastOctet = 43 → port = 4300
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
    
    // Thiết lập địa chỉ Touch Server
    touch_server_address.fromString(TOUCH_SERVER_IP);
    
    Serial.printf("[UDP_TOUCH] Local UDP Port: %d\n", LOCAL_TOUCH_PORT);
    Serial.printf("[UDP_TOUCH] Touch Server: %s:%d\n", TOUCH_SERVER_IP, TOUCH_SERVER_PORT);
    Serial.println("[UDP_TOUCH] UDP Touch module sẵn sàng!");
    
    return true;
}

// ===== UDP TOUCH FUNCTIONS =====
void sendTouchValue(const char* touchMessage) {
    if (!isUDPTouchReady()) {
        // Serial.println("[UDP_TOUCH] Cảnh báo: UDP Touch chưa sẵn sàng!");
        return;
    }
    
    // Kiểm tra message không null và không rỗng
    if (touchMessage == nullptr || strlen(touchMessage) == 0) {
        // Serial.println("[UDP_TOUCH] Cảnh báo: Message rỗng!");
        return;
    }
    
    // Gửi UDP packet với port được tính toán
    touch_udp.beginPacket(touch_server_address, TOUCH_SERVER_PORT);
    touch_udp.write((uint8_t*)touchMessage, strlen(touchMessage));
    touch_udp.endPacket();
    
    // Serial.printf("[UDP_TOUCH] Gửi: %s -> %s:%d\n", touchMessage, TOUCH_SERVER_IP, TOUCH_SERVER_PORT);
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
    if (!isUDPTouchReady()) {
        // Serial.println("[UDP_ADC] Cảnh báo: UDP ADC chưa sẵn sàng!");
        return;
    }
    
    // Tạo message chứa cả ADC raw và voltage
    char adcMessage[64];
    snprintf(adcMessage, sizeof(adcMessage), "ADC_RAW:%d,VOLTAGE:%.3f", adcRaw, adcVoltage);
    
    // Gửi UDP packet
    touch_udp.beginPacket(touch_server_address, TOUCH_SERVER_PORT);
    touch_udp.write((uint8_t*)adcMessage, strlen(adcMessage));
    touch_udp.endPacket();
    
    // Serial.printf("[UDP_ADC] Gửi: %s -> %s:%d\n", adcMessage, TOUCH_SERVER_IP, TOUCH_SERVER_PORT);
}

// Hàm gửi chỉ ADC raw (đơn giản hơn)
void sendADCRaw(uint16_t adcRaw) {
    if (!isUDPTouchReady()) {
        return;
    }
    
    // Tạo message đơn giản
    char adcMessage[32];
    snprintf(adcMessage, sizeof(adcMessage), "ADC:%d", adcRaw);
    
    // Gửi UDP packet
    touch_udp.beginPacket(touch_server_address, TOUCH_SERVER_PORT);
    touch_udp.write((uint8_t*)adcMessage, strlen(adcMessage));
    touch_udp.endPacket();
    
    Serial.printf("[UDP_ADC] Gửi ADC raw: %d -> %s:%d\n", adcRaw, TOUCH_SERVER_IP, TOUCH_SERVER_PORT);
}

// Hàm gửi chỉ voltage (đơn giản hơn)
void sendADCVoltage(float voltage) {
    if (!isUDPTouchReady()) {
        return;
    }
    
    // Tạo message đơn giản
    char voltageMessage[32];
    snprintf(voltageMessage, sizeof(voltageMessage), "VOLTAGE:%.3f", voltage);
    
    // Gửi UDP packet
    touch_udp.beginPacket(touch_server_address, TOUCH_SERVER_PORT);
    touch_udp.write((uint8_t*)voltageMessage, strlen(voltageMessage));
    touch_udp.endPacket();
    
    // Serial.printf("[UDP_ADC] Gửi voltage: %.3fV -> %s:%d\n", voltage, TOUCH_SERVER_IP, TOUCH_SERVER_PORT);
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
    touchActive = isTouchActive();
    touchDuration = getTouchDuration();
    
    // Áp dụng màu hiện tại với touch effect
    applyColorWithBrightness(touchActive, r, g, b);
    if (!isUDPTouchReady()) {
        return;
    }
    
    int packetSize = touch_udp.parsePacket();
    
    if (packetSize > 0) {
        Serial.printf("[DEBUG] Có packet size: %d bytes\n", packetSize);
        
        char buffer[256];
        int bytesRead = touch_udp.read(buffer, sizeof(buffer) - 1);
        
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            
            IPAddress remoteIP = touch_udp.remoteIP();
            int remotePort = touch_udp.remotePort();
            
            Serial.printf("[UDP_RECEIVE] Nhận %d bytes từ %s:%d\n", 
                         bytesRead, remoteIP.toString().c_str(), remotePort);
            Serial.printf("[UDP_RECEIVE] Dữ liệu: %s\n", buffer);
            
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
                    digitalWrite(17, HIGH);
                    digitalWrite(18, LOW);
                    Serial.println("[UDP_XILANH] IO15 -> HIGH (Xi lanh DOWN)");
                } else if (xiLanhValue == 2) {
                    digitalWrite(17, LOW);
                    digitalWrite(18, HIGH);
                    Serial.println("[UDP_XILANH] IO15 -> LOW (Xi lanh UP)");
                } else if (xiLanhValue == 0) {
                    digitalWrite(17, LOW);
                    digitalWrite(18, LOW);
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
                
                tranLedOut(VoltageValue);
                Serial.printf("[UDP_IR] Chân TRAN LED xuất voltage: %.2f V\n", VoltageValue);
                
            }
            else if (data.startsWith("IRRecieveOut:")) {
                int colonPos = data.indexOf(':');
                String valueStr = data.substring(colonPos + 1);
                valueStr.trim();
                
                float VoltageValue = valueStr.toFloat();
                
                revLedOut(VoltageValue);
                Serial.printf("[UDP_IR] Chân REV LED xuất voltage: %.2f V\n", VoltageValue);
                
            }
            
            // ✅ Xử lý lệnh RECALIB
            else if (data.equals("RECALIB")) {
                Serial.println("[UDP_RECALIB] Bắt đầu quá trình tái hiệu chuẩn...");
                
                // Bước 1: Gửi "D"
                sendUARTCommand("D");
                Serial.println("[UDP_RECALIB] Bước 1: Gửi lệnh D");
                
                delay(100); // Delay ngắn giữa các lệnh
                
                // Bước 2: Gửi "A"  
                sendUARTCommand("A");
                Serial.println("[UDP_RECALIB] Bước 2: Gửi lệnh A - Chờ 10 giây...");
                
                // Bước 3: Đợi 10 giây rồi gửi "E"
                delay(10000); // 10 seconds
                
                sendUARTCommand("E");
                Serial.println("[UDP_RECALIB] Bước 3: Gửi lệnh E - Hoàn thành tái hiệu chuẩn!");
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
    if (!isUDPTouchReady()) {
        // Serial.println("[UDP_IR_ADC] Cảnh báo: UDP chưa sẵn sàng!");
        return;
    }
    
    // Tạo message chứa cả IR ADC raw và voltage
    char irAdcMessage[64];
    snprintf(irAdcMessage, sizeof(irAdcMessage), "IR_ADC_RAW:%d,IR_VOLTAGE:%.3f", adcRaw, adcVoltage);
    
    // Gửi UDP packet
    touch_udp.beginPacket(touch_server_address, TOUCH_SERVER_PORT);
    touch_udp.write((uint8_t*)irAdcMessage, strlen(irAdcMessage));
    touch_udp.endPacket();
    
    // Serial.printf("[UDP_IR_ADC] Gửi: %s -> %s:%d\n", irAdcMessage, TOUCH_SERVER_IP, TOUCH_SERVER_PORT);
}

// Hàm gửi chỉ IR ADC raw (đơn giản hơn)
void sendIRADCRaw(uint16_t adcRaw) {
    if (!isUDPTouchReady()) {
        return;
    }
    
    // Tạo message đơn giản
    char irAdcMessage[32];
    snprintf(irAdcMessage, sizeof(irAdcMessage), "IR_ADC:%d", adcRaw);
    
    // Gửi UDP packet
    touch_udp.beginPacket(touch_server_address, TOUCH_SERVER_PORT);
    touch_udp.write((uint8_t*)irAdcMessage, strlen(irAdcMessage));
    touch_udp.endPacket();
    
    Serial.printf("[UDP_IR_ADC] Gửi IR ADC raw: %d -> %s:%d\n", adcRaw, TOUCH_SERVER_IP, TOUCH_SERVER_PORT);
}

// Hàm gửi chỉ IR voltage (đơn giản hơn)
void sendIRVoltage(float voltage) {
    if (!isUDPTouchReady()) {
        return;
    }
    
    // Tạo message đơn giản
    char irVoltageMessage[32];
    snprintf(irVoltageMessage, sizeof(irVoltageMessage), "IR_VOLTAGE:%.3f", voltage);
    
    // Gửi UDP packet
    touch_udp.beginPacket(touch_server_address, TOUCH_SERVER_PORT);
    touch_udp.write((uint8_t*)irVoltageMessage, strlen(irVoltageMessage));
    touch_udp.endPacket();
    
    // Serial.printf("[UDP_IR_ADC] Gửi IR voltage: %.3fV -> %s:%d\n", voltage, TOUCH_SERVER_IP, TOUCH_SERVER_PORT);
}

// Hàm gửi IR receive data (chân 35 đọc tín hiệu phản hồi)
void sendIRReceiveData(uint16_t adcRaw, float voltage) {
    if (!isUDPTouchReady()) {
        return;
    }
    
    // Format đặc biệt cho IR receive data
    char irReceiveMessage[64];
    snprintf(irReceiveMessage, sizeof(irReceiveMessage), "IR_RECEIVE:RAW=%d,VOLT=%.3f", adcRaw, voltage);
    
    // Gửi UDP packet
    touch_udp.beginPacket(touch_server_address, TOUCH_SERVER_PORT);
    touch_udp.write((uint8_t*)irReceiveMessage, strlen(irReceiveMessage));
    touch_udp.endPacket();
    
    Serial.printf("[UDP_IR_RECEIVE] Gửi IR receive data: %s -> %s:%d\n", 
                 irReceiveMessage, TOUCH_SERVER_IP, TOUCH_SERVER_PORT);
}