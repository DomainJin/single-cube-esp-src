#ifndef QMC5883L_H
#define QMC5883L_H

#include <Arduino.h>
#include <Wire.h>

// QMC5883L I2C address
#define QMC5883L_ADDR 0x0D

// QMC5883L Registers
#define QMC5883L_X_LSB       0x00
#define QMC5883L_X_MSB       0x01
#define QMC5883L_Y_LSB       0x02
#define QMC5883L_Y_MSB       0x03
#define QMC5883L_Z_LSB       0x04
#define QMC5883L_Z_MSB       0x05
#define QMC5883L_STATUS      0x06
#define QMC5883L_TEMP_LSB    0x07
#define QMC5883L_TEMP_MSB    0x08
#define QMC5883L_CONFIG      0x09
#define QMC5883L_CONFIG2     0x0A
#define QMC5883L_RESET       0x0B
#define QMC5883L_RESERVED    0x0C
#define QMC5883L_CHIP_ID     0x0D

// Modes
#define QMC5883L_MODE_STANDBY    0x00
#define QMC5883L_MODE_CONTINUOUS 0x01

// Output Data Rate
#define QMC5883L_ODR_10HZ  0x00
#define QMC5883L_ODR_50HZ  0x04
#define QMC5883L_ODR_100HZ 0x08
#define QMC5883L_ODR_200HZ 0x0C

// Range
#define QMC5883L_RNG_2G 0x00
#define QMC5883L_RNG_8G 0x10

// Over Sample Rate
#define QMC5883L_OSR_512 0x00
#define QMC5883L_OSR_256 0x40
#define QMC5883L_OSR_128 0x80
#define QMC5883L_OSR_64  0xC0

class QMC5883L {
private:
    int16_t magX, magY, magZ;
    float declinationAngle;
    uint8_t sdaPin, sclPin;
    
    void writeRegister(uint8_t reg, uint8_t value);
    int16_t readRegister16(uint8_t reg);
    
public:
    QMC5883L();
    
    // Initialize với I2C pins
    bool begin(uint8_t sda = 21, uint8_t scl = 22);
    
    // Soft reset chip
    void reset();
    
    // Đặt góc lệch từ (declination angle)
    void setDeclination(float declinationDegrees);
    
    // Đọc magnetometer raw values
    bool readMag(int16_t* x, int16_t* y, int16_t* z);
    
    // Tính heading (0-360 độ)
    float getHeading();
    
    // Lấy hướng cardinal (N, NE, E, SE, S, SW, W, NW)
    String getCardinalDirection();
    
    // Đọc nhiệt độ chip
    int16_t readTemperature();
    
    // Kiểm tra data ready
    bool isDataReady();
};

#endif // QMC5883L_H
