#include "qmc5883l.h"
#include <math.h>

QMC5883L::QMC5883L() {
    magX = magY = magZ = 0;
    declinationAngle = 0.0;
    sdaPin = 21;
    sclPin = 22;
}

bool QMC5883L::begin(uint8_t sda, uint8_t scl) {
    sdaPin = sda;
    sclPin = scl;
    
    // Initialize I2C
    Wire.begin(sdaPin, sclPin);
    Wire.setClock(100000); // 100kHz
    delay(100);
    
    Serial.println("\n===== QMC5883L Initialization =====");
    Serial.printf("I2C pins: SDA=%d, SCL=%d\n", sdaPin, sclPin);
    
    // Check device presence
    Wire.beginTransmission(QMC5883L_ADDR);
    uint8_t error = Wire.endTransmission();
    
    if (error != 0) {
        Serial.printf("ERROR: QMC5883L not found at 0x%02X!\n", QMC5883L_ADDR);
        return false;
    }
    
    Serial.println("QMC5883L detected!");
    
    // Soft reset
    reset();
    delay(100);
    
    // Configure QMC5883L
    // Mode: Continuous | ODR: 200Hz | Range: 8G | OSR: 512
    uint8_t config = QMC5883L_MODE_CONTINUOUS | QMC5883L_ODR_200HZ | 
                     QMC5883L_RNG_8G | QMC5883L_OSR_512;
    writeRegister(QMC5883L_CONFIG, config);
    
    // SET/RESET Period FBR register
    writeRegister(QMC5883L_CONFIG2, 0x01);
    
    delay(100);
    
    Serial.println("QMC5883L configured successfully!");
    Serial.println("===================================\n");
    
    return true;
}

void QMC5883L::reset() {
    writeRegister(QMC5883L_RESET, 0x01);
    delay(10);
}

void QMC5883L::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

int16_t QMC5883L::readRegister16(uint8_t reg) {
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    
    Wire.requestFrom((uint8_t)QMC5883L_ADDR, (uint8_t)2);
    
    // QMC5883L sends LSB first (Little Endian)
    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    
    return (int16_t)((msb << 8) | lsb);
}

bool QMC5883L::readMag(int16_t* x, int16_t* y, int16_t* z) {
    // Read X, Y, Z directly (no need to check DRDY in continuous mode)
    *x = readRegister16(QMC5883L_X_LSB);
    *y = readRegister16(QMC5883L_Y_LSB);
    *z = readRegister16(QMC5883L_Z_LSB);
    
    magX = *x;
    magY = *y;
    magZ = *z;
    
    return true;
}

bool QMC5883L::isDataReady() {
    Wire.beginTransmission(QMC5883L_ADDR);
    Wire.write(QMC5883L_STATUS);
    Wire.endTransmission(false);
    
    Wire.requestFrom((uint8_t)QMC5883L_ADDR, (uint8_t)1);
    uint8_t status = Wire.read();
    
    // Bit 0 = DRDY (Data Ready)
    return (status & 0x01) != 0;
}

void QMC5883L::setDeclination(float declinationDegrees) {
    declinationAngle = declinationDegrees * PI / 180.0;
    Serial.printf("[QMC5883L] Declination set to: %.2f degrees\n", declinationDegrees);
}

float QMC5883L::getHeading() {
    int16_t x, y, z;
    if (!readMag(&x, &y, &z)) {
        return -1.0; // Error
    }
    
    // Calculate heading from X and Y
    float heading = atan2((float)y, (float)x);
    
    // Add declination
    heading += declinationAngle;
    
    // Normalize to 0-360
    if (heading < 0)
        heading += 2 * PI;
    
    if (heading > 2 * PI)
        heading -= 2 * PI;
    
    // Convert to degrees
    return heading * 180.0 / PI;
}

String QMC5883L::getCardinalDirection() {
    float heading = getHeading();
    
    if (heading < 0) return "?";
    
    // 8 cardinal directions
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

int16_t QMC5883L::readTemperature() {
    return readRegister16(QMC5883L_TEMP_LSB);
}
