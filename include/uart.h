#ifndef UART_H
#define UART_H

#include <Arduino.h>
#include <HardwareSerial.h>

// ===== UART CONFIGURATION =====

// UART pins - Schematic shows TX on MCU side (pin 23) and RX (pin 32)
// Standard ESP32 UART2: GPIO 16 (RX), GPIO 17 (TX) OR custom pins with UART2
// Based on schematic layout: adjusting to GPIO 1 (TX), GPIO 3 (RX) for UART0 or custom UART2
#define UART_TX_PIN 15   // TX pin (gửi dữ liệu đến STM32) - Schematic pin
#define UART_RX_PIN 19   // RX pin (nhận dữ liệu ADC từ STM32) - Schematic pin
#define UART_BAUD 112500  // Baud rate
#define UART_PORT 1     // UART0 (ESP32 có 3 UART: 0, 1, 2)

// Buffer sizes
#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256

// ===== UART GLOBALS =====

extern HardwareSerial* pic_serial;

// ===== UART FUNCTIONS =====

/**
 * @brief Khởi tạo UART module
 * @return true nếu khởi tạo thành công
 */
bool initUART();

/**
 * @brief Kiểm tra có dữ liệu nhận từ STM32 không
 * @return true nếu có dữ liệu
 */
bool uartAvailable();

/**
 * @brief Đọc một ký tự từ UART
 * @return Ký tự nhận, -1 nếu không có dữ liệu
 */
int uartRead();

/**
 * @brief Đọc một dòng dữ liệu (kết thúc bằng newline)
 * @return String nhận được
 */
String uartReadLine();

/**
 * @brief Gửi dữ liệu đến STM32
 * @param data Dữ liệu cần gửi
 */
void uartSend(const char* data);

/**
 * @brief Gửi dòng dữ liệu đến STM32 (kèm newline)
 * @param data Dữ liệu cần gửi
 */
void uartSendLine(const char* data);

/**
 * @brief Kiểm tra trạng thái UART
 * @return true nếu UART sẵn sàng
 */
bool isUARTReady();

/**
 * @brief In thông tin UART ra Serial Monitor
 */
void printUARTStatus();

/**
 * @brief Xử lý dữ liệu UART từ STM32 (ADC frame parsing)
 */
void handleUARTData();

// ===== STM32 ADC DATA FUNCTIONS =====

/**
 * @brief Lấy giá trị ADC từ channel (1-6)
 * @param channel Kênh ADC (1-6)
 * @return Giá trị ADC 12-bit (0-4095)
 */
int16_t getADC(uint8_t channel);

/**
 * @brief Lấy tất cả 6 giá trị ADC
 * @param values Mảng 6 phần tử để lưu giá trị ADC
 */
void getADCValues(int16_t* values);

/**
 * @brief Gửi dữ liệu buffer đến STM32 qua UART
 * @param data Con trỏ đến dữ liệu
 * @param len Độ dài dữ liệu
 */
void sendUARTData(const uint8_t* data, uint16_t len);

/**
 * @brief Gửi lệnh đến STM32 qua UART
 */
void sendUARTCommand(const String& command);

// COMMENTED - PIC Touch-related functions
/*
int getRawTouch(const String& data);
int getThreshold(const String& data);
int getValue(const String& data);
int getValue();
int getThreshold();
int getRawTouch();
void processTouchEvent(int currentTouchState);
bool isTouchActive();
bool isTouchHold();
bool isTouchReleased();
unsigned long getTouchDuration();
unsigned long getTouchStartTime();
void clearTouchReleased();
*/

int16_t getADC(uint8_t channel);
void getADCValues(int16_t* values);
#endif // UART_H
