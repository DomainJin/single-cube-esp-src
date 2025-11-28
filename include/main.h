#ifndef MAIN_H
#define MAIN_H

// ===== INCLUDES =====

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "led.h"
#include "osc.h"
#include "uart.h"
#include "udpconfig.h"
#include "IR.h"
#include "ipconfig.h"

// ===== CONFIGURATION =====

// WiFi Configuration
extern const char* ssid;
extern const char* password;


/**
 * @brief Tạo OSC message packet thủ công
 * @param buffer Buffer để lưu OSC packet
 * @param length Pointer để trả về độ dài packet
 * @param address OSC address string (ví dụ: "/composition/layers/1/clips/1/connect")
 * @param value Giá trị float để gửi
 */
void createOSCMessage(uint8_t* buffer, int* length, const char* address, float value);

/**
 * @brief Gửi OSC message đến Resolume
 * @param address OSC address string
 * @param value Giá trị float để gửi
 */
void sendOSC(const char* address, float value);

/**
 * @brief Khởi tạo hệ thống (được gọi 1 lần khi khởi động)
 */
void setup();

/**
 * @brief Vòng lặp chính của chương trình
 */
void loop();

// ===== HELPER FUNCTIONS =====

/**
 * @brief Kiểm tra trạng thái kết nối WiFi
 * @return true nếu WiFi đã kết nối
 */
inline bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

/**
 * @brief Lấy địa chỉ IP cục bộ của ESP32
 * @return IPAddress của ESP32
 */
inline IPAddress getLocalIP() {
    return WiFi.localIP();
}

/**
 * @brief Lấy RSSI (cường độ tín hiệu WiFi)
 * @return RSSI value in dBm
 */
inline int getWiFiRSSI() {
    return WiFi.RSSI();
}

// IP Config functions
bool initIPConfig();
void sendHeartbeat();
void handleHeartbeat();
bool isIPConfigReady();
String getESPIdentifier();
void sendCustomHeartbeat(const String& customMessage);
void sendESPStatus(const String& status);

// ===== CONSTANTS =====

// Delay time cho loop chính
#define MAIN_LOOP_DELAY_MS 100

// Delay giữa các lệnh OSC
#define OSC_COMMAND_DELAY_MS 100

// Buffer size cho OSC messages
#define OSC_BUFFER_SIZE 256

#endif // MAIN_H