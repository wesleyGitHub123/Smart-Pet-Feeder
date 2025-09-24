#ifndef CONFIG_H
#define CONFIG_H

// Pin Definitions for ESP32-S3 Smart Pet Feeder
// ===============================================

// === STEPPER MOTOR (DRV8825) ===
#define MOTOR_STEP_PIN    2   // GPIO2 - Step signal
#define MOTOR_DIR_PIN     1   // GPIO1 - Direction signal
#define MOTOR_ENABLE_PIN  42  // GPIO42 - Enable (active LOW)

// === I2C ULTRASONIC SENSOR (RCWL-9620) ===
#define I2C_SDA_PIN       8   // GPIO8 - SDA
#define I2C_SCL_PIN       9   // GPIO9 - SCL
#define ULTRASONIC_ADDR   0x57 // I2C address for RCWL-9620

// === MANUAL CONTROLS ===
#define FEED_BUTTON_PIN   10  // GPIO10 - Manual feed button
#define MODE_SWITCH_PIN   11  // GPIO11 - Cat/Dog mode switch

// === ALERTS ===
#define BUZZER_PIN        12  // GPIO12 - Buzzer for alerts

// === GSM MODULE (SIM800L) ===
#define GSM_RX_PIN        17  // GPIO17 - Connect to SIM800L TX
#define GSM_TX_PIN        18  // GPIO18 - Connect to SIM800L RX
#define GSM_RESET_PIN     13  // GPIO13 - Reset pin for SIM800L

// === SYSTEM PARAMETERS ===
// Bowl detection thresholds (in cm)
#define BOWL_EMPTY_THRESHOLD    15  // If distance > 15cm, bowl is empty
#define BOWL_FULL_THRESHOLD     8   // If distance < 8cm, bowl has food

// Hopper monitoring (approximate distances in cm)
#define HOPPER_EMPTY_THRESHOLD  25  // Distance indicating hopper is empty
#define HOPPER_LOW_THRESHOLD    20  // Distance indicating hopper is low

// Feeding portions (in steps - will calibrate in testing)
#define CAT_MIN_PORTION         500   // ~30g equivalent in motor steps
#define CAT_MAX_PORTION         1700  // ~100g equivalent in motor steps
#define DOG_MIN_PORTION         1700  // ~100g equivalent in motor steps
#define DOG_MAX_PORTION         6800  // ~400g equivalent in motor steps

// Motor settings
#define MOTOR_SPEED            200   // Steps per second
#define MOTOR_ACCELERATION     100   // Steps per second^2

// Timing
#define DEBOUNCE_DELAY         50    // Button debounce in ms
#define SENSOR_READ_INTERVAL   1000  // Ultrasonic read interval in ms
#define HOPPER_CHECK_INTERVAL  30000 // Check hopper every 30 seconds

// System states
enum FeedingMode {
  CAT_MODE = 0,
  DOG_MODE = 1
};

enum SystemState {
  IDLE,
  CHECKING_BOWL,
  DISPENSING,
  ALERT_EMPTY_HOPPER,
  MANUAL_FEEDING,
  ERROR_STATE
};

#endif // CONFIG_H