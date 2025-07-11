#include <Arduino.h>
#include <BleKeyboard.h>

BleKeyboard bleKeyboard("Trackball-Keys", "CustomMod", 100);

// Working GPIO pins
const int BUTTON_PINS[9] = {2, 4, 5, 12, 13, 14, 19, 17, 18};
bool lastButtonState[9] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool buttonPressed[9] = {false, false, false, false, false, false, false, false, false};
unsigned long lastDebounceTime[9] = {0};
const unsigned long DEBOUNCE_DELAY = 50;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Setup all button pins with pullups
    for (int i = 0; i < 9; i++) {
        pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    }
    
    Serial.println("Starting 9-button keyboard mod...");
    bleKeyboard.begin();
    Serial.println("Buttons send Ctrl+Shift+1 through Ctrl+Shift+9");
}

void loop() {
    //Checking power consumption
    //Serial.printf("CPU Freq: %dMHz, Free heap: %d\n", getCpuFrequencyMhz(), ESP.getFreeHeap());
    if (bleKeyboard.isConnected()) {
        // Check each button
        for (int i = 0; i < 9; i++) {
            bool currentState = digitalRead(BUTTON_PINS[i]);
            
            // Debounce logic
            if (currentState != lastButtonState[i]) {
                lastDebounceTime[i] = millis();
            }
            
            if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
                // Button was just pressed (went from HIGH to LOW)
                if (currentState == LOW && !buttonPressed[i]) {
                    buttonPressed[i] = true;
                    
                    // Send Ctrl+Shift+(1-9)
                    char keyNumber = '1' + i;
                    
                    Serial.printf("Button %d (GPIO %d) -> Ctrl+Shift+%c\n", 
                                  i+1, BUTTON_PINS[i], keyNumber);
                    
                    // bleKeyboard.press(KEY_LEFT_CTRL);
                    // bleKeyboard.press(KEY_LEFT_SHIFT);
                    bleKeyboard.press(keyNumber);
                    delay(20); // Brief hold
                    bleKeyboard.releaseAll();
                }
                // Button was released (went from LOW to HIGH)
                else if (currentState == HIGH && buttonPressed[i]) {
                    buttonPressed[i] = false;
                    Serial.printf("Button %d released\n", i+1);
                }
            }
            
            lastButtonState[i] = currentState;
        }
    } else {
        static unsigned long lastMessage = 0;
        if (millis() - lastMessage > 5000) {
            Serial.println("Waiting for BLE connection...");
            lastMessage = millis();
        }
    }
    
    delay(10);
}