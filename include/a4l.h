#ifndef A4H_H
#define A4H_H
#include <main.h>

#define Button_Next_Pin 25
#define Button_SyncMode 26
#define Button_HDMI_Mode 12  // ✅ Đổi từ GPIO 27 (swap với Motor 1 Encoder B)
// ===== A4L FUNCTIONS =====

void a4lInit();
void a4lSyncModeEnable();
void a4lNext();
void a4lHDMIModeEnable();

#endif