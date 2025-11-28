#ifndef IPCONFIG_H
#define IPCONFIG_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// ===== IP CONFIG CONFIGURATION =====
#define HEARTBEAT_SERVER_IP "192.168.0.159"
#define HEARTBEAT_SERVER_PORT 1509
#define HEARTBEAT_LOCAL_PORT 8002
#define HEARTBEAT_INTERVAL 1000  // 1 giây

// ===== IP CONFIG FUNCTIONS =====
bool initIPConfig();
void sendHeartbeat();
void handleHeartbeat();
bool isIPConfigReady();
String getESPIdentifier();

#endif