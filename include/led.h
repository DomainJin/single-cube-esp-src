#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// ===== LED CONFIGURATION =====
#define LED_PIN 5
#define NUM_LEDS 150
#define UPDATE_INTERVAL 100  // ms

// ===== LED CONTROL VARIABLES =====
extern Adafruit_NeoPixel strip;
extern bool effectEnable;
extern int brightness;
extern int currentLedCount;
extern int ledDirection;
extern unsigned long lastUpdateTime;

// ===== LED FUNCTION DECLARATIONS =====
void initLED();
void applyColorWithBrightness(bool turnOn, int r, int g, int b);
void setLEDBrightness(int newBrightness);
void setLEDDirection(int direction);
void enableLEDEffect(bool enable);
void clearAllLEDs();
void setAllLEDs(int r, int g, int b);
void setLEDRange(int start, int end, int r, int g, int b);
void ledHandle();
// ===== LED UTILITY FUNCTIONS =====
bool isLEDReady();
int getCurrentLEDCount();
int getLEDBrightness();
int getLEDDirection();
bool isEffectEnabled();

#endif // LED_H