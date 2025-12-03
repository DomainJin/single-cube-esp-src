#ifndef IR_H
#define IR_H

#include <Arduino.h>

// Định nghĩa chân
// #define REV_LED_PIN 25      // Chân DAC1 (GPIO25) - revLedOut
// #define TRAN_LED_PIN 26     // Chân DAC2 (GPIO26) - tranLedOut
#define ANALOG_READ_PIN_1 34  // Chân ADC (GPIO34) - analogRead
#define ANALOG_READ_PIN_2 35  // Chân ADC (GPIO35) - analogRead

// Định nghĩa điện áp tối đa
#define MAX_VOLTAGE 3.3

// Định nghĩa enum cho trạng thái Face
enum FaceStatus {
    None,
    UP,
    DOWN,
    TOUCH
};

// Cấu trúc statusIR
struct StatusIR {
    struct {
        int _1;  // Trạng thái pin 1 (0 hoặc 1)
        int _2;  // Trạng thái pin 2 (0 hoặc 1)
        int _3;  // Trạng thái pin 3 (0 hoặc 1)
        int _4;  // Trạng thái pin 4 (0 hoặc 1)
        int _5;  // Trạng thái pin 5 (0 hoặc 1)
        int _6;  // Trạng thái pin 6 (0 hoặc 1)
    } Pin;
    
    struct {
        FaceStatus _1;  // Trạng thái mặt 1 (UP, DOWN, TOUCH)
        FaceStatus _2;  // Trạng thái mặt 2 (UP, DOWN, TOUCH)
        FaceStatus _3;  // Trạng thái mặt 3 (UP, DOWN, TOUCH)
        FaceStatus _4;  // Trạng thái mặt 4 (UP, DOWN, TOUCH)
        FaceStatus _5;  // Trạng thái mặt 5 (UP, DOWN, TOUCH)
        FaceStatus _6;  // Trạng thái mặt 6 (UP, DOWN, TOUCH)
    } Face;
};

// Biến global statusIR
extern StatusIR statusIR;

// Khai báo hàm
void initIR();
void revLedOut(float voltage);
void tranLedOut(float voltage);
void handleIRModule();
float analogReadVoltage(int pin);          // Đọc voltage từ chân 35 (0-3.3V)
uint16_t analogReadRaw(int pin);           // Đọc ADC raw từ chân 35 (0-4095)
uint16_t voltageToDACValue(float voltage);
float adcValueToVoltage(uint16_t adcValue);

#endif