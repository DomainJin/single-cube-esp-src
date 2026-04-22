#include "qmc5883l.h"
#include "udpconfig.h"
#include "3_motor.h"
#include <math.h>

QMC5883L::QMC5883L() {
    magX = magY = magZ = 0;
    accelX = accelY = accelZ = 0;
    gyroX = gyroY = gyroZ = 0;
    magXfilt = magYfilt = magZfilt = 0.0f;
    magScaleX = magScaleY = magScaleZ = 1.0f;
    
    // Initialize quaternion (identity)
    q0 = 1.0f; q1 = 0.0f; q2 = 0.0f; q3 = 0.0f;
    
    linearVelX = linearVelY = 0.0f;
    angularVelZ = 0.0f;
    
    declinationAngle = 0.0;
    lastHeading = 0.0f;
    headingOffset = 0.0f;
    sdaPin = 3;
    sclPin = 22;
    lastReadTime = 0;
    lastFusionTime = 0;
    updateInterval = 100;  // 100ms = 10Hz update rate
    isEnabled = false;
}

bool QMC5883L::begin(uint8_t sda, uint8_t scl) {
    sdaPin = sda;
    sclPin = scl;
    
    Wire.setClock(400000);  // 400kHz cho MPU9250
    delay(100);
    
    Serial.println("\n===== MPU9250 + AK8963 Initialization =====");
    Serial.printf("I2C pins: SDA=%d, SCL=%d\n", sdaPin, sclPin);
    
    // Kiểm tra MPU9250
    uint8_t whoami = readRegister(MPU9250_ADDR, MPU9250_WHO_AM_I);
    Serial.printf("MPU9250 WHO_AM_I: 0x%02X (should be 0x71 or 0x73)\n", whoami);
    
    if (whoami != 0x71 && whoami != 0x73) {
        Serial.println("ERROR: MPU9250 not found!");
        return false;
    }
    
    // Reset MPU9250
    writeRegister(MPU9250_ADDR, MPU9250_PWR_MGMT_1, 0x80);
    delay(100);
    
    // Wake up MPU9250
    writeRegister(MPU9250_ADDR, MPU9250_PWR_MGMT_1, 0x00);
    delay(10);
    
    // Configure accel: ±2g
    writeRegister(MPU9250_ADDR, MPU9250_ACCEL_CONFIG, 0x00);
    delay(10);
    
    // Configure gyro: ±250 dps
    writeRegister(MPU9250_ADDR, MPU9250_GYRO_CONFIG, 0x00);
    delay(10);
    
    // Bật bypass mode để truy cập AK8963
    writeRegister(MPU9250_ADDR, MPU9250_INT_PIN_CFG, 0x02);
    delay(10);
    
    // Kiểm tra AK8963
    uint8_t ak_whoami = readRegister(AK8963_ADDR, AK8963_WHO_AM_I);
    Serial.printf("AK8963 WHO_AM_I: 0x%02X (should be 0x48)\n", ak_whoami);
    
    if (ak_whoami != 0x48) {
        Serial.println("ERROR: AK8963 magnetometer not found!");
        return false;
    }
    
    // Đọc calibration data từ AK8963
    readAK8963Calibration();
    
    // Đặt chế độ continuous 2 (100Hz)
    writeRegister(AK8963_ADDR, AK8963_CNTL, 0x00);  // Power down
    delay(10);
    writeRegister(AK8963_ADDR, AK8963_CNTL, 0x16);  // 16-bit, 100Hz continuous
    delay(10);
    
    Serial.println("MPU9250 + AK8963 initialized successfully!");
    Serial.printf("Mag calibration: X=%.3f, Y=%.3f, Z=%.3f\n", magScaleX, magScaleY, magScaleZ);
    Serial.println("==========================================\n");
    
    return true;
}

void QMC5883L::readAK8963Calibration() {
    // Chuyển sang Fuse ROM mode
    writeRegister(AK8963_ADDR, AK8963_CNTL, 0x00);
    delay(10);
    writeRegister(AK8963_ADDR, AK8963_CNTL, AK8963_MODE_FUSE_ROM);
    delay(10);
    
    uint8_t rawX = readRegister(AK8963_ADDR, AK8963_ASAX);
    uint8_t rawY = readRegister(AK8963_ADDR, AK8963_ASAY);
    uint8_t rawZ = readRegister(AK8963_ADDR, AK8963_ASAZ);
    
    magScaleX = (float)(rawX - 128) / 256.0f + 1.0f;
    magScaleY = (float)(rawY - 128) / 256.0f + 1.0f;
    magScaleZ = (float)(rawZ - 128) / 256.0f + 1.0f;
    
    // Quay về power down
    writeRegister(AK8963_ADDR, AK8963_CNTL, 0x00);
    delay(10);
}

void QMC5883L::reset() {
    writeRegister(MPU9250_ADDR, MPU9250_PWR_MGMT_1, 0x80);
    delay(100);
}

void QMC5883L::writeRegister(uint8_t addr, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t QMC5883L::readRegister(uint8_t addr, uint8_t reg) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    
    Wire.requestFrom(addr, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

int16_t QMC5883L::readRegister16(uint8_t addr, uint8_t reg) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    
    Wire.requestFrom(addr, (uint8_t)2);
    
    // AK8963 gửi LSB trước (Little Endian)
    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    
    return (int16_t)((msb << 8) | lsb);
}

bool QMC5883L::readMag(int16_t* x, int16_t* y, int16_t* z) {
    // Kiểm tra ST1 (data ready)
    uint8_t st1 = readRegister(AK8963_ADDR, AK8963_ST1);
    if (!(st1 & 0x01)) {
        return false;  // Data not ready
    }
    
    // Đọc dữ liệu magnetometer
    int16_t rawX = readRegister16(AK8963_ADDR, AK8963_XOUT_L);
    int16_t rawY = readRegister16(AK8963_ADDR, AK8963_YOUT_L);
    int16_t rawZ = readRegister16(AK8963_ADDR, AK8963_ZOUT_L);
    
    // Đọc ST2 để kết thúc transaction
    uint8_t st2 = readRegister(AK8963_ADDR, AK8963_ST2);
    
    // Kiểm tra overflow
    if (st2 & 0x08) {
        return false;  // Magnetic sensor overflow
    }
    
    // Áp dụng calibration
    rawX = (int16_t)(rawX * magScaleX);
    rawY = (int16_t)(rawY * magScaleY);
    rawZ = (int16_t)(rawZ * magScaleZ);
    
    // Low-pass filter (EMA - Exponential Moving Average)
    float alpha = 0.2f;  // 0.2 = smooth, 0.8 = responsive
    magXfilt = alpha * rawX + (1 - alpha) * magXfilt;
    magYfilt = alpha * rawY + (1 - alpha) * magYfilt;
    magZfilt = alpha * rawZ + (1 - alpha) * magZfilt;
    
    // Output filtered values
    magX = (int16_t)magXfilt;
    magY = (int16_t)magYfilt;
    magZ = (int16_t)magZfilt;
    
    *x = magX;
    *y = magY;
    *z = magZ;
    
    return true;
}

bool QMC5883L::isDataReady() {
    uint8_t st1 = readRegister(AK8963_ADDR, AK8963_ST1);
    return (st1 & 0x01) != 0;
}

void QMC5883L::setDeclination(float declinationDegrees) {
    declinationAngle = declinationDegrees * PI / 180.0;
    Serial.printf("[MPU9250] Declination set to: %.2f degrees\n", declinationDegrees);
}

float QMC5883L::getHeading() {
    // Tính heading từ giá trị đã lưu (không đọc lại)
    if (magX == 0 && magY == 0) {
        return -1.0;  // Chưa có dữ liệu
    }
    
    // Tính heading từ X và Y
    float heading = atan2((float)magY, (float)magX);
    
    // Thêm declination
    heading += declinationAngle;
    
    // Chuẩn hóa về 0-360
    heading = normalizeAngle(heading * 180.0 / PI);
    
    // Lọc heading để giảm nhiễu
    heading = filterHeading(heading, 0.3);  // alpha = 0.3 (mượt)
    
    return heading;
}

float QMC5883L::normalizeAngle(float angle) {
    while (angle < 0) angle += 360.0;
    while (angle >= 360.0) angle -= 360.0;
    return angle;
}

float QMC5883L::filterHeading(float newHeading, float alpha) {
    // Xử lý wrap-around (359° -> 1° không nhảy qua 180°)
    float diff = newHeading - lastHeading;
    
    if (diff > 180.0) {
        diff -= 360.0;
    } else if (diff < -180.0) {
        diff += 360.0;
    }
    
    // EMA filter
    lastHeading = lastHeading + alpha * diff;
    lastHeading = normalizeAngle(lastHeading);
    
    return lastHeading;
}

String QMC5883L::getCardinalDirection() {
    float heading = getHeading();
    
    if (heading < 0) return "?";
    
    if (heading >= 337.5 || heading < 22.5) return "N";
    else if (heading >= 22.5 && heading < 67.5) return "NE";
    else if (heading >= 67.5 && heading < 112.5) return "E";
    else if (heading >= 112.5 && heading < 157.5) return "SE";
    else if (heading >= 157.5 && heading < 202.5) return "S";
    else if (heading >= 202.5 && heading < 247.5) return "SW";
    else if (heading >= 247.5 && heading < 292.5) return "W";
    else if (heading >= 292.5 && heading < 337.5) return "NW";
    
    return "?";
}

void QMC5883L::update() {
    if (!isEnabled) {
        return;
    }
    
    if (millis() - lastReadTime < updateInterval) {
        return;
    }
    
    lastReadTime = millis();
    
    // ✅ Check data ready trước - Chỉ đọc khi AK8963 có data mới
    if (!isDataReady()) {
        return;  // Chưa có data mới, skip
    }
    
    // Đọc dữ liệu magnetometer
    int16_t mx, my, mz;
    if (readMag(&mx, &my, &mz)) {
        // Dùng getCalibratedHeading() để có giá trị đã hiệu chỉnh
        float heading = getCalibratedHeading();
        String direction = getCardinalDirection();
        
        // ✅ Tắt Serial print để tăng tốc (comment out khi không debug)
        // Serial.printf("[MPU9250] Mag: X=%d, Y=%d, Z=%d | Heading=%.1f° (%s) [Offset=%.1f°]\n", 
        //               mx, my, mz, heading, direction.c_str(), headingOffset);
        
        sendCompassHeading(heading, direction.c_str());
        sendCompassRaw(mx, my, mz);
        
        // Serial.println("[MPU9250] Data sent via UDP");  // Comment để giảm spam
    }
    // else {
    //     Serial.println("[MPU9250 ERROR] Failed to read magnetometer data!");
    // }
}

void QMC5883L::enable() {
    isEnabled = true;
    Serial.println("[MPU9250] Module ENABLED - Sending data");
}

void QMC5883L::disable() {
    isEnabled = false;
    Serial.println("[MPU9250] Module DISABLED");
}

void QMC5883L::setUpdateInterval(uint16_t intervalMs) {
    updateInterval = intervalMs;
    Serial.printf("[MPU9250] Update interval set to %d ms\n", intervalMs);
}

//================= 9-DOF Sensor Fusion =================

bool QMC5883L::readAccel() {
    // Read 6 bytes from accel
    Wire.beginTransmission(MPU9250_ADDR);
    Wire.write(MPU9250_ACCEL_XOUT_H);
    if (Wire.endTransmission(false) != 0) return false;
    
    Wire.requestFrom(MPU9250_ADDR, (uint8_t)6);
    if (Wire.available() < 6) return false;
    
    accelX = (Wire.read() << 8) | Wire.read();
    accelY = (Wire.read() << 8) | Wire.read();
    accelZ = (Wire.read() << 8) | Wire.read();
    
    return true;
}

bool QMC5883L::readGyro() {
    // Read 6 bytes from gyro
    Wire.beginTransmission(MPU9250_ADDR);
    Wire.write(MPU9250_GYRO_XOUT_H);
    if (Wire.endTransmission(false) != 0) return false;
    
    Wire.requestFrom(MPU9250_ADDR, (uint8_t)6);
    if (Wire.available() < 6) return false;
    
    gyroX = (Wire.read() << 8) | Wire.read();
    gyroY = (Wire.read() << 8) | Wire.read();
    gyroZ = (Wire.read() << 8) | Wire.read();
    
    return true;
}

// Fast inverse square root
float QMC5883L::invSqrt(float x) {
    float halfx = 0.5f * x;
    float y = x;
    long i = *(long*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;
    y = y * (1.5f - (halfx * y * y));
    return y;
}

// Madgwick AHRS algorithm - 9-axis fusion
void QMC5883L::madgwickUpdate(float gx, float gy, float gz, 
                               float ax, float ay, float az,
                               float mx, float my, float mz, float dt) {
    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float hx, hy;
    float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1, _2q2, _2q3;
    float _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;
    
    float beta = 0.1f;
    
    qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    qDot2 = 0.5f * (q0 * gx + q2 * gz - q3 * gy);
    qDot3 = 0.5f * (q0 * gy - q1 * gz + q3 * gx);
    qDot4 = 0.5f * (q0 * gz + q1 * gy - q2 * gx);
    
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
        recipNorm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;
        
        recipNorm = invSqrt(mx * mx + my * my + mz * mz);
        mx *= recipNorm;
        my *= recipNorm;
        mz *= recipNorm;
        
        _2q0mx = 2.0f * q0 * mx;
        _2q0my = 2.0f * q0 * my;
        _2q0mz = 2.0f * q0 * mz;
        _2q1mx = 2.0f * q1 * mx;
        _2q0 = 2.0f * q0;
        _2q1 = 2.0f * q1;
        _2q2 = 2.0f * q2;
        _2q3 = 2.0f * q3;
        _2q0q2 = 2.0f * q0 * q2;
        _2q2q3 = 2.0f * q2 * q3;
        q0q0 = q0 * q0;
        q0q1 = q0 * q1;
        q0q2 = q0 * q2;
        q0q3 = q0 * q3;
        q1q1 = q1 * q1;
        q1q2 = q1 * q2;
        q1q3 = q1 * q3;
        q2q2 = q2 * q2;
        q2q3 = q2 * q3;
        q3q3 = q3 * q3;
        
        hx = mx * q0q0 - _2q0my * q3 + _2q0mz * q2 + mx * q1q1 + _2q1 * my * q2 + _2q1 * mz * q3 - mx * q2q2 - mx * q3q3;
        hy = _2q0mx * q3 + my * q0q0 - _2q0mz * q1 + _2q1mx * q2 - my * q1q1 + my * q2q2 + _2q2 * mz * q3 - my * q3q3;
        _2bx = sqrt(hx * hx + hy * hy);
        _2bz = -_2q0mx * q2 + _2q0my * q1 + mz * q0q0 + _2q1mx * q3 - mz * q1q1 + _2q2 * my * q3 - mz * q2q2 + mz * q3q3;
        _4bx = 2.0f * _2bx;
        _4bz = 2.0f * _2bz;
        
        s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) + _2q1 * (2.0f * q0q1 + _2q2q3 - ay) - _2bz * q2 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q3 + _2bz * q1) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q2 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) + _2q0 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * q1 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2 - az) + _2bz * q3 * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q2 + _2bz * q0) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q3 - _4bz * q1) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) + _2q3 * (2.0f * q0q1 + _2q2q3 - ay) - 4.0f * q2 * (1.0f - 2.0f * q1q1 - 2.0f * q2q2 - az) + (-_4bx * q2 - _2bz * q0) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (_2bx * q1 + _2bz * q3) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + (_2bx * q0 - _4bz * q2) * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) + _2q2 * (2.0f * q0q1 + _2q2q3 - ay) + (-_4bx * q3 + _2bz * q1) * (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) + (-_2bx * q0 + _2bz * q2) * (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) + _2bx * q1 * (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
        
        recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
        s0 *= recipNorm;
        s1 *= recipNorm;
        s2 *= recipNorm;
        s3 *= recipNorm;
        
        qDot1 -= beta * s0;
        qDot2 -= beta * s1;
        qDot3 -= beta * s2;
        qDot4 -= beta * s3;
    }
    
    q0 += qDot1 * dt;
    q1 += qDot2 * dt;
    q2 += qDot3 * dt;
    q3 += qDot4 * dt;
    
    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
}

void QMC5883L::updateFusion() {
    unsigned long now = millis();
    if (lastFusionTime == 0) {
        lastFusionTime = now;
        return;
    }
    
    float dt = (now - lastFusionTime) / 1000.0f;
    if (dt <= 0 || dt > 0.1f) {
        lastFusionTime = now;
        return;
    }
    
    if (!readAccel() || !readGyro()) return;
    
    int16_t mx, my, mz;
    if (!readMag(&mx, &my, &mz)) return;
    
    // Convert to physical units
    float ax = accelX / 16384.0f;
    float ay = accelY / 16384.0f;
    float az = accelZ / 16384.0f;
    
    float gx = (gyroX / 131.0f) * DEG_TO_RAD;
    float gy = (gyroY / 131.0f) * DEG_TO_RAD;
    float gz = (gyroZ / 131.0f) * DEG_TO_RAD;
    
    float mx_norm = mx / 1000.0f;
    float my_norm = my / 1000.0f;
    float mz_norm = mz / 1000.0f;
    
    madgwickUpdate(gx, gy, gz, ax, ay, az, mx_norm, my_norm, mz_norm, dt);
    
    angularVelZ = gz;
    
    // Simple velocity integration with decay
    static float velX = 0.0f, velY = 0.0f;
    const float DECAY = 0.95f;
    
    velX = velX * DECAY + ax * 980.0f * dt;
    velY = velY * DECAY + ay * 980.0f * dt;
    
    linearVelX = velX;
    linearVelY = velY;
    
    lastFusionTime = now;
}

void QMC5883L::getVelocities(float &vx, float &vy, float &omega) {
    vx = linearVelX;
    vy = linearVelY;
    omega = angularVelZ;
}

float QMC5883L::getYaw() {
    float yaw = atan2(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3));
    return yaw * RAD_TO_DEG;
}

// ===== COMPASS CALIBRATION FUNCTIONS =====

void QMC5883L::calibrateNorth() {
    float currentHeading = getHeading();
    headingOffset = 0.0f - currentHeading;
    Serial.printf("[COMPASS_CALIB] Calibrate North: Current=%.1f°, Offset=%.1f°\n", currentHeading, headingOffset);
}

void QMC5883L::calibrateEast() {
    float currentHeading = getHeading();
    headingOffset = 90.0f - currentHeading;
    Serial.printf("[COMPASS_CALIB] Calibrate East: Current=%.1f°, Offset=%.1f°\n", currentHeading, headingOffset);
}

void QMC5883L::calibrateSouth() {
    float currentHeading = getHeading();
    headingOffset = 180.0f - currentHeading;
    Serial.printf("[COMPASS_CALIB] Calibrate South: Current=%.1f°, Offset=%.1f°\n", currentHeading, headingOffset);
}

void QMC5883L::calibrateWest() {
    float currentHeading = getHeading();
    headingOffset = 270.0f - currentHeading;
    Serial.printf("[COMPASS_CALIB] Calibrate West: Current=%.1f°, Offset=%.1f°\n", currentHeading, headingOffset);
}

float QMC5883L::getCalibratedHeading() {
    float heading = getHeading() + headingOffset;
    return normalizeAngle(heading);
}

// ===== ROTATE TO DIRECTION FUNCTIONS =====

bool QMC5883L::rotateToNorth(float tolerance) {
    float currentHeading = getCalibratedHeading();
    float targetHeading = 0.0f;
    
    // Tính góc cần xoay (chọn đường ngắn nhất)
    float error = targetHeading - currentHeading;
    if (error > 180.0f) error -= 360.0f;
    if (error < -180.0f) error += 360.0f;
    
    // Kiểm tra đã đến vị trí chưa
    if (fabs(error) <= tolerance) {
        omniStop();
        return true;  // Đã đến
    }
    
    // Xoay về hướng target
    int direction = (error > 0) ? 1 : 0;  // 1 = thuận chiều, 0 = ngược chiều
    float speed = constrain(fabs(error) * 2.0f, 30.0f, 100.0f);  // Tỷ lệ với góc lệch
    if(direction == 0) speed = -speed; // Ngược chiều cần tốc độ cao hơn để vượt qua quán tính
    omniRotate(direction, speed);
    
    return false;  // Chưa đến
}

bool QMC5883L::rotateToEast(float tolerance) {
    float currentHeading = getCalibratedHeading();
    float targetHeading = 90.0f;
    
    float error = targetHeading - currentHeading;
    if (error > 180.0f) error -= 360.0f;
    if (error < -180.0f) error += 360.0f;
    
    if (fabs(error) <= tolerance) {
        omniStop();
        return true;
    }
    
    int direction = (error > 0) ? 1 : 0;
    float speed = constrain(fabs(error) * 2.0f, 30.0f, 100.0f);
    if(direction == 0) speed = -speed;
    omniRotate(direction, speed);
    
    return false;
}

bool QMC5883L::rotateToSouth(float tolerance) {
    float currentHeading = getCalibratedHeading();
    float targetHeading = 180.0f;
    
    float error = targetHeading - currentHeading;
    if (error > 180.0f) error -= 360.0f;
    if (error < -180.0f) error += 360.0f;
    
    if (fabs(error) <= tolerance) {
        omniStop();
        return true;
    }
    
    int direction = (error > 0) ? 1 : 0;
    float speed = constrain(fabs(error) * 2.0f, 30.0f, 100.0f);
    if(direction == 0) speed = -speed;
    omniRotate(direction, speed);
    
    return false;
}

bool QMC5883L::rotateToWest(float tolerance) {
    float currentHeading = getCalibratedHeading();
    float targetHeading = 270.0f;
    
    float error = targetHeading - currentHeading;
    if (error > 180.0f) error -= 360.0f;
    if (error < -180.0f) error += 360.0f;
    
    if (fabs(error) <= tolerance) {
        omniStop();
        return true;
    }
    
    int direction = (error > 0) ? 1 : 0;
    float speed = constrain(fabs(error) * 2.0f, 30.0f, 100.0f);
    if(direction == 0) speed = -speed;
    omniRotate(direction, speed);
    
    return false;
}
