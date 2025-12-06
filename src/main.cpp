#include <Arduino.h>
#include "main.h"
// #include "mpu6050.h"  // Comment vì xung đột I2C
#include "qmc5883l.h"
#include "a4l.h"


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
    Serial.begin(115200);
    Serial.println("\n===== ESP32 WS2812 TEST =====");
    
    // ✅ Khởi tạo LED WS2812 đầu tiên
    initLED();
    
    Serial.println("[SETUP] WS2812 hoàn thành!");
    
    
    
    // Kết nối WiFi
    WiFi.begin(ssid, password);
    Serial.print("Đang kết nối WiFi");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
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
    // initUART();
    initUDPTouch();
    initIR();// Cấu hình pin IO15 cho xi lanh
    

    a4lInit();

    initIPConfig();  // Thêm dòng này
    
    // ✅ Khởi tạo Motor Control System
    setupMotors();
    Serial.println("[SETUP] Motor control system initialized!");
    
    // ✅ Khởi tạo Omni Robot System
    setupOmni();
    Serial.println("[SETUP] Omni robot system initialized!");
    
    // Khởi tạo MPU6050
    // if (mpu.begin()) {
    //     Serial.println("MPU6050 đã khởi tạo thành công!");
    // } else {
    //     Serial.println("Lỗi: Không thể khởi tạo MPU6050!");
    // }
    
    // TEST I2C SCAN TRƯỚC KHI KHỞI TẠO COMPASS
    Serial.println("\n===== I2C SCANNER TEST =====");
    Wire.begin(16, 17);  // SDA=16, SCL=17
    Wire.setClock(100000); // 100kHz
    delay(200);
    
    Serial.println("Scanning I2C bus (0x00-0x7F)...");
    int deviceCount = 0;
    
    for (uint8_t address = 0; address < 128; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("  ==> Device found at 0x%02X", address);
            if (address == 0x0D) Serial.print(" (QMC5883L!)");
            Serial.println();
            deviceCount++;
        }
        delay(5);
    }
    
    if (deviceCount == 0) {
        Serial.println("\n!!! NO I2C DEVICES FOUND !!!");
    } else {
        Serial.printf("\nTotal: %d device(s) found\n", deviceCount);
    }
    Serial.println("=============================\n");
    
    // Khởi tạo QMC5883L
    if (compass.begin(16, 17)) {  // SDA=GPIO16, SCL=GPIO17
        Serial.println("QMC5883L ready!");
        compass.setDeclination(0.5);  // Vietnam ~0.5°
    } else {
        Serial.println("Failed to initialize QMC5883L!");
    }
    
    Serial.println("Tất cả modules đã sẵn sàng!");
    
    // Gửi heartbeat đầu tiên
    sendESPStatus("READY");
}

void loop() {
    // ✅ Kiểm tra WiFi connection mỗi 10 giây
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 10000) {
        lastWiFiCheck = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WIFI] WARNING: WiFi disconnected! Reconnecting...");
            WiFi.reconnect();
        } else {
            // Chỉ log RSSI nếu yếu (< -70 dBm)
            int rssi = WiFi.RSSI();
            if (rssi < -70) {
                Serial.printf("[WIFI] Weak signal: %d dBm\\n", rssi);
            }
        }
    }
    
    // handleUARTData();
    handleUDPReceive();
    processUDPQueue();  // Xử lý hàng đợi UDP
    processRecalibration();  // ✅ Xử lý RECALIB không blocking
    handleHeartbeat();
    handleIRModule();
    
    // ✅ Update Omni Robot (MUST BE CALLED IN LOOP!)
    updateOmni();
    
    // ✅ Gửi SPEED mỗi 500ms để realtime
    static unsigned long lastSpeedSend = 0;
    if (millis() - lastSpeedSend > 500) {
        lastSpeedSend = millis();
        
        // ✅ Lấy RPM từ omni_state thay vì gọi getMotorRPM() lại (tránh conflict)
        OmniRobotState* omni = getOmniState();
        float rpm1 = omni->wheel_rpm[0];
        float rpm2 = omni->wheel_rpm[1];
        float rpm3 = omni->wheel_rpm[2];
        
        // Gửi tốc độ motor qua UDP
        sendSpeed((int16_t)rpm1, (int16_t)rpm2, (int16_t)rpm3);
    }
    
    // ===== TEST QMC5883L =====
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead > 500) {  // Đọc mỗi 500ms
        lastSensorRead = millis();
        
        // Đọc magnetometer từ QMC5883L
        int16_t mx, my, mz;
        if (compass.readMag(&mx, &my, &mz)) {
            // Tính heading
            float heading = compass.getHeading();
            
            // Lấy hướng cardinal (N, NE, E, ...)
            String direction = compass.getCardinalDirection();
            
            // Gửi dữ liệu qua UDP
            sendCompassHeading(heading, direction.c_str());  // Gửi heading + direction
            sendCompassRaw(mx, my, mz);                      // Gửi raw mag data
            
            // In kết quả
            Serial.println("========================================");
            Serial.printf("QMC5883L Mag: X=%d, Y=%d, Z=%d\n", mx, my, mz);
            Serial.printf("Heading: %.1f°\n", heading);
            Serial.printf("Direction: %s\n", direction.c_str());
            Serial.println("========================================");
        }
    }
    
    
    
    delay(10);  // Giảm delay xuống 10ms để responsive hơn
}