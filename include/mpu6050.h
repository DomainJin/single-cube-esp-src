#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>

// MPU6050 I2C address
#define MPU6050_ADDR 0x68

// MPU6050 Register addresses
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_ACCEL_YOUT_H 0x3D
#define MPU6050_ACCEL_ZOUT_H 0x3F
#define MPU6050_GYRO_XOUT_H  0x43
#define MPU6050_GYRO_YOUT_H  0x45
#define MPU6050_GYRO_ZOUT_H  0x47

class MPU6050 {
private:
    int16_t accelX, accelY, accelZ;
    int16_t gyroX, gyroY, gyroZ;
    
    // Helper function to read 16-bit data from register
    int16_t readRegister16(uint8_t reg);
    
public:
    // Constructor
    MPU6050();
    
    // Initialize MPU6050
    bool begin();
    
    // Read accelerometer data (raw values)
    void readAccel(int16_t* x, int16_t* y, int16_t* z);
    
    // Read gyroscope data (raw values)
    void readGyro(int16_t* x, int16_t* y, int16_t* z);
    
    // Read accelerometer data in g units (1g = 9.81 m/s²)
    void readAccelG(float* x, float* y, float* z);
    
    // Read all sensor data
    void readAll();
    
    // Get individual axis values
    int16_t getAccelX() { return accelX; }
    int16_t getAccelY() { return accelY; }
    int16_t getAccelZ() { return accelZ; }
    int16_t getGyroX() { return gyroX; }
    int16_t getGyroY() { return gyroY; }
    int16_t getGyroZ() { return gyroZ; }
};

#endif // MPU6050_H
