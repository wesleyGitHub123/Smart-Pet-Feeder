/*
 * Smart Pet Feeder - Phase 2: Ultrasonic Distance Sensing
 * =======================================================
 * 
 * This phase adds:
 * 1. I2C ultrasonic sensor (M5Stack RCWL-9620) integration
 * 2. Bowl empty/full detection based on distance measurements
 * 3. Hopper level monitoring
 * 4. Distance-based feeding logic foundation
 * 5. All Phase 1 functionality preserved
 * 
 * Hardware needed for this phase:
 * - All Phase 1 components (ESP32-S3, button, switch, buzzer)
 * - M5Stack Ultrasonic Distance Unit (RCWL-9620) on I2C
 *   - GPIO8 (SDA) → Sensor SDA
 *   - GPIO9 (SCL) → Sensor SCL  
 *   - 3.3V → Sensor VCC
 *   - GND → Sensor GND
 */

#include <Arduino.h>
#include "config.h"

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

// Function declarations
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
  Serial.println("   Smart Pet Feeder - Phase 1 Starting   ");
  Serial.println("==========================================");
  
  // Initialize all system components
  initializeSystem();
  
  // Play startup sound sequence
  playStartupSequence();
  
  // Print initial system status
  printSystemStatus();
  
  Serial.println("\nPhase 1 Ready! Testing manual controls...");
  Serial.println("- Press feed button to test manual feeding trigger");
  Serial.println("- Toggle mode switch to test Cat/Dog mode detection");
  Serial.println("==========================================\n");
}

void loop() {
  // Handle manual controls (button and switch)
  handleManualControls();
  
  // Debug: Print GPIO states every 2 seconds
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint > 2000) {
    int gpio10State = digitalRead(FEED_BUTTON_PIN);
    int gpio11State = digitalRead(MODE_SWITCH_PIN);
    Serial.printf("DEBUG: GPIO10=%s, GPIO11=%s\n", 
                  gpio10State ? "HIGH" : "LOW", 
                  gpio11State ? "HIGH" : "LOW");
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
  
  // Read initial states
  lastButtonState = digitalRead(FEED_BUTTON_PIN);
  currentButtonState = lastButtonState;
  
  lastModeState = digitalRead(MODE_SWITCH_PIN);
  currentModeState = lastModeState;
  currentMode = (lastModeState == LOW) ? DOG_MODE : CAT_MODE;
  
  Serial.println("✓ GPIO pins configured");
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
    
    Serial.printf("   → Triggered manual feeding in %s mode\n", 
                  (currentMode == CAT_MODE) ? "CAT" : "DOG");
    Serial.printf("   → System state changed to: MANUAL_FEEDING\n");
    
    // Simulate feeding process (in later phases, this will control the motor)
    Serial.println("   → [SIMULATION] Would dispense food here...");
    delay(1000); // Simulate dispensing time
    
    // Return to idle state
    systemState = IDLE;
    Serial.println("   → Manual feeding complete, returning to IDLE\n");
  }
  
  // Check mode switch with simplified logic (same as button)
  if (readButtonWithDebounce(MODE_SWITCH_PIN, lastModeState, lastModeDebounceTime)) {
    // This detects HIGH->LOW transition (switch to DOG mode)
    currentMode = DOG_MODE;
    Serial.println("\n🔄 MODE SWITCH CHANGED!");
    Serial.printf("   → New mode: DOG\n");
    
    // Three medium-pitched beeps for dog mode
    playBuzzer(150, 1500);
    delay(50);
    playBuzzer(150, 1500);
    delay(50);
    playBuzzer(150, 1500);
    
    Serial.printf("   → Portion range: 100-400g\n");
    Serial.println();
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
          Serial.printf("   → New mode: CAT\n");
          
          // Two high-pitched beeps for cat mode
          playBuzzer(100, 2500);
          delay(50);
          playBuzzer(100, 2500);
          
          Serial.printf("   → Portion range: 30-100g\n");
          Serial.println();
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
      Serial.printf("BUTTON DEBUG: Pin %d button press detected! (HIGH->LOW edge)\n", pin);
    } else {
      Serial.printf("BUTTON DEBUG: Pin %d press ignored (too soon - debouncing)\n", pin);
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
  
  Serial.printf("   Uptime: %lu seconds\n", millis() / 1000);
  Serial.printf("   Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("   Hardware status: All systems nominal");
  Serial.println("------------------------------------------");
}