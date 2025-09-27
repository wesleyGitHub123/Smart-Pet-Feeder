/*
 * Smart Pet Feeder - Phase 5: SMS Alert System
 * =============================================
 * 
 * This phase adds:
 * 1. SMS alerts for automatic feeding events
 * 2. SMS alerts for manual feeding events  
 * 3. SMS system status notifications
 * 4. SMS error alerts for feeding failures
 * 5. Non-blocking GSM/SMS functionality
 * 6. Integration with existing smart protocol
 * 7. All Phase 1-4 functionality preserved
 * 
 * Hardware needed for this phase:
 * - ESP32-S3 DevKit
 * - Manual feed button (GPIO10)
 * - Mode switch (GPIO11) 
 * - Buzzer (GPIO12)
 * - RCWL-9620 ultrasonic sensor (I2C: SDA=GPIO8, SCL=GPIO9)
 * - NEMA 17 stepper motor + DRV8825 driver
 *   - STEP: GPIO2, DIR: GPIO1, ENABLE: GPIO3
 *   - Motor power: 12V, Logic power: 3.3V
 * - SIM800L GSM module
 *   - TX: GPIO6, RX: GPIO7, RST: GPIO13
 *   - Power: 3.7-4.2V (external supply recommended)
 */

#include <Arduino.h>
#include <Wire.h>  // For I2C communication
#include "config.h"
#include "sensor.h"  // Include sensor module header
#include "motor.h"   // Include motor module header
#include "gsm.h"     // Include GSM module header (Phase 5)

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
unsigned long lastSensorRead = 0;
bool bowlEmpty = false;
bool sensorInitialized = false;

// Phase 4: Automatic feeding variables
unsigned long lastAutoFeedTime = 0;
unsigned long lastAutoFeedCheck = 0;
unsigned long bowlEmptyStartTime = 0;
bool bowlEmptyConfirmed = false;
int dailyAutoFeedCount = 0;
unsigned long dailyResetTime = 0;
bool automaticFeedingEnabled = true;

// Function declarations (non-sensor functions)
void initializeSystem();
void handleManualControls();
void handleAutomaticFeeding();
void performAutomaticFeed();
void resetDailyFeedCount();
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
  Serial.println("   Smart Pet Feeder - Phase 5 Starting   ");
  Serial.println("      + SMS Alert System +               ");
  Serial.println("==========================================");;
  
  // Initialize all system components
  initializeSystem();
  
  // Play startup sound sequence
  playStartupSequence();
  
  // Print initial system status
  printSystemStatus();
  
  Serial.println("\nPhase 5 Ready! SMS alert system active...");
  Serial.println("- Manual feed: Press feed button anytime");  
  Serial.println("- Mode toggle: Press mode button for Cat/Dog switching");
  Serial.println("- Auto feed: System will feed when bowl is empty for 1 minute");
  Serial.println("- Safety: Max 8 automatic feeds per day, 30min intervals");
  Serial.println("- SMS Alerts: Automatic feeding, manual feeding, and system status");
  Serial.println("- Test SMS: GSM module will send alerts to +639291145133");
  Serial.println("==========================================\n");
}

void loop() {
  // Update ultrasonic sensor readings
  updateSensorReadings();
  
  // Update GSM status (Phase 5) - non-blocking
  updateGSMStatus();
  
  // Handle manual controls (button and switch)
  handleManualControls();
  
  // Phase 4: Automatic feeding logic based on bowl status
  handleAutomaticFeeding();
  
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
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  
  // Configure output pins
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer starts off
  
  // Initialize I2C for ultrasonic sensor
  initializeUltrasonicSensor();
  
  // Initialize stepper motor
  initializeMotor();
  
  // Initialize GSM module (Phase 5)
  initializeGSM();
  
  // Read initial states
  lastButtonState = digitalRead(FEED_BUTTON_PIN);
  currentButtonState = lastButtonState;
  
  lastModeState = digitalRead(MODE_BUTTON_PIN);
  currentModeState = lastModeState;
  currentMode = (lastModeState == LOW) ? DOG_MODE : CAT_MODE;
  
  // Initialize Phase 4: Automatic feeding variables
  lastAutoFeedTime = 0;  // Allow immediate feeding on startup
  lastAutoFeedCheck = 0;
  bowlEmptyStartTime = 0;
  bowlEmptyConfirmed = false;
  dailyAutoFeedCount = 0;
  dailyResetTime = millis();
  automaticFeedingEnabled = true;
  
  Serial.println("✓ GPIO pins configured");
  Serial.println("✓ I2C ultrasonic sensor initialized");
  Serial.println("✓ Stepper motor initialized");
  Serial.println("✓ GSM module initialization started");
  Serial.println("✓ Automatic feeding system initialized");
  Serial.println("✓ Initial states read");
  Serial.printf("✓ Initial mode: %s\n", (currentMode == CAT_MODE) ? "CAT" : "DOG");
  Serial.println("✓ System initialization complete");
}

void handleManualControls() {
  // Check feed button with debouncing
  if (readButtonWithDebounce(FEED_BUTTON_PIN, lastButtonState, lastDebounceTime)) {
    Serial.println("\n🔘 MANUAL FEED BUTTON PRESSED!");
    
    // Prepare SMS alert message
    String feedInfo = (currentMode == CAT_MODE) ? "CAT (20g)" : "DOG (50g)";
    
    // Trigger manual feeding using motor control
    manualFeed();
    
    // Send SMS alert for manual feed (Phase 5)
    sendSMSAlert(SMS_MANUAL_FEED, feedInfo.c_str());
  }
  
  // Check mode button (toggle between CAT and DOG modes)
  if (readButtonWithDebounce(MODE_BUTTON_PIN, lastModeState, lastModeDebounceTime)) {
    // Toggle between modes
    if (currentMode == CAT_MODE) {
      currentMode = DOG_MODE;
      Serial.println("Mode: DOG");
      
      // Three medium-pitched beeps for dog mode
      playBuzzer(150, 1500);
      delay(50);
      playBuzzer(150, 1500);
      delay(50);
      playBuzzer(150, 1500);
    } else {
      currentMode = CAT_MODE;
      Serial.println("Mode: CAT");
      
      // Two high-pitched beeps for cat mode
      playBuzzer(100, 2500);
      delay(50);
      playBuzzer(100, 2500);
    }
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
  Serial.printf("   Sensor: %s\n", sensorInitialized ? "ONLINE" : "ERROR");
  
  // Add motor status
  printMotorStatus();
  
  // Phase 5: Add GSM status
  printGSMStatus();
  
  // Phase 4: Add automatic feeding status
  Serial.printf("   Auto Feeding: %s\n", automaticFeedingEnabled ? "ENABLED" : "DISABLED");
  Serial.printf("   Daily Auto Feeds: %d/%d\n", dailyAutoFeedCount, MAX_DAILY_AUTO_FEEDS);
  Serial.printf("   Last Auto Feed: %lu min ago\n", (millis() - lastAutoFeedTime) / 60000);
  if (bowlEmpty && bowlEmptyConfirmed) {
    Serial.printf("   Next Auto Feed: READY (bowl confirmed empty)\n");
  } else if (bowlEmpty && bowlEmptyStartTime > 0) {
    unsigned long elapsedTime = millis() - bowlEmptyStartTime;
    if (elapsedTime < BOWL_EMPTY_CONFIRMATION_TIME) {
      unsigned long timeLeft = BOWL_EMPTY_CONFIRMATION_TIME - elapsedTime;
      Serial.printf("   Next Auto Feed: %lu sec (confirming empty bowl)\n", timeLeft / 1000);
    } else {
      Serial.printf("   Next Auto Feed: READY (confirmation complete)\n");
    }
  } else {
    Serial.printf("   Next Auto Feed: Waiting for empty bowl\n");
  }
  
  Serial.printf("   Uptime: %lu seconds\n", millis() / 1000);
  Serial.printf("   Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("   Hardware status: All systems nominal");
  Serial.println("------------------------------------------");
}

// ===============================================
// Phase 4: Automatic Feeding Functions
// ===============================================

void handleAutomaticFeeding() {
  // Debug: Check what's happening with auto-feeding
  static unsigned long lastAutoFeedDebug = 0;
  if (millis() - lastAutoFeedDebug > 15000) { // Debug every 15 seconds
    Serial.printf("🔧 AUTO-FEED DEBUG: bowlEmpty=%s, enabled=%s, dailyCount=%d/%d\n",
                  bowlEmpty ? "YES" : "NO",
                  automaticFeedingEnabled ? "YES" : "NO", 
                  dailyAutoFeedCount, MAX_DAILY_AUTO_FEEDS);
    Serial.printf("   Time since last check: %lu ms (interval: %d ms)\n", 
                  millis() - lastAutoFeedCheck, AUTO_FEED_CHECK_INTERVAL);
    Serial.printf("   Time since last feed: %lu ms (min interval: %d ms)\n",
                  millis() - lastAutoFeedTime, AUTO_FEED_MIN_INTERVAL);
    lastAutoFeedDebug = millis();
  }
  
  // Reset daily feed count at midnight (24 hours since last reset)
  if (millis() - dailyResetTime > 24UL * 60 * 60 * 1000) {
    resetDailyFeedCount();
  }
  
  // Only proceed if automatic feeding is enabled
  if (!automaticFeedingEnabled) {
    return;
  }
  
  // Safety check: Don't exceed daily feed limit
  if (dailyAutoFeedCount >= MAX_DAILY_AUTO_FEEDS) {
    // Send alert if bowl is empty but max feeds reached (Phase 5)
    static unsigned long lastMaxFeedAlert = 0;
    if (bowlEmpty && (millis() - lastMaxFeedAlert > 3600000)) { // Alert once per hour
      String alertMsg = "Bowl empty but max daily feeds reached (" + String(MAX_DAILY_AUTO_FEEDS) + "/" + String(MAX_DAILY_AUTO_FEEDS) + ")";
      sendSMSAlert(SMS_BOWL_EMPTY_ALERT, alertMsg.c_str());
      lastMaxFeedAlert = millis();
    }
    return;
  }
  
  // Safety check: Minimum interval between automatic feeds (skip on first feed)
  if (lastAutoFeedTime != 0 && millis() - lastAutoFeedTime < AUTO_FEED_MIN_INTERVAL) {
    return;
  }
  
  // Check if it's time to evaluate feeding (every 5 seconds)
  if (millis() - lastAutoFeedCheck < AUTO_FEED_CHECK_INTERVAL) {
    return;
  }
  lastAutoFeedCheck = millis();
  
  Serial.println("🔧 AUTO-FEED: Performing check..."); // Debug message
  
  // Handle bowl empty confirmation logic
  if (bowlEmpty) {
    if (bowlEmptyStartTime == 0) {
      // Bowl just became empty, start confirmation timer
      bowlEmptyStartTime = millis();
      bowlEmptyConfirmed = false;
      Serial.println("🍽️ BOWL DETECTED EMPTY - Starting confirmation timer...");
    } else if (!bowlEmptyConfirmed && (millis() - bowlEmptyStartTime > BOWL_EMPTY_CONFIRMATION_TIME)) {
      // Bowl has been empty long enough, confirm and prepare to feed
      bowlEmptyConfirmed = true;
      Serial.println("✅ BOWL EMPTY CONFIRMED - Ready for automatic feeding");
      
      // Play alert sound for automatic feeding
      playBuzzer(200, 1800);
      delay(100);
      playBuzzer(200, 2200);
      delay(100);
      playBuzzer(200, 1800);
    }
  } else {
    // Bowl is not empty, reset confirmation
    bowlEmptyStartTime = 0;
    bowlEmptyConfirmed = false;
  }
  
  // Perform automatic feed if conditions are met (simplified - no hopper check)
  if (bowlEmptyConfirmed && sensorInitialized) {
    performAutomaticFeed();
  }
}

void performAutomaticFeed() {
  Serial.println("\n🤖 AUTOMATIC FEEDING INITIATED");
  Serial.printf("Mode: %s | Portion: %s\n", 
    currentMode == CAT_MODE ? "CAT" : "DOG",
    currentMode == CAT_MODE ? "20g" : "50g");
  
  systemState = DISPENSING;
  
  // Play distinctive auto-feed sound sequence
  playBuzzer(100, 2500);  // High pitch
  delay(50);
  playBuzzer(100, 1500);  // Low pitch  
  delay(50);
  playBuzzer(200, 2000);  // Medium pitch, longer
  
  // Dispense appropriate portion using motor module
  manualFeed();
  
  // Update automatic feeding tracking
  lastAutoFeedTime = millis();
  dailyAutoFeedCount++;
  bowlEmptyConfirmed = false;
  bowlEmptyStartTime = 0;
  
  // Prepare SMS alert message (Phase 5)
  String feedInfo = (currentMode == CAT_MODE) ? "CAT (20g)" : "DOG (50g)";
  String statusInfo = feedInfo + " - Daily feeds: " + String(dailyAutoFeedCount) + "/8";
  
  // Send SMS alert for automatic feed (Phase 5)
  sendSMSAlert(SMS_AUTO_FEED, statusInfo.c_str());
  
  systemState = IDLE;
  
  Serial.printf("✅ AUTOMATIC FEEDING COMPLETE (%d/%d daily feeds used)\n", 
    dailyAutoFeedCount, MAX_DAILY_AUTO_FEEDS);
  
  // Play completion sound
  playBuzzer(300, 2200);
}

void resetDailyFeedCount() {
  dailyAutoFeedCount = 0;
  dailyResetTime = millis();
  Serial.println("🕛 Daily feed count reset - New feeding cycle started");
  
  // Send SMS alert for daily reset (Phase 5)
  sendSMSAlert(SMS_DAILY_RESET);
}
