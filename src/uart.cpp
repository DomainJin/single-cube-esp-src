#include "uart.h"

#define TOUCH_HOLD_THRESHOLD 500  // ms để coi là touch hold
#define TOUCH_DEBOUNCE_TIME 50    // ms debounce

// ===== UART GLOBALS =====

HardwareSerial* pic_serial = NULL;
static String uart_rx_buffer = "";
static bool uart_initialized = false;

//touch value getters
int status = 0;
int value = 0;
int threshold = 0; 


// Touch state variables
static int last_raw_touch = 0;
static int last_threshold = 0;
static unsigned long last_data_time = 0;
static bool touchActive = false;
static bool touchHold = false;
static bool touchReleased = false;
static unsigned long touchStartMilis = 0;
static unsigned long touchDuration = 0;
static unsigned long lastTouchTime = 0;
static bool prevTouchState = false;

// ===== UART FUNCTIONS =====

bool initUART() {
    Serial.println("\n[UART] Khởi tạo UART module...");
    
    // Tạo HardwareSerial object cho UART2
    pic_serial = new HardwareSerial(UART_PORT);
    
    // Khởi tạo UART với các tham số
    pic_serial->begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    
    // Đặt rx buffer size
    pic_serial->setRxBufferSize(UART_RX_BUFFER_SIZE);
    
    uart_initialized = true;
    
    Serial.printf("[UART] UART%d khởi tạo thành công\n", UART_PORT);
    Serial.printf("[UART] RX=%d, TX=%d, Baud=%d\n", UART_RX_PIN, UART_TX_PIN, UART_BAUD);
    Serial.println("[UART] Sẵn sàng nhận dữ liệu từ PIC...");
    
    return true;
}


bool isUARTReady() {
    return uart_initialized && pic_serial != NULL;
}

void handleUARTData() {
    if (!isUARTReady()) {
        return;
    }
    
    // Đọc từng ký tự từ UART
    while (pic_serial->available()) {
        int ch = pic_serial->read();
        
        if (ch < 0) break;  // Lỗi đọc
        
        if (ch == '\r' || ch == '\n') {
            // Kết thúc dòng
            if (uart_rx_buffer.length() > 0) {
                Serial.printf("[PIC] Thô: %s\n", uart_rx_buffer.c_str());
                
                if (uart_rx_buffer.startsWith("Val:")) {
                    value = getValue(uart_rx_buffer);
                    // Serial.printf("[PIC] Value: %d\n", getValue(uart_rx_buffer));
                    
                }
                else if (uart_rx_buffer.startsWith("Thr:")) {
                    threshold = getThreshold(uart_rx_buffer);
                    // Serial.printf("[PIC] Threshold: %d\n", getThreshold(uart_rx_buffer));
                }
                else if (uart_rx_buffer.startsWith("Stt:")) {
                    status = getRawTouch(uart_rx_buffer);
                    // Serial.printf("[PIC] Status: %d\n", getRawTouch(uart_rx_buffer));
                    // int touchState = getRawTouch(uart_rx_buffer);
                    // if (touchState >= 0) {  // Chỉ xử lý nếu có dữ liệu hợp lệ
                    //     processTouchEvent(touchState);
                    // }
                }
                uart_rx_buffer = "";
            }
        } else if (ch >= 32 && ch <= 126) {  // Chỉ lấy ký tự in được
            uart_rx_buffer += (char)ch;
        }
    }
}

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

void sendUARTCommand(const String& command) {
    if (!isUARTReady()) {
        Serial.println("[UART] UART chưa được khởi tạo!");
        return;
    }
    
    pic_serial->println(command);
    // pic_serial->println("\r\n");
    pic_serial->print("\r");
    Serial.printf("[UART] Gửi lệnh đến PIC: %s\n", command.c_str());
    
}