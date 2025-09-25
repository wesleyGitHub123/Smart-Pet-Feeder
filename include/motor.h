#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

// ========================================
// MOTOR CONTROL MODULE HEADER  
// ========================================
// This module handles stepper motor control for food dispensing
// using DRV8825 driver with NEMA 17 stepper motor

// Motor control functions
void initializeMotor();
void enableMotor();
void disableMotor();

// Feeding functions
void dispensePortion(int steps);
void dispensePortionSmooth(int steps, int maxSpeed = 200, int acceleration = 100);
void manualFeed();
void automaticFeed();

// Calibration and utility functions
void calibrateMotor();
int gramsToSteps(float grams);
float stepsToGrams(int steps);

// Motor status functions
bool isMotorEnabled();
bool isMotorMoving();
void emergencyStop();

// Debug and testing functions
void testMotorMovement();
void printMotorStatus();

#endif // MOTOR_H