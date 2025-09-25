// motor.cpp
// Stepper motor control module for Smart Pet Feeder
// Handles DRV8825 + NEMA 17 stepper motor for food dispensing

#include <Arduino.h>
#include "config.h"
#include "motor.h"

// External global variables (defined in main.cpp)
extern SystemState systemState;
extern FeedingMode currentMode;

// External function declarations (defined in main.cpp)
extern void playBuzzer(int duration, int frequency);

// Motor control variables
bool motorEnabled = false;
bool motorMoving = false;
int currentPosition = 0;
unsigned long lastMotorAction = 0;

// Motor timing variables
unsigned long stepDelay = 2500; // Microseconds between steps (400 Hz default)
const unsigned long MIN_STEP_DELAY = 1000; // Max speed limit (1000 Hz)
const unsigned long MAX_STEP_DELAY = 10000; // Min speed limit (100 Hz)

// ========================================
// MOTOR INITIALIZATION
// ========================================

void initializeMotor() {
  Serial.println("Initializing stepper motor...");
  
  // Configure motor control pins
  pinMode(MOTOR_STEP_PIN, OUTPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  
  // Initialize pins to safe state
  digitalWrite(MOTOR_STEP_PIN, LOW);
  digitalWrite(MOTOR_DIR_PIN, LOW);
  digitalWrite(MOTOR_ENABLE_PIN, HIGH); // Disable motor (active LOW)
  
  Serial.printf("✓ Motor pins configured (STEP:%d, DIR:%d, EN:%d)\n", 
                MOTOR_STEP_PIN, MOTOR_DIR_PIN, MOTOR_ENABLE_PIN);
  
  // Test motor enable/disable
  enableMotor();
  delay(100);
  disableMotor();
  
  Serial.println("✓ Motor initialization complete");
}

// ========================================
// BASIC MOTOR CONTROL
// ========================================

void enableMotor() {
  digitalWrite(MOTOR_ENABLE_PIN, LOW); // Active LOW
  motorEnabled = true;
  Serial.println("Motor enabled");
  delay(2); // Allow motor to energize
}

void disableMotor() {
  digitalWrite(MOTOR_ENABLE_PIN, HIGH); // Disable
  motorEnabled = false;
  motorMoving = false;
  Serial.println("Motor disabled");
}

void emergencyStop() {
  disableMotor();
  motorMoving = false;
  Serial.println("🚨 EMERGENCY STOP - Motor disabled");
  
  // Play warning sound
  for (int i = 0; i < 3; i++) {
    playBuzzer(200, 1000);
    delay(100);
  }
}

// ========================================
// STEPPING FUNCTIONS
// ========================================

void stepMotor(int steps, bool clockwise = true) {
  if (!motorEnabled) {
    Serial.println("Motor not enabled - cannot step");
    return;
  }
  
  motorMoving = true;
  
  // Set direction
  digitalWrite(MOTOR_DIR_PIN, clockwise ? HIGH : LOW);
  delayMicroseconds(5); // Direction setup time
  
  // Execute steps
  for (int i = 0; i < abs(steps); i++) {
    digitalWrite(MOTOR_STEP_PIN, HIGH);
    delayMicroseconds(5); // Pulse width
    digitalWrite(MOTOR_STEP_PIN, LOW);
    delayMicroseconds(stepDelay);
    
    // Update position tracking
    currentPosition += clockwise ? 1 : -1;
    
    // Yield to system occasionally for long moves
    if (i % 50 == 0) {
      yield();
    }
  }
  
  motorMoving = false;
}

void dispensePortion(int steps) {
  if (steps <= 0) {
    Serial.println("Invalid portion size");
    return;
  }
  
  Serial.printf("Dispensing %d steps...\n", steps);
  
  enableMotor();
  stepMotor(steps, true); // Clockwise to dispense
  delay(100); // Allow movement to complete
  disableMotor();
  
  Serial.printf("✓ Portion dispensed (%d steps)\n", steps);
  lastMotorAction = millis();
}

// ========================================
// SMOOTH MOTOR CONTROL WITH ACCELERATION
// ========================================

void dispensePortionSmooth(int steps, int maxSpeed, int acceleration) {
  if (steps <= 0) return;
  
  Serial.printf("Smooth dispensing %d steps (speed:%d, accel:%d)...\n", 
                steps, maxSpeed, acceleration);
  
  enableMotor();
  motorMoving = true;
  
  digitalWrite(MOTOR_DIR_PIN, HIGH); // Clockwise
  delayMicroseconds(5);
  
  // Calculate acceleration profile
  int accelSteps = min(steps / 4, acceleration); // 1/4 of move for accel
  int cruiseSteps = steps - (2 * accelSteps);
  int decelSteps = accelSteps;
  
  unsigned long currentDelay = MAX_STEP_DELAY; // Start slow
  unsigned long targetDelay = 1000000UL / maxSpeed; // Convert Hz to microseconds
  unsigned long delayStep = (currentDelay - targetDelay) / accelSteps;
  
  int stepCount = 0;
  
  // Acceleration phase
  for (int i = 0; i < accelSteps; i++) {
    digitalWrite(MOTOR_STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(MOTOR_STEP_PIN, LOW);
    delayMicroseconds(currentDelay);
    
    currentDelay -= delayStep;
    stepCount++;
    if (stepCount % 20 == 0) yield();
  }
  
  // Cruise phase
  for (int i = 0; i < cruiseSteps; i++) {
    digitalWrite(MOTOR_STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(MOTOR_STEP_PIN, LOW);
    delayMicroseconds(targetDelay);
    
    stepCount++;
    if (stepCount % 20 == 0) yield();
  }
  
  // Deceleration phase
  for (int i = 0; i < decelSteps; i++) {
    digitalWrite(MOTOR_STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(MOTOR_STEP_PIN, LOW);
    delayMicroseconds(currentDelay);
    
    currentDelay += delayStep;
    stepCount++;
    if (stepCount % 20 == 0) yield();
  }
  
  currentPosition += steps;
  motorMoving = false;
  disableMotor();
  
  Serial.printf("✓ Smooth portion complete (%d steps)\n", steps);
  lastMotorAction = millis();
}

// ========================================
// FEEDING FUNCTIONS
// ========================================

void manualFeed() {
  Serial.println("🍽️ Manual feeding triggered");
  
  int portionSteps;
  const char* modeName;
  
  if (currentMode == CAT_MODE) {
    portionSteps = CAT_MIN_PORTION; // Start with minimum portion
    modeName = "CAT";
  } else {
    portionSteps = DOG_MIN_PORTION;
    modeName = "DOG";
  }
  
  Serial.printf("Manual feed: %s mode (%d steps, ~%.1fg)\n", 
                modeName, portionSteps, stepsToGrams(portionSteps));
  
  // Play feeding sound
  playBuzzer(100, 1800);
  delay(50);
  playBuzzer(100, 2200);
  
  // Dispense portion with smooth acceleration
  systemState = MANUAL_FEEDING;
  dispensePortionSmooth(portionSteps, MOTOR_SPEED, MOTOR_ACCELERATION);
  systemState = IDLE;
  
  // Play completion sound
  playBuzzer(150, 2500);
  
  Serial.println("✓ Manual feeding complete");
}

void automaticFeed() {
  Serial.println("🤖 Automatic feeding triggered");
  
  int portionSteps;
  
  if (currentMode == CAT_MODE) {
    portionSteps = (CAT_MIN_PORTION + CAT_MAX_PORTION) / 2; // Medium portion
  } else {
    portionSteps = (DOG_MIN_PORTION + DOG_MAX_PORTION) / 2;
  }
  
  Serial.printf("Auto feed: %s mode (%d steps, ~%.1fg)\n", 
                (currentMode == CAT_MODE) ? "CAT" : "DOG", 
                portionSteps, stepsToGrams(portionSteps));
  
  // Play automatic feeding sound (different from manual)
  for (int i = 0; i < 2; i++) {
    playBuzzer(80, 2000);
    delay(30);
  }
  
  systemState = DISPENSING;
  dispensePortionSmooth(portionSteps, MOTOR_SPEED, MOTOR_ACCELERATION);
  systemState = IDLE;
  
  Serial.println("✓ Automatic feeding complete");
}

// ========================================
// CALIBRATION AND CONVERSION
// ========================================

int gramsToSteps(float grams) {
  // Calibration: approximately 17 steps per gram (to be refined)
  return (int)(grams * 17.0f);
}

float stepsToGrams(int steps) {
  // Inverse calibration
  return (float)steps / 17.0f;
}

void calibrateMotor() {
  Serial.println("🔧 Starting motor calibration...");
  Serial.println("This will dispense test portions for weight measurement");
  
  const int testSteps[] = {100, 500, 1000, 1700}; // Various test amounts
  const int numTests = sizeof(testSteps) / sizeof(testSteps[0]);
  
  for (int i = 0; i < numTests; i++) {
    Serial.printf("\nTest %d: Dispensing %d steps\n", i+1, testSteps[i]);
    Serial.println("Press any key when ready...");
    
    // Wait for user input (simplified - in real implementation might use button)
    delay(5000);
    
    dispensePortion(testSteps[i]);
    Serial.printf("Weigh the dispensed food and record: %d steps = ? grams\n", testSteps[i]);
    delay(3000);
  }
  
  Serial.println("\n✓ Calibration test complete");
  Serial.println("Update STEPS_PER_GRAM in config.h based on measurements");
}

// ========================================
// STATUS AND DEBUG FUNCTIONS
// ========================================

bool isMotorEnabled() {
  return motorEnabled;
}

bool isMotorMoving() {
  return motorMoving;
}

void testMotorMovement() {
  Serial.println("🧪 Testing motor movement...");
  
  enableMotor();
  
  // Test small movements in both directions
  Serial.println("Testing clockwise rotation...");
  stepMotor(200, true);
  delay(1000);
  
  Serial.println("Testing counterclockwise rotation...");
  stepMotor(200, false);
  delay(1000);
  
  // Test smooth movement
  Serial.println("Testing smooth movement...");
  dispensePortionSmooth(100, 200, 50);
  
  disableMotor();
  Serial.println("✓ Motor test complete");
}

void printMotorStatus() {
  Serial.printf("MOTOR: %s | Moving: %s | Position: %d | Last: %lus ago\n",
                motorEnabled ? "ON" : "OFF",
                motorMoving ? "YES" : "NO", 
                currentPosition,
                (millis() - lastMotorAction) / 1000);
}