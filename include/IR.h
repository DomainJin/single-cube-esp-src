#ifndef IR_H
#define IR_H

#include <Arduino.h>

// Định nghĩa chân
#define REV_LED_PIN 25      // Chân DAC1 (GPIO25) - revLedOut
#define TRAN_LED_PIN 26     // Chân DAC2 (GPIO26) - tranLedOut
#define ANALOG_READ_PIN 34  // Chân ADC (GPIO34) - analogRead

// Định nghĩa điện áp tối đa
#define MAX_VOLTAGE 3.3

// Khai báo hàm
void initIR();
void revLedOut(float voltage);
void tranLedOut(float voltage);
float analogReadVoltage();          // Đọc voltage từ chân 35 (0-3.3V)
uint16_t analogReadRaw();           // Đọc ADC raw từ chân 35 (0-4095)
uint16_t voltageToDACValue(float voltage);
float adcValueToVoltage(uint16_t adcValue);

#endif