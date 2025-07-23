#include <Arduino.h>
#include <BleKeyboard.h>
#include <esp_sleep.h>

BleKeyboard bleKeyboard("Trackball-KB", "CustomMod", 100);

// Individual button pins
const int BUTTON_PINS[5] = {2, 4, 12, 13, 14};
const int MASTER_WAKE_PIN = 15; // All buttons also connect here through diodes
const int NUM_BUTTONS = 5;

RTC_DATA_ATTR int bootCount = 0;
const unsigned long ACTIVE_TIMEOUT = 30000;
unsigned long lastButtonPress = 0;

void setupDeepSleep() {
    // Configure individual button pins
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    }
    
    // Configure master wake pin
    pinMode(MASTER_WAKE_PIN, INPUT_PULLUP);
    
    // Only the master wake pin needs to wake the device
    esp_sleep_enable_ext0_wakeup((gpio_num_t)MASTER_WAKE_PIN, 0);
    
    Serial.printf("Wake-up configured: Any button press wakes via GPIO %d\n", MASTER_WAKE_PIN);
}

void goToDeepSleep() {
    Serial.println("=== GOING TO DEEP SLEEP ===");
    Serial.println("Press ANY button to wake up!");
    Serial.flush();
    
    bleKeyboard.end();
    delay(500);
    
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    setCpuFrequencyMhz(80);
    
    bootCount++;
    Serial.printf("\n=== BOOT #%d ===\n", bootCount);
    
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("Wake up: Button press detected!");
    } else {
        Serial.println("Wake up: First boot or reset");
    }
    
    setupDeepSleep();
    
    Serial.println("INITIALIZING - Starting BLE keyboard...");
    bleKeyboard.begin();
    lastButtonPress = millis();
    
    Serial.println("=== 5-BUTTON DIODE-WAKE TRACKBALL MOD READY ===");
}

void loop() {
    // Check all 5 buttons individually
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (digitalRead(BUTTON_PINS[i]) == LOW) {
            lastButtonPress = millis();
            
            if (bleKeyboard.isConnected()) {
                char keyNumber = '1' + i;
                Serial.printf("Button %d (GPIO %d) -> Ctrl+Shift+%c\n", i+1, BUTTON_PINS[i], keyNumber);
                
                // bleKeyboard.press(KEY_LEFT_CTRL);
                // bleKeyboard.press(KEY_LEFT_SHIFT);
                bleKeyboard.press(keyNumber);
                delay(20);
                bleKeyboard.releaseAll();
            }
            
            delay(200);
        }
    }
    
    unsigned long timeSinceLastButton = millis() - lastButtonPress;
    if (timeSinceLastButton >= ACTIVE_TIMEOUT) {
        goToDeepSleep();
    }
    
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 5000) {
        unsigned long timeToSleep = ACTIVE_TIMEOUT - timeSinceLastButton;
        Serial.printf("BLE: %s, Sleep in: %d seconds\n", 
                      bleKeyboard.isConnected() ? "Connected" : "Disconnected",
                      (int)(timeToSleep/1000));
        lastStatus = millis();
    }
    
    delay(100);
}