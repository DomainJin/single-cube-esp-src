#ifndef A4H_H
#define A4H_H
#include <main.h>

#define Button_Next_Pin 25
#define Button_SyncMode 26
#define Button_HDMI_Mode 27
// ===== A4L FUNCTIONS =====

void a4lInit();
void a4lSyncModeEnable();
void a4lNext();
void a4lHDMIModeEnable();

#endif