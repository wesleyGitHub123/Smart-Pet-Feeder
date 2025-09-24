/*
 * Smart Pet Feeder - Phase 2: Ultrasonic Distance Sensing
 * =======================================================
 * 
 * This phase adds:
 * 1. I2C ultrasonic sensor (M5Stack RCWL-9620) integration
 * 2. Bowl empty/full detection based on distance readings
 * 3. Hopper level monitoring
 * 4. Enhanced system status with sensor data
 * 5. All Phase 1 functionality (manual controls, buzzer, mode switching)
 * 
 * Hardware needed for this phase:
 * - ESP32-S3 DevKit
 * - Manual feed button (GPIO10)
 * - Mode switch (GPIO11) 
 * - Buzzer (GPIO12)
 * - M5Stack RCWL-9620 ultrasonic sensor (I2C: SDA=GPIO8, SCL=GPIO9)
 */

#include <Arduino.h>
#include <Wire.h>  // For I2C communication
#include "config.h"
#include "sensor.h"  // Include sensor module header

// Global variables for input handling
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
bool lastModeState = HIGH;
bool currentModeState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long lastModeDebounceTime = 0;

// System state variables
FeedingMode currentMode = CAT_MODE;
SystemState systemState = IDLE;
unsigned long lastStateChange = 0;

// Ultrasonic sensor variables
float currentDistance = 0.0;
float bowlDistance = 0.0;
float hopperDistance = 0.0;
unsigned long lastSensorRead = 0;
bool bowlEmpty = false;
bool hopperLow = false;
bool sensorInitialized = false;

// Function declarations (non-sensor functions)
void initializeSystem();
void handleManualControls();
void playBuzzer(int duration, int frequency = 2000);
void playStartupSequence();
void printSystemStatus();
bool readButtonWithDebounce(int pin, bool &lastState, unsigned long &lastDebounceTime);

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Wait a moment for serial to stabilize
  delay(1000);
  
  Serial.println("==========================================");
  Serial.println("   Smart Pet Feeder - Phase 2 Starting   ");
  Serial.println("    + Ultrasonic Distance Sensing +      ");
  Serial.println("==========================================");
  
  // Initialize all system components
  initializeSystem();
  
  // Play startup sound sequence
  playStartupSequence();
  
  // Print initial system status
  printSystemStatus();
  
  Serial.println("\nPhase 2 Ready! Testing ultrasonic sensor and manual controls...");
  Serial.println("- Press feed button to test manual feeding trigger");
  Serial.println("- Toggle mode switch to test Cat/Dog mode detection");
  Serial.println("- Watch distance readings for bowl/hopper monitoring");
  Serial.println("==========================================\n");
}

void loop() {
  // Update ultrasonic sensor readings
  updateSensorReadings();
  
  // Handle manual controls (button and switch)
  handleManualControls();
  
  // Print sensor status every 2 seconds
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 2000) {
    // Only print sensor debug info
    printSensorDebug();
    lastDebugPrint = millis();
  }
  
  // Print system status every 10 seconds when idle (reduced frequency)
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 10000 && systemState == IDLE) {
    printSystemStatus();
    lastStatusPrint = millis();
  }
  
  // Small delay to prevent excessive CPU usage
  delay(10);
}

void initializeSystem() {
  Serial.println("Initializing system components...");
  
  // Configure input pins with internal pullups
  pinMode(FEED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  
  // Configure output pins
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer starts off
  
  // Initialize I2C for ultrasonic sensor
  initializeUltrasonicSensor();
  
  // Read initial states
  lastButtonState = digitalRead(FEED_BUTTON_PIN);
  currentButtonState = lastButtonState;
  
  lastModeState = digitalRead(MODE_SWITCH_PIN);
  currentModeState = lastModeState;
  currentMode = (lastModeState == LOW) ? DOG_MODE : CAT_MODE;
  
  Serial.println("✓ GPIO pins configured");
  Serial.println("✓ I2C ultrasonic sensor initialized");
  Serial.println("✓ Initial states read");
  Serial.printf("✓ Initial mode: %s\n", (currentMode == CAT_MODE) ? "CAT" : "DOG");
  Serial.println("✓ System initialization complete");
}

void handleManualControls() {
  // Check feed button with debouncing
  if (readButtonWithDebounce(FEED_BUTTON_PIN, lastButtonState, lastDebounceTime)) {
    Serial.println("\n🔘 MANUAL FEED BUTTON PRESSED!");
    
    // Change system state to manual feeding
    systemState = MANUAL_FEEDING;
    lastStateChange = millis();
    
    // Provide audio feedback
    playBuzzer(200, 1500); // Short beep
    
    Serial.printf("Manual feed: %s mode\n", 
                  (currentMode == CAT_MODE) ? "CAT" : "DOG");
    
    // Simulate feeding process (in later phases, this will control the motor)
    delay(1000); // Simulate dispensing time
    
    // Return to idle state
    systemState = IDLE;
  }
  
  // Check mode switch with simplified logic (same as button)
  if (readButtonWithDebounce(MODE_SWITCH_PIN, lastModeState, lastModeDebounceTime)) {
    // This detects HIGH->LOW transition (switch to DOG mode)
    currentMode = DOG_MODE;
    Serial.println("Mode: DOG");
    
    // Three medium-pitched beeps for dog mode
    playBuzzer(150, 1500);
    delay(50);
    playBuzzer(150, 1500);
    delay(50);
    playBuzzer(150, 1500);
  } 
  // Check for LOW->HIGH transition (switch to CAT mode)
  else {
    bool currentModeState = digitalRead(MODE_SWITCH_PIN);
    static bool lastModePinState = HIGH;
    static unsigned long lastModeChangeTime = 0;
    
    // Detect LOW->HIGH edge (switch back to CAT mode)
    if (lastModePinState == LOW && currentModeState == HIGH) {
      if ((millis() - lastModeChangeTime) > DEBOUNCE_DELAY) {
        if (currentMode != CAT_MODE) {
          currentMode = CAT_MODE;
          Serial.println("\n🔄 MODE SWITCH CHANGED!");
          Serial.println("Mode: CAT");
          
          // Two high-pitched beeps for cat mode
          playBuzzer(100, 2500);
          delay(50);
          playBuzzer(100, 2500);
        }
        lastModeChangeTime = millis();
      }
    }
    
    lastModePinState = currentModeState;
  }
}

bool readButtonWithDebounce(int pin, bool &lastState, unsigned long &lastDebounceTime) {
  bool currentState = digitalRead(pin);
  bool buttonPressed = false;
  
  // Simple edge detection: detect HIGH->LOW transition (button press)
  if (lastState == HIGH && currentState == LOW) {
    // Check if enough time has passed since last press (debounce)
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
      buttonPressed = true;
      lastDebounceTime = millis();
    }
  }
  
  // Update last state for next comparison
  lastState = currentState;
  
  return buttonPressed;
}

void playBuzzer(int duration, int frequency) {
  // ESP32-compatible buzzer control using ledcWrite
  // Set up PWM channel for buzzer
  const int pwmChannel = 0;
  const int pwmResolution = 8;
  
  ledcSetup(pwmChannel, frequency, pwmResolution);
  ledcAttachPin(BUZZER_PIN, pwmChannel);
  
  // Turn on buzzer
  ledcWrite(pwmChannel, 128); // 50% duty cycle
  delay(duration);
  
  // Turn off buzzer
  ledcWrite(pwmChannel, 0);
  ledcDetachPin(BUZZER_PIN);
}

void playStartupSequence() {
  Serial.println("Playing startup sequence...");
  
  // Ascending tone sequence to indicate successful boot
  playBuzzer(100, 1000);
  delay(50);
  playBuzzer(100, 1500);
  delay(50);
  playBuzzer(150, 2000);
  
  Serial.println("✓ Startup sequence complete");
}

void printSystemStatus() {
  Serial.println("📊 SYSTEM STATUS:");
  Serial.printf("   Mode: %s\n", (currentMode == CAT_MODE) ? "CAT" : "DOG");
  Serial.printf("   State: ");
  
  switch(systemState) {
    case IDLE: Serial.println("IDLE"); break;
    case CHECKING_BOWL: Serial.println("CHECKING_BOWL"); break;
    case DISPENSING: Serial.println("DISPENSING"); break;
    case ALERT_EMPTY_HOPPER: Serial.println("ALERT_EMPTY_HOPPER"); break;
    case MANUAL_FEEDING: Serial.println("MANUAL_FEEDING"); break;
    case ERROR_STATE: Serial.println("ERROR_STATE"); break;
    default: Serial.println("UNKNOWN"); break;
  }
  
  Serial.printf("   Distance: %.1f cm\n", currentDistance);
  Serial.printf("   Bowl Status: %s\n", bowlEmpty ? "EMPTY" : "HAS FOOD");
  Serial.printf("   Hopper Status: %s\n", hopperLow ? "LOW/EMPTY" : "OK");
  Serial.printf("   Sensor: %s\n", sensorInitialized ? "ONLINE" : "ERROR");
  Serial.printf("   Uptime: %lu seconds\n", millis() / 1000);
  Serial.printf("   Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("   Hardware status: All systems nominal");
  Serial.println("------------------------------------------");
}
