#ifndef UART_H
#define UART_H

#include <Arduino.h>
#include <HardwareSerial.h>

// ===== UART CONFIGURATION =====

// UART pins
#define UART_RX_PIN 4  // RX pin (nhận dữ liệu từ PIC)
#define UART_TX_PIN 5  // TX pin (gửi dữ liệu đến PIC)
#define UART_BAUD 115200  // Baud rate
#define UART_PORT 2     // UART2 (ESP32 có 3 UART: 0, 1, 2)

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
 * @brief Kiểm tra có dữ liệu nhận từ PIC không
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
 * @brief Gửi dữ liệu đến PIC
 * @param data Dữ liệu cần gửi
 */
void uartSend(const char* data);

/**
 * @brief Gửi dòng dữ liệu đến PIC (kèm newline)
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
 * @brief Xử lý lệnh UART trong loop
 */
void handleUARTData();

/**
 * @brief Xử lý lệnh UART trong loop
 */
int getRawTouch(const String& data);

/**
 * @brief Xử lý lệnh UART trong loop
 */
int getThreshold(const String& data);

/**
 * @brief Xử lý lệnh UART trong loop
 */
int getValue(const String& data);

int getValue();
int getThreshold();
int getRawTouch();

/**
 * @brief Xử lý lệnh UART trong loop
 */
void processTouchEvent(int currentTouchState);

/**
 * @brief Xử lý lệnh UART trong loop
 */
bool isTouchActive();

/**
 * @brief Xử lý lệnh UART trong loop
 */
bool isTouchHold();

/**
 * @brief Xử lý lệnh UART trong loop
 */
bool isTouchReleased();

/**
 * @brief Xử lý lệnh UART trong loop
 */
unsigned long getTouchDuration();

/**
 * @brief Xử lý lệnh UART trong loop
 */
unsigned long getTouchStartTime();

/**
 * @brief Xử lý lệnh UART trong loop
 */
void clearTouchReleased();

/**
 * @brief Gửi lệnh đến PIC qua UART
 */
void sendUARTCommand(const String& command);

#endif // UART_H
