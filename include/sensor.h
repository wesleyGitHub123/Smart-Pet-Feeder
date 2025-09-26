#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

// ========================================
// SENSOR MODULE HEADER
// ========================================
// This module handles all ultrasonic sensor operations
// for the Smart Pet Feeder project

// Function declarations for sensor initialization
void initializeUltrasonicSensor();

// Function declarations for sensor reading and processing
void updateSensorReadings();
float readUltrasonicDistance();

// Function declarations for status analysis
void analyzeBowlStatus();

// Function declarations for debugging and diagnostics
void printSensorDebug();

// Sensor status getter functions
bool isSensorInitialized();
float getCurrentDistance();
bool isBowlEmpty();

#endif // SENSOR_H