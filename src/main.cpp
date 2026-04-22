#include <Arduino.h>
#include "main.h"
// #include "mpu6050.h"  // Comment vì xung đột I2C
#include "qmc5883l.h"
#include "a4l.h"
#include "3_motor.h"


// Cấu hình WiFi
const char* ssid = "Cube Touch";
const char* password = "admin123";
// const char* ssid = "SGM";
// const char* password = "19121996";

// Cấu hình Server - Chung cho tất cả modules
const char* SERVER_IP = "192.168.0.202";  // IP server nhận UDP
int SERVER_PORT = 1509;                   // Port server nhận UDP

WiFiUDP udp;
IPAddress resolume_address;

// Khởi tạo MPU6050
// MPU6050 mpu;  // Comment vì xung đột I2C

// Khởi tạo QMC5883L (L883 chip)
QMC5883L compass;


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
    
    // ✅ Khởi tạo I2C bus cho QMC5883L
    Wire.begin(3, 22);  // SDA=3, SCL=22
    Serial.println("[SETUP] I2C initialized: SDA=3, SCL=22");
    delay(100);
    // ===== I2C SCANNER =====
    Serial.println("\n[I2C SCAN] Scanning I2C bus...");
    int nDevices = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("[I2C SCAN] Device found at 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            
            // Nhận dạng thiết bị
            if (address == 0x68 || address == 0x69) Serial.print(" <- MPU9250");
            if (address == 0x0C) Serial.print(" <- AK8963 (magnetometer)");
            if (address == 0x0D) Serial.print(" <- QMC5883L");
            
            Serial.println();
            nDevices++;
        }
    }
    if (nDevices == 0) {
        Serial.println("[I2C SCAN] No I2C devices found!");
        Serial.println("[I2C SCAN] Check wiring: SDA=3, SCL=22");
        Serial.println("[I2C SCAN] Check pull-up resistors (4.7kΩ)");
    } else {
        Serial.printf("[I2C SCAN] Found %d device(s)\n", nDevices);
    }
    Serial.println("[I2C SCAN] Scan complete\n");
    
    // ✅ Khởi tạo MPU9250 + AK8963 Compass
    if (compass.begin(3, 22)) {
        compass.setUpdateInterval(100);  // Cập nhật mỗi 100ms (10Hz) để responsive hơn
        compass.enable();                // Bật module
        Serial.println("[SETUP] MPU9250 compass initialized!");
    } else {
        Serial.println("[SETUP] MPU9250 initialization FAILED!");
        Serial.println("[SETUP] Possible issues:");
        Serial.println("  - MPU9250 module not connected");
        Serial.println("  - Wrong I2C address (check if 0x68 or 0x69)");
        Serial.println("  - Missing pull-up resistors");
        Serial.println("  - Faulty module or wiring");
    }
    
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

    // ✅ 9-DOF Sensor Fusion - Cập nhật liên tục
    compass.updateFusion();
    
    // ✅ Bù trượt từ sensor fusion (KHÔNG ghi đè lệnh UDP)
    static unsigned long lastFusionApply = 0;
    if (millis() - lastFusionApply > 200) {  // 5Hz update rate - giảm can thiệp
        lastFusionApply = millis();
        
        float vx, vy, omega;
        compass.getVelocities(vx, vy, omega);
        
        // Chỉ bù trượt, không điều khiển chính
        // Gain nhỏ (5%) để không làm ảnh hưởng lệnh UDP
        omniCompensateDrift(vx, vy, omega, 0.05f);
    }
    
    // ✅ Gửi SPEED mỗi 500ms để realtime
    static unsigned long lastSpeedSend = 0;
    if (millis() - lastSpeedSend > 500) {
        lastSpeedSend = millis();
        
        // Gửi tốc độ motor qua UDP từ 3_motor system
        sendSpeed((int16_t)m1.rpmFilt, (int16_t)m2.rpmFilt, (int16_t)m3.rpmFilt);
    }

    // ✅ QMC5883L COMPASS - Tự động đọc và gửi dữ liệu
    compass.update();
    
    delay(10);  // Giảm delay xuống 10ms để responsive hơn
}