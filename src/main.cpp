#include <Arduino.h>
#include "main.h"


// Cấu hình WiFi
const char* ssid = "Cube Touch";
const char* password = "admin123";

WiFiUDP udp;
IPAddress resolume_address;


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
    
    // Cấu hình pin IO15 cho xi lanh
    pinMode(2, OUTPUT);
    pinMode(15, OUTPUT);
    
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
    initIR();
    initIPConfig();  // Thêm dòng này
    
    Serial.println("Tất cả modules đã sẵn sàng!");
    
    // Gửi heartbeat đầu tiên
    sendESPStatus("READY");
}

void loop() {
    handleUARTData();
    handleUDPReceive();
    handleHeartbeat();  // Thêm dòng này
    
    float inputVoltage = analogReadVoltage();
    uint16_t rawValue = analogReadRaw();
    sendIRADCRaw(rawValue);
    delay(100);
}