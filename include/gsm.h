#ifndef GSM_H
#define GSM_H

#include <Arduino.h>
#include <HardwareSerial.h>

// ========================================
// GSM MODULE HEADER - PHASE 5
// ========================================
// This module handles SIM800L GSM communication for SMS alerts
// Hardware: SIM800L module connected to ESP32-S3
// Connections: TXD→GPIO6, RXD→GPIO7, RST→GPIO13

// GSM module status
enum GSMStatus {
  GSM_OFFLINE = 0,
  GSM_INITIALIZING,
  GSM_NETWORK_SEARCHING,
  GSM_NETWORK_CONNECTED,
  GSM_SMS_READY,
  GSM_ERROR
};

// SMS alert types with priority levels
enum SMSAlertType {
  SMS_AUTO_FEED = 0,        // HIGH PRIORITY - feeding events
  SMS_MANUAL_FEED,          // HIGH PRIORITY - user actions
  SMS_FEEDING_ERROR,        // HIGH PRIORITY - system failures
  SMS_DAILY_RESET,          // MEDIUM PRIORITY - daily events
  SMS_BOWL_EMPTY_ALERT,     // MEDIUM PRIORITY - status warnings
  SMS_SYSTEM_STATUS         // LOW PRIORITY - routine monitoring
};

// SMS priority levels
enum SMSPriority {
  SMS_PRIORITY_HIGH = 0,    // Auto-feed, manual feed, errors
  SMS_PRIORITY_MEDIUM = 1,  // Daily events, bowl warnings  
  SMS_PRIORITY_LOW = 2      // System status, diagnostics
};

// GSM initialization and control functions
void initializeGSM();
bool isGSMReady();
GSMStatus getGSMStatus();
void updateGSMStatus();
void resetGSMModule();

// SMS functions (priority-based, non-blocking)
void sendSMSAlert(SMSAlertType alertType);
void sendSMSAlert(SMSAlertType alertType, const char* additionalInfo);
void sendCustomSMS(const char* phoneNumber, const char* message, SMSPriority priority = SMS_PRIORITY_MEDIUM);
void sendHighPrioritySMS(const char* phoneNumber, const char* message);

// Internal SMS queue management
SMSPriority getSMSPriority(SMSAlertType alertType);
void queueSMS(const char* phoneNumber, const char* message, SMSPriority priority);
bool processSMSQueue();

// Status and utility functions
void printGSMStatus();
void testGSMModule();
bool checkNetworkConnection();

// Internal helper functions (declared for completeness)
bool sendATCommand(const char* command, const char* expectedResponse, unsigned long timeout = 5000);
void processGSMResponse();
String formatPhoneNumber(const char* number);

// Testing phone number (Philippines format)
#define TEST_PHONE_NUMBER "+639291145133"

// SMS message templates (examples for testing)
/*
 * Example SMS messages for testing with +639291145133:
 * 
 * Auto Feed: "🤖 Smart Pet Feeder: Auto-fed CAT (20g) - Bowl was empty. Time: 14:30. Daily feeds: 3/8"
 * Manual Feed: "👤 Smart Pet Feeder: Manual feed DOG (50g) by button press. Time: 09:15"
 * System Status: "📊 Smart Pet Feeder Status: Online | Mode: CAT | Bowl: Empty | Auto feeds today: 2/8"
 * Error Alert: "⚠️ Smart Pet Feeder ERROR: Feeding failed after 3 attempts. Check mechanism. Time: 16:45"
 * Bowl Alert: "🍽️ Smart Pet Feeder: Bowl empty for 5+ minutes but max daily feeds reached (8/8)"
 * Daily Reset: "🌅 Smart Pet Feeder: New day started. Feed counter reset. Auto feeding enabled."
 */

#endif // GSM_H