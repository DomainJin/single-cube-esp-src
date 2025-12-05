#include "ipconfig.h"
#include "udpconfig.h"  // Để dùng sendUDPPacket với priority
#include "main.h"       // Để dùng SERVER_IP và SERVER_PORT

// ===== GLOBAL IP CONFIG OBJECTS =====
WiFiUDP heartbeat_udp;
IPAddress heartbeat_server_address;

// ===== DEVICE CONFIGURATION =====
String device_name = "";  // Sử dụng String để cập nhật động
int heartbeat_local_port = 0;  // ✅ Port động, sẽ được tính trong initIPConfig()

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
    
    // ✅ Tính heartbeat port động: 8000 + lastOctet
    // Ví dụ: IP 192.168.1.14 → port 8014
    //        IP 192.168.1.13 → port 8013
    heartbeat_local_port = 8000 + lastOctet;

    // Khởi tạo UDP cho heartbeat với port động
    if (!heartbeat_udp.begin(heartbeat_local_port)) {
        Serial.printf("[IPCONFIG] Lỗi: Không thể khởi tạo UDP heartbeat trên port %d!\n", heartbeat_local_port);
        return false;
    }
    
    // Thiết lập địa chỉ Heartbeat Server
    heartbeat_server_address.fromString(SERVER_IP);
    
    // Sử dụng device_name thay vì MAC address
    esp_identifier = device_name;
    
    ipconfig_initialized = true;
    lastHeartbeatTime = 0;  // Gửi heartbeat ngay lần đầu
    
    Serial.printf("[IPCONFIG] ✅ Heartbeat Local Port: %d (DYNAMIC - No Conflict!)\n", heartbeat_local_port);
    Serial.printf("[IPCONFIG] Heartbeat Server: %s:%d\n", SERVER_IP, SERVER_PORT);
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
    
    // Gửi với priority CRITICAL và immediate = true
    heartbeat_udp.beginPacket(heartbeat_server_address, SERVER_PORT);
    heartbeat_udp.write((uint8_t*)heartbeatMessage, strlen(heartbeatMessage));
    heartbeat_udp.endPacket();
    
    Serial.printf("[HEARTBEAT] Gửi: %s -> %s:%d\n", 
                 heartbeatMessage, SERVER_IP, SERVER_PORT);
}

void handleHeartbeat() {
    // Kiểm tra WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HEARTBEAT] WARNING: WiFi disconnected! Attempting reconnect...");
        // Thử reconnect
        WiFi.reconnect();
        return;
    }
    
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
    
    heartbeat_udp.beginPacket(heartbeat_server_address, SERVER_PORT);
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
    
    heartbeat_udp.beginPacket(heartbeat_server_address, SERVER_PORT);
    heartbeat_udp.write((uint8_t*)statusMessage, strlen(statusMessage));
    heartbeat_udp.endPacket();
    
    Serial.printf("[IPCONFIG] Gửi status: %s\n", statusMessage);
}