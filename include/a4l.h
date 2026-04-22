#ifndef A4H_H
#define A4H_H
#include <main.h>

// 📍 CỤM A4L RELAY CONTROL - From original documentation
// GPIO 32 = NEXT, GPIO 33 = SyncMode, GPIO 23 = HDMI (from GPIO_PINOUT_CLUSTERS.md)
#define Button_Next_Pin 23      // GPIO 32 - A4L Next Button
#define Button_SyncMode 33      // GPIO 33 - A4L Sync Mode
#define Button_HDMI_Mode 32     // GPIO 23 - A4L HDMI Mode
// ===== A4L FUNCTIONS =====
    
void a4lInit();
void a4lSyncModeEnable();
void a4lNext();
void a4lHDMIModeEnable();

#endif