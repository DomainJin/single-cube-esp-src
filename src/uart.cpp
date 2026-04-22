#include "uart.h"
#include "udpconfig.h"

// COMMENTED - PIC Touch-related code
// #define TOUCH_HOLD_THRESHOLD 500  // ms để coi là touch hold
// #define TOUCH_DEBOUNCE_TIME 50    // ms debounce

// ===== UART GLOBALS =====

HardwareSerial* pic_serial = NULL;
static String uart_rx_buffer = "";
static bool uart_initialized = false;

// STM32 ADC Data (6 channels)
int16_t adc_values[6] = {0, 0, 0, 0, 0, 0};

// COMMENTED - PIC touch value getters
// int status = 0;
// int value = 0;
// int threshold = 0; 


// COMMENTED - PIC Touch state variables
// static int last_raw_touch = 0;
// static int last_threshold = 0;
// static unsigned long last_data_time = 0;
// static bool touchActive = false;
// static bool touchHold = false;
// static bool touchReleased = false;
// static unsigned long touchStartMilis = 0;
// static unsigned long touchDuration = 0;
// static unsigned long lastTouchTime = 0;
// static bool prevTouchState = false;

// ===== UART FUNCTIONS =====

bool initUART() {
    // Serial.println("\n[UART] Khởi tạo UART module...");
    
    // Tạo HardwareSerial object cho UART2
    pic_serial = new HardwareSerial(UART_PORT);
    
    // Khởi tạo UART với các tham số (GPIO 19 RX, 15 TX)
    // GPIO 15 strapping pin nhưng dùng làm TX (ESP32 kiểm soát, safe)
    pic_serial->begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    
    // Đặt rx buffer size
    pic_serial->setRxBufferSize(UART_RX_BUFFER_SIZE);
    
    uart_initialized = true;
    
    // Serial.printf("[UART] UART%d khởi tạo thành công\n", UART_PORT);
    // Serial.printf("[UART] RX=%d (STM32 ADC data), TX=%d (Commands), Baud=%d\n", UART_RX_PIN, UART_TX_PIN, UART_BAUD);
    // Serial.println("[UART] Sẵn sàng nhận dữ liệu từ STM32...");
    
    return true;
}


bool isUARTReady() {
    return uart_initialized && pic_serial != NULL;
}

void sendUARTData(const uint8_t* data, uint16_t len) {
    if (!isUARTReady()) {
        // Serial.println("[UART] UART chưa được khởi tạo!");
        return;
    }
    
    pic_serial->write(data, len);
    // Serial.printf("[UART_TX] Gửi %d bytes đến STM32\n", len);
}

void handleUARTData() {
    if (!isUARTReady()) {
        return;
    }
    
    // Debug: In số bytes available
    // static unsigned long lastDebug = 0;
    // if (pic_serial->available() > 0 && millis() - lastDebug > 1000) {
    //     lastDebug = millis();
    //     // Serial.printf("[UART_DEBUG] %d bytes available\n", pic_serial->available());
    // }
    
    // Đọc từng ký tự từ UART
    while (pic_serial->available()) {
        int ch = pic_serial->read();
        
        if (ch < 0) break;  // Lỗi đọc
        
        if (ch == '\r' || ch == '\n') {
            // Kết thúc dòng
            if (uart_rx_buffer.length() > 0) {
                // ✅ In dữ liệu thô nhận được
                // Serial.printf("[UART_RX] Raw: %s\n", uart_rx_buffer.c_str());
                
                // ✅ ECHO LẠI DỮ LIỆU ĐẾN STM32
                pic_serial->printf("[ECHO] %s\r\n", uart_rx_buffer.c_str());
                // Serial.printf("[UART_ECHO] Đã gửi lại: %s\n", uart_rx_buffer.c_str());
                
                // ✅ Parse STM32 ADC frame: "A0:2377 A1  2191" format
                // New format: "A0:xxxx A1:xxxx A2:xxxx ..." (2-6 channels depending on STM32)
                if (uart_rx_buffer.startsWith("A0:") || uart_rx_buffer.startsWith("ADC:")) {
                    
                    // Try new format first: "A0:2377 A1  2191 ..."
                    if (uart_rx_buffer.startsWith("A0:")) {
                        // Parse format: "A0:2377 A1  2191 A2:2345 ..."
                        // Split by space and extract A0, A1, A2, etc.
                        
                        int channelIndex = 0;
                        int pos = 0;
                        
                        while (pos < uart_rx_buffer.length() && channelIndex < 6) {
                            // Find "An:" pattern
                            if (uart_rx_buffer[pos] == 'A' && 
                                (pos + 2) < uart_rx_buffer.length() && 
                                uart_rx_buffer[pos + 2] == ':') {
                                
                                // Extract channel number
                                int expectedChannel = uart_rx_buffer[pos + 1] - '0';
                                
                                // Find next space or end of string
                                int valueStart = pos + 3;
                                int valueEnd = valueStart;
                                while (valueEnd < uart_rx_buffer.length() && 
                                       uart_rx_buffer[valueEnd] != ' ') {
                                    valueEnd++;
                                }
                                
                                // Extract value
                                String valueStr = uart_rx_buffer.substring(valueStart, valueEnd);
                                valueStr.trim();
                                int value = valueStr.toInt();
                                
                                // Store in appropriate channel
                                if (expectedChannel >= 0 && expectedChannel <= 5) {
                                    adc_values[expectedChannel] = value;
                                    channelIndex++;
                                }
                                
                                // Move to next
                                pos = valueEnd;
                            } else {
                                pos++;
                            }
                        }
                        
                        // In giá trị ADC
                        // Serial.printf("[STM32_ADC] A0=%d, A1=%d, A2=%d, A3=%d, A4=%d, A5=%d\n",
                        //              adc_values[0], adc_values[1], adc_values[2],
                        //              adc_values[3], adc_values[4], adc_values[5]);
                        
                        // ✅ GỬI QUA UDP ĐẾN SERVER
                        char adcMessage[128];
                        snprintf(adcMessage, sizeof(adcMessage), 
                                "STM32_ADC:[%d,%d,%d,%d,%d,%d]",
                                adc_values[0], adc_values[1], adc_values[2],
                                adc_values[3], adc_values[4], adc_values[5]);
                        
                        // Gửi qua UDP packet
                        if (sendUDPPacket(adcMessage, UDP_PRIORITY_NORMAL)) {
                            // Serial.printf("[UDP_TX] Sent STM32 ADC data: %s\n", adcMessage);
                        } else {
                            // Serial.printf("[UDP_TX] Failed to send ADC data\n");
                        }
                    } 
                    // Old format fallback: "ADC:  [1234, 5678, ...]"
                    else if (uart_rx_buffer.startsWith("ADC:")) {
                        // Tìm vị trí dấu '[' và ']'
                        int startBracket = uart_rx_buffer.indexOf('[');
                        int endBracket = uart_rx_buffer.indexOf(']');
                        
                        if (startBracket != -1 && endBracket != -1) {
                            // Lấy chuỗi giữa '[' và ']'
                            String values = uart_rx_buffer.substring(startBracket + 1, endBracket);
                            
                            // Parse 6 giá trị ADC
                            int idx = 0;
                            int startPos = 0;
                            for (int i = 0; i < 6; i++) {
                                int commaPos = values.indexOf(',', startPos);
                                String valueStr;
                                
                                if (commaPos == -1) {
                                    // Giá trị cuối cùng
                                    valueStr = values.substring(startPos);
                                } else {
                                    valueStr = values.substring(startPos, commaPos);
                                    startPos = commaPos + 1;
                                }
                                
                                valueStr.trim();
                                adc_values[i] = valueStr.toInt();
                            }
                            
                            // In giá trị ADC
                            // Serial.printf("[STM32_ADC] CH1=%d, CH2=%d, CH3=%d, CH4=%d, CH5=%d, CH6=%d\n",
                            //              adc_values[0], adc_values[1], adc_values[2],
                            //              adc_values[3], adc_values[4], adc_values[5]);
                            
                            // ✅ GỬI QUA UDP ĐẾN SERVER
                            char adcMessage[128];
                            snprintf(adcMessage, sizeof(adcMessage), 
                                    "STM32_ADC:[%d,%d,%d,%d,%d,%d]",
                                    adc_values[0], adc_values[1], adc_values[2],
                                    adc_values[3], adc_values[4], adc_values[5]);
                            
                            // Gửi qua UDP packet
                            if (sendUDPPacket(adcMessage, UDP_PRIORITY_NORMAL)) {
                                // Serial.printf("[UDP_TX] Sent STM32 ADC data: %s\n", adcMessage);
                            } else {
                                // Serial.printf("[UDP_TX] Failed to send ADC data\n");
                            }
                        }
                    }
                }
                
                uart_rx_buffer = "";
            }
        } else if (ch >= 32 && ch <= 126) {  // Chỉ lấy ký tự in được
            uart_rx_buffer += (char)ch;
        }
    }
}

// COMMENTED - PIC Touch Event Processing
/*
void processTouchEvent(int currentTouchState) {
    unsigned long currentTime = millis();
    bool currentTouch = (currentTouchState == 1);
    
    // Debounce - chỉ xử lý nếu đã qua thời gian debounce
    if (currentTime - lastTouchTime < TOUCH_DEBOUNCE_TIME) {
        return;
    }
    
    // Phát hiện bắt đầu chạm (rising edge)
    if (currentTouch && !prevTouchState) {
        touchStartMilis = currentTime;
        touchActive = true;
        touchHold = false;
        touchReleased = false;
        touchDuration = 0;
        lastTouchTime = currentTime;
        
        // Serial.println("[TOUCH] Bắt đầu chạm");
    }
    
    // Phát hiện thả chạm (falling edge)
    else if (!currentTouch && prevTouchState) {
        if (touchActive) {
            touchDuration = currentTime - touchStartMilis;
            touchActive = false;
            touchHold = false;
            touchReleased = true;
            lastTouchTime = currentTime;
            
            // Serial.printf("[TOUCH] Thả chạm - Duration: %lu ms\n", touchDuration);
        }
    }
    
    // Xử lý touch hold
    else if (currentTouch && prevTouchState && touchActive) {
        touchDuration = currentTime - touchStartMilis;
        
        // Phát hiện touch hold
        if (!touchHold && touchDuration >= TOUCH_HOLD_THRESHOLD) {
            touchHold = true;
            // Serial.printf("[TOUCH] Touch Hold - Duration: %lu ms\n", touchDuration);
        }
        
        // Cập nhật duration liên tục khi đang hold
        if (touchHold && touchDuration % 100 == 0) {  // Print mỗi 100ms
            // Serial.printf("[TOUCH] Holding... Duration: %lu ms\n", touchDuration);
        }
    }
    
    prevTouchState = currentTouch;
}

// Getter functions để truy cập touch states từ bên ngoài
bool isTouchActive() { return touchActive; }
bool isTouchHold() { return touchHold; }
bool isTouchReleased() { return touchReleased; }
unsigned long getTouchDuration() { return touchDuration; }
unsigned long getTouchStartTime() { return touchStartMilis; }

// Function để reset touch released flag sau khi đã xử lý
void clearTouchReleased() { touchReleased = false; }

int getRawTouch(const String& data) {
    if (data.startsWith("Stt:")) {
        int colonPos = data.indexOf(':');
        String value = data.substring(colonPos + 1);
        value.trim();
        if(value.toInt())return 1;
        else return 0;
    }
    return -1;
}

int getThreshold(const String& data) {
    if (data.startsWith("Thr:")) {
        int colonPos = data.indexOf(':');
        String value = data.substring(colonPos + 1);
        value.trim();
        return value.toInt();
    }
    return -1;
}

int getValue(const String& data) {
    if (data.startsWith("Val:")) {
        int colonPos = data.indexOf(':');
        String value = data.substring(colonPos + 1);
        value.trim();
        return value.toInt();
    }
    return -1;
}

int getRawTouch() { 
    return status;
}
int getValue() { 
    return value;
}
int getThreshold() { 
    return threshold;
}
*/

// ===== GETTER FUNCTIONS FOR STM32 ADC DATA =====
int16_t getADC(uint8_t channel) {
    if (channel >= 1 && channel <= 6) {
        return adc_values[channel - 1];
    }
    return 0;
}

void getADCValues(int16_t* values) {
    for (int i = 0; i < 6; i++) {
        values[i] = adc_values[i];
    }
}

void sendUARTCommand(const String& command) {
    if (!isUARTReady()) {
        // Serial.println("[UART] UART chưa được khởi tạo!");
        return;
    }
    
    pic_serial->println(command);
    pic_serial->print("\r");
    // Serial.printf("[UART_TX] Gửi lệnh đến STM32: %s\n", command.c_str());
    
}