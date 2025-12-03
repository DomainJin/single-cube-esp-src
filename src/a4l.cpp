#include "a4l.h"
// ===== A4L FUNCTIONS =====
void a4lInit() {
    pinMode(Button_Next_Pin, OUTPUT);
    pinMode(Button_SyncMode, OUTPUT);
    pinMode(Button_HDMI_Mode, OUTPUT);
    digitalWrite(Button_Next_Pin, HIGH);
    digitalWrite(Button_SyncMode, HIGH);
    digitalWrite(Button_HDMI_Mode, HIGH);
    Serial.println("[A4L_INIT] A4L BUTTON initialized");
}

void a4lSyncModeEnable() {
    digitalWrite(Button_SyncMode, LOW);
    delay(100);
    digitalWrite(Button_SyncMode, HIGH);
    Serial.println("[A4L] Sync Mode PRESSED");
}

void a4lHDMIModeEnable() {
    digitalWrite(Button_HDMI_Mode, LOW);
    delay(100);
    digitalWrite(Button_HDMI_Mode, HIGH);
    Serial.println("[A4L] HDMI Mode PRESSED");
}

void a4lNext() {
    digitalWrite(Button_Next_Pin, LOW);
    delay(100);
    digitalWrite(Button_Next_Pin, HIGH);
    Serial.println("[A4L] NEXT button PRESSED");
}