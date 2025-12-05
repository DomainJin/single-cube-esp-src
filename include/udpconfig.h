#ifndef UDP_CONFIG_H
#define UDP_CONFIG_H

#include <WiFi.h>
#include <WiFiUdp.h>

// ===== UDP PRIORITY LEVELS =====
enum UDPPriority {
    UDP_PRIORITY_LOW = 0,      // IR ADC, Threshold (có thể bỏ qua)
    UDP_PRIORITY_NORMAL = 1,   // Touch value, general messages
    UDP_PRIORITY_HIGH = 2,     // Face status (quan trọng)
    UDP_PRIORITY_CRITICAL = 3  // System status, errors
};

// ===== UDP TOUCH CONFIGURATION =====
// SERVER_IP từ main.h
// TOUCH_SERVER_PORT tính theo IP (lastOctet * 100)
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
void sendIRADCRaw(int index, uint16_t adcRaw);                        // Gửi chỉ IR ADC raw

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
 * Gửi giá trị threshold qua UDP
 * @param index Chân mấy
 * @param threshold Giá trị threshold
 */
void sendIRThreshold(int index, uint16_t threshold);       // Gửi IR threshold

/**
 * Gửi trạng thái mặt qua UDP
 * @param faceNumber Số mặt (1-6)
 * @param status Trạng thái mặt ("UP", "DOWN", "TOUCH", "None")
 */
void sendStatusFace(int faceNumber, const char* status);   // Gửi trạng thái mặt

/**
 * Gửi hướng từ compass qua UDP (processed data)
 * @param heading Góc hướng (0-360 độ)
 * @param direction Hướng cardinal (N, NE, E, SE, S, SW, W, NW)
 * Format: "COMPASS:heading,offset,direction"
 * Ví dụ: "COMPASS:45.5,0.5,NE"
 */
void sendCompassHeading(float heading, const char* direction);

/**
 * Gửi dữ liệu thô từ magnetometer qua UDP (raw data)
 * @param mx Từ trường trục X
 * @param my Từ trường trục Y
 * @param mz Từ trường trục Z
 * Format: "MAG:mx,my,mz"
 * Ví dụ: "MAG:-147,-1430,110"
 */
void sendCompassRaw(int16_t mx, int16_t my, int16_t mz);

/**
 * Gửi tốc độ motor qua UDP
 * @param s1 Tốc độ motor 1
 * @param s2 Tốc độ motor 2
 * @param s3 Tốc độ motor 3
 * Format: "SPEED:s1,s2,s3"
 * Ví dụ: "SPEED:100,150,200"
 */
void sendSpeed(int16_t s1, int16_t s2, int16_t s3);

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

/**
 * Xử lý hàng đợi UDP - gửi các message đang chờ
 * Gọi function này trong loop() để xử lý queue
 */
void processUDPQueue();

/**
 * Xử lý quá trình RECALIB không blocking
 * Gọi function này trong loop() để xử lý recalibration
 */
void processRecalibration();

#endif // UDP_CONFIG_H