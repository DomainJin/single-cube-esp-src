#include <Arduino.h>
#include "main.h"
#include "mpu6050.h"


// Cấu hình WiFi
// const char* ssid = "Cube Touch";
// const char* password = "admin123";
const char* ssid = "SGM";
const char* password = "19121996";

WiFiUDP udp;
IPAddress resolume_address;

// Khởi tạo MPU6050
MPU6050 mpu;


bool sendEnableOnce = false;
bool sendBackOnce = false;
bool sendMainOnce = false;
bool sendInitOnce = false;
int mainRunMillis = 0;

int valuetouch = 0;
int operationTime = 2000;    // 2 giây để kích hoạt main effect
int mainEffectime = 6000;    // 6 giây main effect chạy



void setup() {
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
    
    Serial.println("");
    Serial.println("WiFi đã kết nối!");
    Serial.print("Địa chỉ IP: ");
    Serial.println(WiFi.localIP());
    
    // Khởi tạo các modules
    initOSC();
    initUART();
    initUDPTouch();
    initIR();// Cấu hình pin IO15 cho xi lanh
    pinMode(17, OUTPUT);
    digitalWrite(17, LOW);
    pinMode(18, OUTPUT);
    digitalWrite(18, LOW);
    initIPConfig();  // Thêm dòng này
    
    // Khởi tạo MPU6050
    if (mpu.begin()) {
        Serial.println("MPU6050 đã khởi tạo thành công!");
    } else {
        Serial.println("Lỗi: Không thể khởi tạo MPU6050!");
    }
    
    Serial.println("Tất cả modules đã sẵn sàng!");
    
    // Gửi heartbeat đầu tiên
    sendESPStatus("READY");
}

void loop() {
    handleUARTData();
    handleUDPReceive();
    handleHeartbeat();
    
    float inputVoltage = analogReadVoltage();
    uint16_t rawValue = analogReadRaw();
    sendIRADCRaw(rawValue);
    
    // Đọc và in giá trị MPU6050
    // int16_t x, y, z;
    // mpu.readAccel(&x, &y, &z);
    
    // Serial.print("MPU6050 - X: ");
    // Serial.print(x);
    // Serial.print(" | Y: ");
    // Serial.print(y);
    // Serial.print(" | Z: ");
    // Serial.println(z);
    
    // Hoặc in theo đơn vị g
    float gx, gy, gz;
    mpu.readAccelG(&gx, &gy, &gz);
    Serial.print("MPU6050 (g) - X: ");
    Serial.print(gx);
    Serial.print("g | Y: ");
    Serial.print(gy);
    Serial.print("g | Z: ");
    Serial.print(gz);
    Serial.println("g");
    
    delay(100);
}