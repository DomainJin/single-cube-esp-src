#ifndef QMC5883L_H
#define QMC5883L_H

#include <Arduino.h>
#include <Wire.h>

// MPU9250 I2C addresses
#define MPU9250_ADDR     0x68  // MPU9250 chính (gyro + accel)
#define AK8963_ADDR      0x0C  // Magnetometer AK8963 bên trong MPU9250

// MPU9250 Registers
#define MPU9250_WHO_AM_I     0x75
#define MPU9250_PWR_MGMT_1   0x6B
#define MPU9250_INT_PIN_CFG  0x37
#define MPU9250_INT_ENABLE   0x38
#define MPU9250_USER_CTRL    0x6A
#define MPU9250_ACCEL_XOUT_H 0x3B
#define MPU9250_GYRO_XOUT_H  0x43
#define MPU9250_ACCEL_CONFIG 0x1C
#define MPU9250_GYRO_CONFIG  0x1B

// AK8963 Magnetometer Registers
#define AK8963_WHO_AM_I      0x00  // Should return 0x48
#define AK8963_INFO          0x01
#define AK8963_ST1           0x02  // Status 1
#define AK8963_XOUT_L        0x03  // X-axis data
#define AK8963_XOUT_H        0x04
#define AK8963_YOUT_L        0x05  // Y-axis data
#define AK8963_YOUT_H        0x06
#define AK8963_ZOUT_L        0x07  // Z-axis data
#define AK8963_ZOUT_H        0x08
#define AK8963_ST2           0x09  // Status 2
#define AK8963_CNTL          0x0A  // Control
#define AK8963_ASTC          0x0C  // Self test
#define AK8963_ASAX          0x10  // Sensitivity adjustment X
#define AK8963_ASAY          0x11  // Sensitivity adjustment Y
#define AK8963_ASAZ          0x12  // Sensitivity adjustment Z

// AK8963 Modes
#define AK8963_MODE_POWERDOWN    0x00
#define AK8963_MODE_SINGLE       0x01
#define AK8963_MODE_CONTINUOUS_1 0x02  // 8Hz
#define AK8963_MODE_CONTINUOUS_2 0x06  // 100Hz
#define AK8963_MODE_FUSE_ROM     0x0F

class QMC5883L {
private:
    // Raw sensor data
    int16_t magX, magY, magZ;
    int16_t accelX, accelY, accelZ;
    int16_t gyroX, gyroY, gyroZ;
    
    // Filtered magnetometer
    float magXfilt, magYfilt, magZfilt;
    float magScaleX, magScaleY, magScaleZ;  // AK8963 calibration
    
    // Quaternion for sensor fusion (Madgwick filter)
    float q0, q1, q2, q3;  // w, x, y, z
    
    // Velocities from sensor fusion
    float linearVelX, linearVelY;  // cm/s
    float angularVelZ;  // rad/s
    
    float declinationAngle;
    float lastHeading;  // For heading filter
    float headingOffset;  // Offset for compass calibration
    uint8_t sdaPin, sclPin;
    unsigned long lastReadTime, lastFusionTime;
    uint16_t updateInterval;
    bool isEnabled;
    
    void writeRegister(uint8_t addr, uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t addr, uint8_t reg);
    int16_t readRegister16(uint8_t addr, uint8_t reg);
    void readAK8963Calibration();
    float normalizeAngle(float angle);
    float filterHeading(float newHeading, float alpha = 0.3);
    
    // 9-DOF sensor reading
    bool readAccel();
    bool readGyro();
    
    // Madgwick sensor fusion
    void madgwickUpdate(float gx, float gy, float gz, 
                        float ax, float ay, float az,
                        float mx, float my, float mz, float dt);
    float invSqrt(float x);
    
public:
    QMC5883L();
    
    // Initialize MPU9250 + AK8963
    bool begin(uint8_t sda = 3, uint8_t scl = 22);
    
    // Soft reset chip
    void reset();
    
    // Đặt góc lệch từ (declination angle)
    void setDeclination(float declinationDegrees);
    
    // Đọc magnetometer raw values từ AK8963
    bool readMag(int16_t* x, int16_t* y, int16_t* z);
    
    // Tính heading (0-360 độ)
    float getHeading();
    
    // Lấy hướng cardinal (N, NE, E, SE, S, SW, W, NW)
    String getCardinalDirection();
    
    // Kiểm tra data ready
    bool isDataReady();
    
    // Cập nhật và gửi dữ liệu tự động (gọi trong loop)
    void update();
    
    // Update 9-DOF sensor fusion
    void updateFusion();
    
    // Get velocities from sensor fusion
    void getVelocities(float &vx, float &vy, float &omega);
    
    // Get yaw angle from quaternion
    float getYaw();
    
    // Bật/tắt module
    void enable();
    void disable();
    
    // Đặt chu kỳ cập nhật (ms)
    void setUpdateInterval(uint16_t intervalMs);
    
    // Calibrate robot theo 4 hướng chính
    void calibrateNorth();  // Đông (0°)
    void calibrateEast();   // Tây (90°)
    void calibrateSouth();  // Nam (180°)
    void calibrateWest();   // Bắc (270°)
    
    // Lấy heading đã hiệu chỉnh
    float getCalibratedHeading();
    
    // Xoay robot về hướng cụ thể (trả về true khi hoàn thành)
    bool rotateToNorth(float tolerance = 5.0f);   // Xoay về Bắc (0°)
    bool rotateToEast(float tolerance = 5.0f);    // Xoay về Đông (90°)
    bool rotateToSouth(float tolerance = 5.0f);   // Xoay về Nam (180°)
    bool rotateToWest(float tolerance = 5.0f);    // Xoay về Tây (270°)
};

#endif // QMC5883L_H
