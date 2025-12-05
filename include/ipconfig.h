#ifndef IPCONFIG_H
#define IPCONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ===== IP CONFIG CONFIGURATION =====
// Server IP và Port lấy từ main.h (SERVER_IP, SERVER_PORT)
#define HEARTBEAT_INTERVAL 1000  // ✅ 1 giây (gửi thường xuyên hơn để tránh timeout)

// ===== IP CONFIG FUNCTIONS =====
bool initIPConfig();
void sendHeartbeat();
void handleHeartbeat();
bool isIPConfigReady();
String getESPIdentifier();

#endif