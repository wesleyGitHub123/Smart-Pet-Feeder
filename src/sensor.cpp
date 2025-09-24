// sensor.cpp
// Ultrasonic sensor module for Smart Pet Feeder
// Handles RCWL-9620 I2C ultrasonic sensor operations

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "sensor.h"

// External global variables (defined in main.cpp)
extern float currentDistance;
extern float bowlDistance;
extern float hopperDistance;
extern unsigned long lastSensorRead;
extern bool bowlEmpty;
extern bool hopperLow;
extern bool sensorInitialized;
extern SystemState systemState;

// External function declarations (defined in main.cpp)
extern void playBuzzer(int duration, int frequency);


// ========================================
// PHASE 2: ULTRASONIC SENSOR FUNCTIONS
// ========================================

void initializeUltrasonicSensor() {
  Serial.println("Initializing RCWL-9620 sensor...");
  
  // Initialize I2C with custom pins and slower speed for stability
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(50000); // 50kHz for better stability with RCWL-9620
  Wire.setTimeout(1000); // 1 second timeout
  
  // Give the sensor time to initialize
  delay(100);
  
  // Test sensor communication with multiple attempts
  bool sensorFound = false;
  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.printf("Sensor attempt %d/3...\n", attempt);
    
    Wire.beginTransmission(ULTRASONIC_ADDR);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      sensorFound = true;
      Serial.printf("✓ Sensor found at 0x%02X\n", ULTRASONIC_ADDR);
      break;
    } else {
      Serial.printf("Attempt %d failed (error %d)\n", attempt, error);
    }
    delay(100); // Wait before retry
  }
  
  if (sensorFound) {
    sensorInitialized = true;
    
    // Take initial reading
    float reading = readUltrasonicDistance();
    if (reading > 0) {
      currentDistance = reading;
      Serial.printf("✓ Initial reading: %.1f cm\n", reading);
    }
  } else {
    sensorInitialized = false;
    Serial.printf("✗ ERROR: RCWL-9620 sensor not found at address 0x%02X\n", ULTRASONIC_ADDR);
    Serial.println("   Check wiring: SDA=GPIO8, SCL=GPIO9, VCC=3.3V, GND=GND");
    Serial.println("   Verify sensor address and I2C connections");
  }
}

void updateSensorReadings() {
  // Only update sensor readings at specified intervals
  if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL) {
    if (sensorInitialized) {
      float newDistance = readUltrasonicDistance();
      
      if (newDistance > 0) {
        currentDistance = newDistance;
        
        // Analyze bowl and hopper status based on distance
        analyzeBowlStatus();
        analyzeHopperStatus();
      }
    }
    
    lastSensorRead = millis();
  }
}

float readUltrasonicDistance() {
  if (!sensorInitialized) {
    return -1.0; // Error indicator
  }
  
  // RCWL-9620 specific communication protocol
  // Step 1: Send measurement trigger command
  Wire.beginTransmission(ULTRASONIC_ADDR);
  Wire.write(0x01); // Trigger measurement command
  byte error = Wire.endTransmission();
  
  if (error != 0) {
    // Try to reinitialize I2C on error 5 (timeout)
    if (error == 5) {
      Serial.println("I2C timeout, reinitializing...");
      Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
      Wire.setClock(100000);
      delay(10);
      
      // Retry the command
      Wire.beginTransmission(ULTRASONIC_ADDR);
      Wire.write(0x01);
      error = Wire.endTransmission();
      
      if (error != 0) {
        Serial.printf("Sensor write error after retry: %d\n", error);
        return -1.0;
      }
    } else {
      Serial.printf("Sensor write error: %d\n", error);
      return -1.0;
    }
  }
  
  // Step 2: Wait for measurement completion (RCWL-9620 needs ~60-80ms)
  delay(80);
  
  // Step 3: Read the distance data (3 bytes: high, low, checksum)
  Wire.requestFrom(ULTRASONIC_ADDR, 3);
  
  if (Wire.available() >= 3) {
    byte highByte = Wire.read();
    byte lowByte = Wire.read();
    byte checksumReceived = Wire.read();
    
    // Calculate distance in mm, then convert to cm
    uint16_t distance_mm = (highByte << 8) | lowByte;
    float distance_cm = distance_mm / 10.0;
    
    // RCWL-9620 checksum calculation: sum of high and low bytes
    byte calculatedChecksum = (highByte + lowByte) & 0xFF;
    
    // Validate the reading
    if (distance_cm > 0 && distance_cm < 500) { // Valid range check
      // Accept reading but show checksum status
      if (checksumReceived != calculatedChecksum) {
        Serial.printf("Checksum error (got:0x%02X calc:0x%02X) - accepting %.1f cm\n", 
                     checksumReceived, calculatedChecksum, distance_cm);
      }
      return distance_cm;
    } else {
      Serial.printf("Distance out of range: %.1f cm\n", distance_cm);
      return -1.0;
    }
  } else {
    Serial.printf("Insufficient sensor data: %d bytes\n", Wire.available());
    return -1.0;
  }
}

void analyzeBowlStatus() {
  bool previousBowlEmpty = bowlEmpty;
  
  // Determine if bowl is empty based on distance thresholds
  if (currentDistance > BOWL_EMPTY_THRESHOLD) {
    bowlEmpty = true;
  } else if (currentDistance < BOWL_FULL_THRESHOLD) {
    bowlEmpty = false;
  }
  // Use hysteresis - don't change state if in middle range
  
  // Alert if bowl status changed
  if (bowlEmpty != previousBowlEmpty) {
    if (bowlEmpty) {
      Serial.println("🍽️ ALERT: Bowl is now EMPTY!");
      // Play alerting beeps
      playBuzzer(100, 1000);
      delay(50);
      playBuzzer(100, 1000);
    } else {
      Serial.println("🍽️ INFO: Bowl now has food");
      playBuzzer(100, 2000); // Happy beep
    }
  }
}

void analyzeHopperStatus() {
  bool previousHopperLow = hopperLow;
  
  // For hopper monitoring, we'd need a second sensor or different positioning
  // For now, simulate hopper status based on very high distances
  // (This would be refined based on actual hardware setup)
  if (currentDistance > HOPPER_EMPTY_THRESHOLD) {
    hopperLow = true;
  } else if (currentDistance < HOPPER_LOW_THRESHOLD) {
    hopperLow = false;
  }
  
  // Alert if hopper status changed to low/empty
  if (hopperLow && !previousHopperLow) {
    Serial.println("🥫 ALERT: Hopper is running LOW or EMPTY!");
    systemState = ALERT_EMPTY_HOPPER;
    
    // Play urgent alert sequence
    for (int i = 0; i < 3; i++) {
      playBuzzer(200, 800);
      delay(100);
    }
    
    systemState = IDLE;
  }
}

void printSensorDebug() {
  if (sensorInitialized) {
    Serial.printf("SENSOR: %.1f cm | Bowl: %s | Hopper: %s\n", 
                  currentDistance,
                  bowlEmpty ? "EMPTY" : "OK",
                  hopperLow ? "LOW" : "OK");
  } else {
    Serial.println("SENSOR: ERROR - Not initialized");
  }
}

// ========================================
// SENSOR STATUS GETTER FUNCTIONS
// ========================================

bool isSensorInitialized() {
  return sensorInitialized;
}

float getCurrentDistance() {
  return currentDistance;
}

bool isBowlEmpty() {
  return bowlEmpty;
}

bool isHopperLow() {
  return hopperLow;
}