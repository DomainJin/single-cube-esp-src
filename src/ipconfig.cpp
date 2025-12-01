#include "ipconfig.h"

// ===== GLOBAL IP CONFIG OBJECTS =====
WiFiUDP heartbeat_udp;
IPAddress heartbeat_server_address;

// ===== DEVICE CONFIGURATION =====
String device_name = "";  // Sử dụng String để cập nhật động

// ===== HEARTBEAT VARIABLES =====
static bool ipconfig_initialized = false;
static unsigned long lastHeartbeatTime = 0;
static String esp_identifier = "";

// ===== IP CONFIG INITIALIZATION =====
bool initIPConfig() {
    Serial.println("[IPCONFIG] Khởi tạo IP Config module...");
    
    // Kiểm tra WiFi trước khi khởi tạo UDP
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[IPCONFIG] Lỗi: WiFi chưa kết nối!");
        return false;
    }
    
    // Tạo device_name tự động theo IP
    IPAddress localIP = WiFi.localIP();
    uint8_t lastOctet = localIP[3];
    device_name = "Cube " + String(lastOctet);

    // Khởi tạo UDP cho heartbeat
    if (!heartbeat_udp.begin(HEARTBEAT_LOCAL_PORT)) {
        Serial.println("[IPCONFIG] Lỗi: Không thể khởi tạo UDP heartbeat!");
        return false;
    }
    
    // Thiết lập địa chỉ Heartbeat Server
    heartbeat_server_address.fromString(HEARTBEAT_SERVER_IP);
    
    // Sử dụng device_name thay vì MAC address
    esp_identifier = device_name;
    
    ipconfig_initialized = true;
    lastHeartbeatTime = 0;  // Gửi heartbeat ngay lần đầu
    
    Serial.printf("[IPCONFIG] Local UDP Port: %d\n", HEARTBEAT_LOCAL_PORT);
    Serial.printf("[IPCONFIG] Heartbeat Server: %s:%d\n", HEARTBEAT_SERVER_IP, HEARTBEAT_SERVER_PORT);
    Serial.printf("[IPCONFIG] Device Name: %s\n", device_name.c_str());
    Serial.printf("[IPCONFIG] ESP Identifier: %s\n", esp_identifier.c_str());
    Serial.printf("[IPCONFIG] Local IP: %s\n", localIP.toString().c_str());
    Serial.println("[IPCONFIG] IP Config module sẵn sàng!");
    
    return true;
}

// ===== IP CONFIG FUNCTIONS =====
bool isIPConfigReady() {
    return ipconfig_initialized && (WiFi.status() == WL_CONNECTED);
}

String getESPIdentifier() {
    return esp_identifier;
}

void sendHeartbeat() {
    if (!isIPConfigReady()) {
        return;
    }
    
    String localIP = WiFi.localIP().toString();
    char heartbeatMessage[128];
    snprintf(heartbeatMessage, sizeof(heartbeatMessage), 
             "HEARTBEAT:%s,IP:%s,HELLO", 
             device_name.c_str(),
             localIP.c_str());
    
    heartbeat_udp.beginPacket(heartbeat_server_address, HEARTBEAT_SERVER_PORT);
    heartbeat_udp.write((uint8_t*)heartbeatMessage, strlen(heartbeatMessage));
    heartbeat_udp.endPacket();
    
    Serial.printf("[IPCONFIG] Gửi heartbeat: %s -> %s:%d\n", 
                 heartbeatMessage, HEARTBEAT_SERVER_IP, HEARTBEAT_SERVER_PORT);
}

void handleHeartbeat() {
    if (!isIPConfigReady()) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
        sendHeartbeat();
        lastHeartbeatTime = currentTime;
    }
}

void sendCustomHeartbeat(const String& customMessage) {
    if (!isIPConfigReady()) {
        return;
    }
    
    String localIP = WiFi.localIP().toString();
    char heartbeatMessage[128];
    snprintf(heartbeatMessage, sizeof(heartbeatMessage), 
             "HEARTBEAT:%s,IP:%s,%s", 
             device_name.c_str(),
             localIP.c_str(),
             customMessage.c_str());
    
    heartbeat_udp.beginPacket(heartbeat_server_address, HEARTBEAT_SERVER_PORT);
    heartbeat_udp.write((uint8_t*)heartbeatMessage, strlen(heartbeatMessage));
    heartbeat_udp.endPacket();
    
    Serial.printf("[IPCONFIG] Gửi custom heartbeat: %s\n", heartbeatMessage);
}

void sendESPStatus(const String& status) {
    if (!isIPConfigReady()) {
        return;
    }
    
    String localIP = WiFi.localIP().toString();
    char statusMessage[128];
    snprintf(statusMessage, sizeof(statusMessage), 
             "STATUS:%s,IP:%s,%s", 
             device_name.c_str(),
             localIP.c_str(),
             status.c_str());
    
    heartbeat_udp.beginPacket(heartbeat_server_address, HEARTBEAT_SERVER_PORT);
    heartbeat_udp.write((uint8_t*)statusMessage, strlen(statusMessage));
    heartbeat_udp.endPacket();
    
    Serial.printf("[IPCONFIG] Gửi status: %s\n", statusMessage);
}