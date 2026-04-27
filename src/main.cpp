#include <Arduino.h>
#include "main.h"
// #include "mpu6050.h"  // Comment vì xung đột I2C
// #include "qmc5883l.h"  // Tắt I2C
#include "a4l.h"
#include "3_motor.h"


// Cấu hình WiFi
const char* ssid = "SGM";
const char* password = "19121996";
// const char* ssid = "SGM";
// const char* password = "19121996";

// Cấu hình Server - Chung cho tất cả modules
const char* SERVER_IP = "192.168.1.111";  // IP server nhận UDP
int SERVER_PORT = 1509;                   // Port server nhận UDP

WiFiUDP udp;
IPAddress resolume_address;

// Khởi tạo MPU6050
// MPU6050 mpu;  // Comment vì xung đột I2C

// QMC5883L compass;  // Tắt I2C


bool sendEnableOnce = false;
bool sendBackOnce = false;
bool sendMainOnce = false;
bool sendInitOnce = false;
int mainRunMillis = 0;

int valuetouch = 0;
int operationTime = 2000;    // 2 giây để kích hoạt main effect
int mainEffectime = 6000;    // 6 giây main effect chạy



void setup() {
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
    pinMode(15, OUTPUT);
    digitalWrite(15, LOW);
    
    // UART0 Debug - Chỉ TX (GPIO 1), không dùng RX (GPIO 3 để cho I2C)
    // RX = -1 (disable), TX = 1 (default)
    Serial.begin(115200, SERIAL_8N1, -1, 1);
    // Serial.println("\n===== ESP32 WS2812 TEST =====");
    
    // ✅ Khởi tạo LED WS2812 đầu tiên
    initLED();
    
    // Serial.println("[SETUP] WS2812 hoàn thành!");
    
    
    
    // Kết nối WiFi
    WiFi.begin(ssid, password);
    // Serial.print("Đang kết nối WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        // Serial.print(".");
    }
    
    // ✅ Tắt WiFi sleep mode để giảm latency
    WiFi.setSleep(false);
    
    // ✅ Bật auto reconnect
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    Serial.println("");
    Serial.println("WiFi đã kết nối!");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    
    // Khởi tạo các modules
    initOSC();
    initUART();  // ✅ UART sử dụng GPIO 34 (RX), 35 (TX) để nhận dữ liệu ADC từ module ngoài
    initUDPTouch();
    a4lInit();
    initIPConfig();  
    init3Motors();
    Serial.println("[SETUP] 3 Motor PID control system initialized!");
    
    // I2C / Compass disabled
    
    // Gửi heartbeat đầu tiên
    sendESPStatus("READY");
}

void loop() {
    // Kiểm tra WiFi connection mỗi 10 giây
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 10000) {
        lastWiFiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.reconnect();
        }
    }
    
    handleUARTData();  // ✅ Xử lý dữ liệu ADC từ module ngoài qua UART
    handleUDPReceive();
    processUDPQueue();  // Xử lý hàng đợi UDP
    processRecalibration();  // ✅ Xử lý RECALIB không blocking
    processRotateToDirection();  // ✅ Xử lý xoay về hướng chính
    handleHeartbeat();
    handleIRModule();  // ✅ ADC moved to UART module, data comes via UART now
    update3Motors();
    updateMotorTest();
    updateMotorRecord();

    // compass.updateFusion();  // Tắt I2C
    
    // ✅ Gửi SPEED mỗi 500ms để realtime
    static unsigned long lastSpeedSend = 0;
    if (millis() - lastSpeedSend > 500) {
        lastSpeedSend = millis();
        
        // Gửi tốc độ motor qua UDP từ 3_motor system
        sendSpeed((int16_t)m1.rpmFilt, (int16_t)m2.rpmFilt, (int16_t)m3.rpmFilt);
    }

    // compass.update();  // Tắt I2C
    
    delay(10);  // Giảm delay xuống 10ms để responsive hơn
}