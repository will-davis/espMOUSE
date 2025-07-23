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
const unsigned long DEBOUNCE_DELAY = 10;
unsigned long lastButtonPress = 0;
unsigned long buttonLastPress[5] = {0}; // Debounce timing for each button
bool buttonState[5] = {HIGH, HIGH, HIGH, HIGH, HIGH}; // Track current state
bool lastButtonState[5] = {HIGH, HIGH, HIGH, HIGH, HIGH}; // Track previous state
bool buttonTriggered[5] = {false, false, false, false, false}; // Track if button has been triggered

// Note: BleKeyboard library doesn't support custom callbacks
// Connection state is tracked via bleKeyboard.isConnected()

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
    // Check all 5 buttons individually with press-on-down and repeat lockout
    for (int i = 0; i < NUM_BUTTONS; i++) {
        // Read current button state
        bool currentReading = digitalRead(BUTTON_PINS[i]);
        
        // Always detect state changes immediately
        if (currentReading != lastButtonState[i]) {
            // State has changed - check if enough time has passed for debouncing
            if ((millis() - buttonLastPress[i]) > DEBOUNCE_DELAY) {
                buttonLastPress[i] = millis(); // Reset debounce timer
                
                // Update the button state
                buttonState[i] = currentReading;
                
                // Trigger action on button PRESS (transition from HIGH to LOW)
                // But only if we haven't already triggered for this press cycle
                if (lastButtonState[i] == HIGH && buttonState[i] == LOW && !buttonTriggered[i]) {
                    buttonTriggered[i] = true; // Lock out further triggers
                    lastButtonPress = millis(); // Update activity timer
                    
                    if (bleKeyboard.isConnected()) {
                        char keyNumber = '1' + i;
                        Serial.printf("Button %d (GPIO %d) PRESSED -> Key '%c'\n", i+1, BUTTON_PINS[i], keyNumber);
                        
                        // Commented out for testing - uncomment when ready for modifier keys
                        // bleKeyboard.press(KEY_LEFT_CTRL);
                        // bleKeyboard.press(KEY_LEFT_SHIFT);
                        bleKeyboard.press(keyNumber);
                        // delay(20);
                        bleKeyboard.releaseAll();
                    } else {
                        Serial.printf("Button %d pressed but BLE not connected\n", i+1);
                    }
                }
                
                // Reset the trigger lock when button is released
                if (lastButtonState[i] == LOW && buttonState[i] == HIGH) {
                    buttonTriggered[i] = false; // Ready for next press
                }
                
                // Update the last button state after processing
                lastButtonState[i] = currentReading;
            }
            // If debounce time hasn't passed, we ignore this change (but keep checking)
        }
    }
    
    // Check for sleep timeout
    unsigned long timeSinceLastButton = millis() - lastButtonPress;
    if (timeSinceLastButton >= ACTIVE_TIMEOUT) {
        goToDeepSleep();
    }
    
    // Status reporting every 5 seconds with connection state tracking
    static unsigned long lastStatus = 0;
    static bool wasConnected = false;
    bool isConnected = bleKeyboard.isConnected();
    
    // Log connection state changes
    if (isConnected != wasConnected) {
        if (isConnected) {
            Serial.println("BLE: Device connected");
        } else {
            Serial.println("BLE: Device disconnected");
        }
        wasConnected = isConnected;
    }
    
    if (millis() - lastStatus > 5000) {
        unsigned long timeToSleep = ACTIVE_TIMEOUT - timeSinceLastButton;
        Serial.printf("BLE: %s, Sleep in: %d seconds\n", 
                      isConnected ? "Connected" : "Disconnected",
                      (int)(timeToSleep/1000));
        lastStatus = millis();
    }
    
    delay(1);
}