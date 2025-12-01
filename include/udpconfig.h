#ifndef UDP_CONFIG_H
#define UDP_CONFIG_H

#include <WiFi.h>
#include <WiFiUdp.h>

// ===== UDP TOUCH CONFIGURATION =====
extern const char* TOUCH_SERVER_IP;
extern int TOUCH_SERVER_PORT;
extern int LOCAL_TOUCH_PORT;

// ===== GLOBAL UDP TOUCH OBJECTS =====
extern WiFiUDP touch_udp;
extern IPAddress touch_server_address;

// ===== UDP TOUCH FUNCTION DECLARATIONS =====

/**
 * Khởi tạo UDP Touch module
 * @return true nếu khởi tạo thành công, false nếu thất bại
 */
bool initUDPTouch();

/**
 * Gửi touch value qua UDP đến server
 * @param touchMessage Chuỗi message cần gửi
 */
void sendTouchValue(const char* touchMessage);

/**
 * Gửi touch value dạng số nguyên qua UDP đến server (wrapper function)
 * @param touchValue Giá trị touch cần gửi
 */
void sendTouchValueInt(int touchValue);

/**
 * Gửi giá trị IR ADC qua UDP
 * @param adcRaw Giá trị ADC thô
 * @param adcVoltage Giá trị điện áp tương ứng
 */
void sendIRADCValue(uint16_t adcRaw, float adcVoltage);    // Gửi cả IR ADC raw và voltage

/**
 * Gửi giá trị ADC qua UDP
 * @param adcRaw Giá trị ADC thô
 * @param adcVoltage Giá trị điện áp tương ứng
 */
void sendIRADCRaw(uint16_t adcRaw);                        // Gửi chỉ IR ADC raw

/**
 * Gửi giá trị voltage qua UDP
 * @param voltage Giá trị điện áp
 */
void sendIRVoltage(float voltage);                         // Gửi chỉ IR voltage

/**
 * Gửi giá trị ADC qua UDP
 * @param adcRaw Giá trị ADC thô
 * @param adcVoltage Giá trị điện áp tương ứng
 */
void sendIRReceiveData(uint16_t adcRaw, float voltage);    // Gửi IR receive data

/**
 * Kiểm tra trạng thái kết nối UDP Touch
 * @return true nếu UDP Touch sẵn sàng, false nếu chưa kết nối
 */
bool isUDPTouchReady();

/**
 * Nhận dữ liệu UDP
 * @param buffer Buffer để chứa dữ liệu nhận được
 * @param bufferSize Kích thước buffer
 * @return Số bytes nhận được, 0 nếu không có dữ liệu, -1 nếu lỗi
 */
int receiveUDPData(char* buffer, int bufferSize);

/**
 * Kiểm tra có dữ liệu UDP đang chờ không
 * @return Số bytes có sẵn để đọc, 0 nếu không có dữ liệu
 */
int availableUDPData();

/**
 * Nhận và xử lý dữ liệu UDP (non-blocking)
 * Function này sẽ tự động parse và in ra Serial
 */
void handleUDPReceive();

#endif // UDP_CONFIG_H