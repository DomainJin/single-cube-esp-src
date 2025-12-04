#ifndef IPCONFIG_H
#define IPCONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ===== IP CONFIG CONFIGURATION =====
#define HEARTBEAT_SERVER_IP "192.168.1.150"
#define HEARTBEAT_SERVER_PORT 1509
// ❌ KHÔNG dùng port cố định nữa - sẽ tính động theo IP trong initIPConfig()
// #define HEARTBEAT_LOCAL_PORT 8002  
#define HEARTBEAT_INTERVAL 1000  // ✅ 1 giây (gửi thường xuyên hơn để tránh timeout)

// ===== IP CONFIG FUNCTIONS =====
bool initIPConfig();
void sendHeartbeat();
void handleHeartbeat();
bool isIPConfigReady();
String getESPIdentifier();

#endif