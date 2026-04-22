// I2C Scanner để kiểm tra các thiết bị I2C
// Upload code này để scan địa chỉ I2C của QMC5883L

#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 35
#define SCL_PIN 22

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n===== I2C SCANNER =====");
    Serial.printf("SDA Pin: %d\n", SDA_PIN);
    Serial.printf("SCL Pin: %d\n", SCL_PIN);
    
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000); // 100kHz
    
    Serial.println("Scanning I2C bus...\n");
}

void loop() {
    byte error, address;
    int nDevices = 0;

    Serial.println("Scanning...");

    for(address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            Serial.println(" !");

            // Kiểm tra các địa chỉ phổ biến
            if (address == 0x0D) Serial.println("  -> Likely QMC5883L!");
            if (address == 0x1E) Serial.println("  -> Likely HMC5883L!");
            if (address == 0x68) Serial.println("  -> Likely MPU6050!");
            
            nDevices++;
        }
        else if (error == 4) {
            Serial.print("Unknown error at address 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
        }
    }
    
    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("Scan complete\n");

    delay(5000); // Scan lại sau 5 giây
}
