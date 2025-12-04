#include "mpu6050.h"

MPU6050::MPU6050() {
    accelX = accelY = accelZ = 0;
    gyroX = gyroY = gyroZ = 0;
}

bool MPU6050::begin() {
    // Initialize I2C (SDA=GPIO27, SCL=GPIO12 - mặc định cho ESP32)
    Wire.begin(16, 17); // SDA=GPIO16, SCL=GPIO17
    
    // Wake up MPU6050 (it starts in sleep mode)
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(MPU6050_PWR_MGMT_1);
    Wire.write(0x00); // Set to 0 to wake up
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
        delay(100); // Wait for sensor to stabilize
        return true;
    }
    return false;
}

int16_t MPU6050::readRegister16(uint8_t reg) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 2, true);
    
    int16_t value = Wire.read() << 8 | Wire.read();
    return value;
}

void MPU6050::readAccel(int16_t* x, int16_t* y, int16_t* z) {
    *x = readRegister16(MPU6050_ACCEL_XOUT_H);
    *y = readRegister16(MPU6050_ACCEL_YOUT_H);
    *z = readRegister16(MPU6050_ACCEL_ZOUT_H);
    
    accelX = *x;
    accelY = *y;
    accelZ = *z;
}

void MPU6050::readGyro(int16_t* x, int16_t* y, int16_t* z) {
    *x = readRegister16(MPU6050_GYRO_XOUT_H);
    *y = readRegister16(MPU6050_GYRO_YOUT_H);
    *z = readRegister16(MPU6050_GYRO_ZOUT_H);
    
    gyroX = *x;
    gyroY = *y;
    gyroZ = *z;
}

void MPU6050::readAccelG(float* x, float* y, float* z) {
    int16_t rawX, rawY, rawZ;
    readAccel(&rawX, &rawY, &rawZ);
    
    // Convert to g units (assuming default ±2g range)
    // Raw value range: -32768 to 32767
    // Sensitivity: 16384 LSB/g
    *x = rawX / 16384.0;
    *y = rawY / 16384.0;
    *z = rawZ / 16384.0;
}

void MPU6050::readAll() {
    readAccel(&accelX, &accelY, &accelZ);
    readGyro(&gyroX, &gyroY, &gyroZ);
}
