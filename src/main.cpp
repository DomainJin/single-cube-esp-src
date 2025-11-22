#include <Arduino.h>
#include "main.h"
#include "osc.h"
#include "uart.h"
#include "udpconfig.h"

// Cấu hình WiFi
const char* ssid = "Cube Touch";
const char* password = "admin123";

// Cấu hình OSC
const char* resolume_ip = "192.168.0.241";
const int resolume_port = 7000;

// Cấu hình OSC
const char* server_ip = "192.168.0.202";
const int server_port = 7000;

WiFiUDP udp;
IPAddress resolume_address;

bool touchActive = false;
unsigned long touchDuration = 0;

int valuetouch = 0;


void setup() {
  Serial.begin(115200);
  Serial.println("\nKhởi động ESP32 OSC Client...");
  
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
  
  // Khởi tạo OSC module
  initOSC();
  
  // Khởi tạo UART module
  initUART();
  
  // Khởi tạo UDP Touch module
  initUDPTouch();
  
  Serial.println("Sẵn sàng gửi OSC đến Resolume!");
  Serial.println("Nhấn phím bất kỳ trong Serial Monitor để gửi lệnh OSC");
  Serial.println("Lệnh: status | test | send:DATA | hoặc Enter");
}

void loop() {
    // Xử lý dữ liệu UART từ PIC
    handleUARTData();
    valuetouch = getValue();
    // Serial.println("value: " + String(valuetouch));
    int threshold = getThreshold();
    Serial.println("threshold: " + String(threshold));
    // touchActive = isTouchActive();
    // touchDuration = getTouchDuration();
    // Serial.printf("Touch Active: %s, Duration: %lu ms\n", touchActive ? "YES" : "NO", touchDuration);
    
    // Gửi touch value qua UDP đến server
    // sendTouchValue(valuetouch);
    // delay(100); // Tránh spam quá nhiều

    // delay(1000);
    // sendResolumeInit();
    // delay(1000);
    // sendResolumeEnable();
    // delay(1000);
    // sendResolumeBack(5);
    // delay(1000);
    // sendResolumeMain();
    // delay(5000);

    
   
    
    sendUARTCommand("E");
    delay(3000);
}