#include "IR.h"
#include "udpconfig.h"
#include "uart.h"
#include <driver/gpio.h>

// ===== Direct GPIO Mux — 16 channel x 2 board =====

uint16_t muxData[2][MUX_CHANNELS];
uint16_t muxThreshold[2][MUX_CHANNELS];
uint8_t  muxDigital[2][MUX_CHANNELS];

void setThreshold(uint8_t board, uint8_t ch, uint16_t thr) {
    if (board < 2 && ch < MUX_CHANNELS) {
        muxThreshold[board][ch] = thr;
        // ✅ DEBUG: Log thresholds cao (> 3000)
        if (thr > 3000) {
            Serial.printf("[THRESH_SET] B%d-CH%d: %d\n", board, ch, thr);
        }
    }
}

void setThresholdAll(uint16_t thr) {
    for (uint8_t b = 0; b < 2; b++)
        for (uint8_t ch = 0; ch < MUX_CHANNELS; ch++)
            muxThreshold[b][ch] = thr;
}

static void setMuxAddr(uint8_t ch) {
    digitalWrite(MUX_A0, (ch >> 0) & 1);
    digitalWrite(MUX_A1, (ch >> 1) & 1);
    digitalWrite(MUX_A2, (ch >> 2) & 1);
    digitalWrite(MUX_A3, (ch >> 3) & 1);
}

void initMux() {
    gpio_reset_pin(GPIO_NUM_3);  // free GPIO3 khỏi UART0 RX
    pinMode(MUX_A0, OUTPUT);
    pinMode(MUX_A1, OUTPUT);
    pinMode(MUX_A2, OUTPUT);
    pinMode(MUX_A3, OUTPUT);
    setMuxAddr(0);
    setThresholdAll(MUX_DEFAULT_THRESHOLD);
    memset(muxDigital, 0, sizeof(muxDigital));
    analogSetPinAttenuation(MUX_ADC1, ADC_11db);
    analogSetPinAttenuation(MUX_ADC2, ADC_11db);
    analogReadResolution(12);
}

// Test: bật lần lượt từng bit địa chỉ, dùng VOM đo A0-A3 trên mux board
void testMux() {
    Serial.println("=== MUX ADDR TEST ===");
    const char* names[] = {"A0(GPIO3)", "A1(GPIO0)", "A2(GPIO12)", "A3(GPIO19)"};
    for (uint8_t bit = 0; bit < 4; bit++) {
        setMuxAddr(1 << bit);
        Serial.printf("%s HIGH — do VOM\n", names[bit]);
        delay(2000);
        setMuxAddr(0);
        Serial.printf("%s LOW\n", names[bit]);
        delay(500);
    }
    Serial.println("=== DONE ===");
}

void scanMux16() {
    static unsigned long lastDebugLog = 0;
    bool shouldDebugLog = (millis() - lastDebugLog > 1000);  // Log mỗi 1 giây
    
    for (uint8_t ch = 0; ch < MUX_CHANNELS; ch++) {
        setMuxAddr(ch);
        delayMicroseconds(500);      // mux settle
        analogRead(MUX_ADC1);        // dummy xả tụ
        analogRead(MUX_ADC2);
        delayMicroseconds(100);
        uint32_t sum1 = 0, sum2 = 0;
        for (int s = 0; s < 4; s++) {
            sum1 += analogRead(MUX_ADC1);
            sum2 += analogRead(MUX_ADC2);
        }
        muxData[0][ch]    = sum1 >> 2;
        muxData[1][ch]    = sum2 >> 2;
        
        // ✅ Kiểm tra logic: nếu raw < threshold thì detected (digital = 1)
        // Nếu muốn inverted, đổi < thành >
        muxDigital[0][ch] = (muxData[0][ch] < muxThreshold[0][ch]) ? 1 : 0;
        muxDigital[1][ch] = (muxData[1][ch] < muxThreshold[1][ch]) ? 1 : 0;
        
        // ✅ DEBUG: Log khi threshold cao
        if (shouldDebugLog && ch == 0) {
            if (muxThreshold[0][0] > 3000) {
                Serial.printf("[SCAN_DEBUG] B0-CH%d: raw=%d thresh=%d → digital=%d (comparison: %d < %d = %s)\n",
                    ch, muxData[0][ch], muxThreshold[0][ch], muxDigital[0][ch],
                    muxData[0][ch], muxThreshold[0][ch],
                    (muxData[0][ch] < muxThreshold[0][ch]) ? "TRUE→digital=1" : "FALSE→digital=0");
            }
        }
    }
    
    if (shouldDebugLog) lastDebugLog = millis();
}

