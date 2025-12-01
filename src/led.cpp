#include "main.h"

// ===== LED STRIP OBJECT =====
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ===== LED CONTROL VARIABLES =====
bool effectEnable = true;
int brightness = 255;
int currentLedCount = 0;
int ledDirection = 1;  // 1: từ đầu đến cuối, -1: từ cuối về đầu
unsigned long lastUpdateTime = 0;

// ===== COLOR VARIABLES =====
int current_r = 255, current_g = 0, current_b = 0;  // Màu mặc định đỏ
bool colorUpdated = false;



// ===== LED INITIALIZATION =====
void initLED() {
    Serial.println("[LED] Khởi tạo LED Strip...");
    
    // Khởi tạo LED strip
    strip.begin();
    
    // ✅ Test rõ ràng: Tắt hết trước
    Serial.println("[LED] Tắt hết LED...");
    strip.clear();
    strip.show();
    delay(1000);
    
    // ✅ Test màu đơn giản - chỉ 5 LED đầu
    Serial.println("[LED] Test 5 LED đầu - màu đỏ...");
    for(int i = 0; i < 5; i++) {
        strip.setPixelColor(i, strip.Color(255, 0, 0));
    }
    strip.show();
    delay(2000);
    
    Serial.println("[LED] Test 5 LED đầu - màu xanh...");
    for(int i = 0; i < 5; i++) {
        strip.setPixelColor(i, strip.Color(0, 255, 0));
    }
    strip.show();
    delay(2000);
    
    // ✅ Tắt hết và confirm
    Serial.println("[LED] Tắt hết LED - hoàn thành init");
    strip.clear();
    strip.show();
    
    // Thiết lập giá trị mặc định
    effectEnable = true;
    brightness = 255;
    currentLedCount = 0;
    ledDirection = 1;
    lastUpdateTime = 0;
    
    Serial.printf("[LED] LED Strip sẵn sàng - Pin: %d, LEDs: %d\n", LED_PIN, NUM_LEDS);
    Serial.printf("[LED] Brightness: %d, Direction: %d, Effect: %s\n", 
                  brightness, ledDirection, effectEnable ? "ON" : "OFF");
}

// ===== COLOR UPDATE FUNCTIONS =====
void updateLEDColor(int r, int g, int b) {
    current_r = r;
    current_g = g;
    current_b = b;
    colorUpdated = true;
    Serial.printf("[LED] Màu cập nhật từ UDP: RGB(%d,%d,%d)\n", r, g, b);
}

void resetLEDColor() {
    current_r = 255;
    current_g = 0;
    current_b = 0;
    colorUpdated = true;
    Serial.println("[LED] Reset màu về đỏ mặc định");
}

// ===== LED CONTROL FUNCTIONS =====
void applyColorWithBrightness(bool turnOn, int r, int g, int b) {
    if (!effectEnable) return;
    
    unsigned long now = millis();
    if (now - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = now;
        
        // Update LED count
        if (turnOn && currentLedCount < NUM_LEDS) {
            currentLedCount++;
        } else if (!turnOn && currentLedCount > 0) {
            currentLedCount--;
        }
        
        // Calculate adjusted colors with brightness
        int adj_r = r * brightness / 255;
        int adj_g = g * brightness / 255;
        int adj_b = b * brightness / 255;
        
        // Apply colors based on direction
        for (int i = 0; i < NUM_LEDS; i++) {
            bool shouldLight = false;
            
            if (ledDirection == 1) {
                shouldLight = (i < currentLedCount);
            } else {
                shouldLight = (i >= NUM_LEDS - currentLedCount);
            }
            
            if (shouldLight) {
                strip.setPixelColor(i, strip.Color(adj_r, adj_g, adj_b));
            } else {
                strip.setPixelColor(i, strip.Color(0, 0, 0));
            }
        }
        
        strip.show();
        
        // Debug màu hiện tại
        // Serial.printf("[LED] Touch: %s, Count: %d, RGB: (%d,%d,%d)\n", 
        //               turnOn ? "YES" : "NO", currentLedCount, adj_r, adj_g, adj_b);
    }
}

// ===== LED CONTROL FUNCTIONS =====
void setLEDBrightness(int newBrightness) {
    if (newBrightness < 0) newBrightness = 0;
    if (newBrightness > 255) newBrightness = 255;
    
    brightness = newBrightness;
    Serial.printf("[LED] Brightness đã đặt: %d\n", brightness);
}

void setLEDDirection(int direction) {
    if (direction == 1 || direction == -1) {
        ledDirection = direction;
        Serial.printf("[LED] Direction đã đặt: %d\n", ledDirection);
    } else {
        Serial.printf("[LED] Direction không hợp lệ: %d (chỉ chấp nhận 1 hoặc -1)\n", direction);
    }
}

void enableLEDEffect(bool enable) {
    effectEnable = enable;
    Serial.printf("[LED] Effect %s\n", enable ? "BẬT" : "TẮT");
    
    if (!enable) {
        clearAllLEDs();
    }
}

void clearAllLEDs() {
    strip.clear();
    strip.show();
    currentLedCount = 0;
    Serial.println("[LED] Tất cả LED đã tắt");
}

void setAllLEDs(int r, int g, int b) {
    int adj_r = (r * brightness) / 255;
    int adj_g = (g * brightness) / 255;
    int adj_b = (b * brightness) / 255;
    
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(adj_r, adj_g, adj_b));
    }
    strip.show();
    currentLedCount = NUM_LEDS;
    
    Serial.printf("[LED] Tất cả LED đặt màu RGB: (%d,%d,%d)\n", adj_r, adj_g, adj_b);
}

void setLEDRange(int start, int end, int r, int g, int b) {
    if (start < 0) start = 0;
    if (end >= NUM_LEDS) end = NUM_LEDS - 1;
    if (start > end) return;
    
    int adj_r = (r * brightness) / 255;
    int adj_g = (g * brightness) / 255;
    int adj_b = (b * brightness) / 255;
    
    for (int i = start; i <= end; i++) {
        strip.setPixelColor(i, strip.Color(adj_r, adj_g, adj_b));
    }
    strip.show();
    
    Serial.printf("[LED] Range [%d-%d] đặt màu RGB: (%d,%d,%d)\n", start, end, adj_r, adj_g, adj_b);
}

// ===== LED UTILITY FUNCTIONS =====
bool isLEDReady() {
    return true; // LED strip luôn sẵn sàng sau khi khởi tạo
}

int getCurrentLEDCount() {
    return currentLedCount;
}

int getLEDBrightness() {
    return brightness;
}

int getLEDDirection() {
    return ledDirection;
}

bool isEffectEnabled() {
    return effectEnable;
}

// ===== COLOR GETTER FUNCTIONS =====
int getCurrentR() { return current_r; }
int getCurrentG() { return current_g; }
int getCurrentB() { return current_b; }