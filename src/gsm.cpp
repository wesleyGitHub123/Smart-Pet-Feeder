/*
 * GSM MODULE IMPLEMENTATION - PHASE 5
 * ===================================
 * 
 * This module handles SIM800L GSM communication for SMS alerts in the Smart Pet Feeder.
 * Designed to be non-blocking and maintain compatibility with existing Phase 1-4 functionality.
 * 
 * Hardware connections:
 * - SIM800L TX → ESP32 GPIO6 (GSM_RX_PIN)
 * - SIM800L RX → ESP32 GPIO7 (GSM_TX_PIN) 
 * - SIM800L RST → ESP32 GPIO13 (GSM_RESET_PIN)
 * - SIM800L VCC → 3.7V-4.2V (use external power supply, not ESP32)
 * - SIM800L GND → Common ground
 * 
 * Features:
 * - Non-blocking SMS sending
 * - Automatic GSM status monitoring
 * - Integration with existing feeding logic
 * - Philippines phone number format support
 * - Simple error handling that doesn't halt main system
 */

#include "gsm.h"
#include "config.h"
#include <Arduino.h>

// Global GSM variables
HardwareSerial gsmSerial(1); // Use Serial1 for SIM800L communication
GSMStatus currentGSMStatus = GSM_OFFLINE;
unsigned long lastGSMStatusCheck = 0;
unsigned long gsmInitStartTime = 0;
bool gsmInitialized = false;
bool smsInProgress = false;
unsigned long lastSMSSendTime = 0;

// Priority-based SMS queue system
struct SMSQueueItem {
  String phoneNumber;
  String message;
  SMSPriority priority;
  unsigned long queueTime;
};

const int MAX_SMS_QUEUE = 5;
SMSQueueItem smsQueue[MAX_SMS_QUEUE];
int queueHead = 0;
int queueTail = 0;
int queueCount = 0;

// Priority-based rate limiting
unsigned long lastHighPrioritySMS = 0;
unsigned long lastMediumPrioritySMS = 0;
unsigned long lastLowPrioritySMS = 0;

void initializeGSM() {
  Serial.println("📱 Initializing GSM module (SIM800L)...");
  
  // Configure reset pin
  pinMode(GSM_RESET_PIN, OUTPUT);
  digitalWrite(GSM_RESET_PIN, HIGH);
  
  // Initialize hardware serial for GSM communication
  gsmSerial.begin(GSM_BAUD_RATE, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
  
  // Wait for serial to stabilize
  delay(1000);
  
  // Perform hardware reset
  Serial.println("📱 Performing GSM hardware reset...");
  digitalWrite(GSM_RESET_PIN, LOW);
  delay(100);
  digitalWrite(GSM_RESET_PIN, HIGH);
  delay(3000); // Allow module to boot
  
  // Start initialization sequence
  currentGSMStatus = GSM_INITIALIZING;
  gsmInitStartTime = millis();
  gsmInitialized = false;
  
  Serial.println("📱 GSM module reset complete, starting initialization...");
  
  // Clear any pending data
  while(gsmSerial.available()) {
    gsmSerial.read();
  }
  
  // Send initial AT command
  gsmSerial.println("AT");
  
  Serial.println("📱 GSM initialization started");
}

bool isGSMReady() {
  return (currentGSMStatus == GSM_SMS_READY);
}

GSMStatus getGSMStatus() {
  return currentGSMStatus;
}

void updateGSMStatus() {
  // Don't update too frequently
  if (millis() - lastGSMStatusCheck < GSM_STATUS_CHECK_INTERVAL) {
    return;
  }
  lastGSMStatusCheck = millis();
  
  // Handle different states
  switch (currentGSMStatus) {
    case GSM_INITIALIZING:
      if (millis() - gsmInitStartTime > GSM_INIT_TIMEOUT) {
        Serial.println("📱 GSM initialization timeout, setting to error state");
        currentGSMStatus = GSM_ERROR;
        break;
      }
      
      // Try basic AT command
      if (sendATCommand("AT", "OK", 2000)) {
        Serial.println("📱 GSM responds to AT commands");
        currentGSMStatus = GSM_NETWORK_SEARCHING;
        
        // Set text mode for SMS
        gsmSerial.println("AT+CMGF=1");
        delay(1000);
      }
      break;
      
    case GSM_NETWORK_SEARCHING:
      // Check network registration
      if (sendATCommand("AT+CREG?", "+CREG: 0,1", 5000) || 
          sendATCommand("AT+CREG?", "+CREG: 0,5", 5000)) {
        Serial.println("📱 GSM network connected");
        currentGSMStatus = GSM_NETWORK_CONNECTED;
      } else if (millis() - gsmInitStartTime > GSM_INIT_TIMEOUT * 2) {
        Serial.println("📱 GSM network connection timeout");
        currentGSMStatus = GSM_ERROR;
      }
      break;
      
    case GSM_NETWORK_CONNECTED:
      // Check if SMS is ready
      if (sendATCommand("AT+CMGF=1", "OK", 2000)) {
        Serial.println("📱 GSM SMS ready");
        currentGSMStatus = GSM_SMS_READY;
        gsmInitialized = true;
      }
      break;
      
    case GSM_SMS_READY:
      // Periodically verify connection is still active
      static unsigned long lastConnectionCheck = 0;
      if (millis() - lastConnectionCheck > 60000) { // Check every minute
        if (!sendATCommand("AT", "OK", 2000)) {
          Serial.println("📱 GSM connection lost, reinitializing...");
          currentGSMStatus = GSM_INITIALIZING;
          gsmInitStartTime = millis();
        }
        lastConnectionCheck = millis();
      }
      break;
      
    case GSM_ERROR:
      // Try to recover every 30 seconds
      static unsigned long lastErrorRecovery = 0;
      if (millis() - lastErrorRecovery > 30000) {
        Serial.println("📱 Attempting GSM error recovery...");
        initializeGSM();
        lastErrorRecovery = millis();
      }
      break;
  }
  
  // Process SMS queue instead of single pending SMS
  processSMSQueue();
}

void resetGSMModule() {
  Serial.println("📱 Resetting GSM module...");
  digitalWrite(GSM_RESET_PIN, LOW);
  delay(100);
  digitalWrite(GSM_RESET_PIN, HIGH);
  delay(3000);
  
  currentGSMStatus = GSM_INITIALIZING;
  gsmInitStartTime = millis();
  gsmInitialized = false;
}

void sendSMSAlert(SMSAlertType alertType) {
  sendSMSAlert(alertType, "");
}

void sendSMSAlert(SMSAlertType alertType, const char* additionalInfo) {
  if (!gsmInitialized) {
    Serial.println("📱 SMS Alert skipped - GSM not ready");
    return;
  }
  
  String message = "";
  String timeStr = String(millis() / 1000); // Simple timestamp in seconds
  
  switch (alertType) {
    case SMS_AUTO_FEED:
      message = "🤖 Smart Pet Feeder: Auto-fed ";
      message += additionalInfo;
      message += " - Bowl was empty. Time: ";
      message += timeStr;
      message += "s";
      break;
      
    case SMS_MANUAL_FEED:
      message = "👤 Smart Pet Feeder: Manual feed ";
      message += additionalInfo;
      message += " by button press. Time: ";
      message += timeStr;
      message += "s";
      break;
      
    case SMS_SYSTEM_STATUS:
      message = "📊 Smart Pet Feeder Status: ";
      message += additionalInfo;
      break;
      
    case SMS_FEEDING_ERROR:
      message = "⚠️ Smart Pet Feeder ERROR: ";
      message += additionalInfo;
      message += " Time: ";
      message += timeStr;
      message += "s";
      break;
      
    case SMS_BOWL_EMPTY_ALERT:
      message = "🍽️ Smart Pet Feeder: ";
      message += additionalInfo;
      break;
      
    case SMS_DAILY_RESET:
      message = "🌅 Smart Pet Feeder: New day started. Feed counter reset. Auto feeding enabled.";
      break;
  }
  
  // Get priority and send with priority system
  SMSPriority priority = getSMSPriority(alertType);
  queueSMS(TEST_PHONE_NUMBER, message.c_str(), priority);
}

void sendCustomSMSInternal(const char* phoneNumber, const char* message) {
  if (currentGSMStatus != GSM_SMS_READY) {
    // Queue with medium priority by default
    queueSMS(phoneNumber, message, SMS_PRIORITY_MEDIUM);
    return;
  }
  
  if (smsInProgress) {
    Serial.println("📱 SMS already in progress, queueing message");
    queueSMS(phoneNumber, message, SMS_PRIORITY_MEDIUM);
    return;
  }
  
  Serial.printf("📱 Sending SMS to %s: %s\n", phoneNumber, message);
  
  smsInProgress = true;
  lastSMSSendTime = millis();
  
  // Set SMS to text mode
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  
  // Set recipient
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(phoneNumber);
  gsmSerial.println("\"");
  delay(1000);
  
  // Send message
  gsmSerial.print(message);
  delay(500);
  
  // Send Ctrl+Z to indicate end of message
  gsmSerial.write(26);
  delay(5000); // Wait for SMS to be sent
  
  smsInProgress = false;
  Serial.println("📱 SMS send command completed");
}

bool sendATCommand(const char* command, const char* expectedResponse, unsigned long timeout) {
  String response = "";
  unsigned long startTime = millis();
  
  // Clear input buffer
  while(gsmSerial.available()) {
    gsmSerial.read();
  }
  
  // Send command
  gsmSerial.println(command);
  
  // Wait for response
  while (millis() - startTime < timeout) {
    if (gsmSerial.available()) {
      char c = gsmSerial.read();
      response += c;
      
      // Check if we got the expected response
      if (response.indexOf(expectedResponse) != -1) {
        return true;
      }
      
      // Check for error responses
      if (response.indexOf("ERROR") != -1) {
        return false;
      }
    }
  }
  
  return false;
}

void printGSMStatus() {
  const char* statusNames[] = {
    "OFFLINE", "INITIALIZING", "NETWORK_SEARCHING", 
    "NETWORK_CONNECTED", "SMS_READY", "ERROR"
  };
  
  Serial.printf("📱 GSM Status: %s", statusNames[currentGSMStatus]);
  if (queueCount > 0) {
    Serial.printf(" | Queue: %d SMS", queueCount);
    
    // Show priority breakdown
    int highCount = 0, mediumCount = 0, lowCount = 0;
    for (int i = 0; i < queueCount; i++) {
      int index = (queueHead + i) % MAX_SMS_QUEUE;
      switch (smsQueue[index].priority) {
        case SMS_PRIORITY_HIGH: highCount++; break;
        case SMS_PRIORITY_MEDIUM: mediumCount++; break;
        case SMS_PRIORITY_LOW: lowCount++; break;
      }
    }
    
    if (highCount > 0) Serial.printf(" (H:%d", highCount);
    if (mediumCount > 0) Serial.printf("%sM:%d", highCount > 0 ? ", " : " (", mediumCount);
    if (lowCount > 0) Serial.printf("%sL:%d", (highCount > 0 || mediumCount > 0) ? ", " : " (", lowCount);
    if (highCount > 0 || mediumCount > 0 || lowCount > 0) Serial.print(")");
  }
  if (smsInProgress) {
    Serial.print(" | Sending SMS...");
  }
  Serial.println();
}

void testGSMModule() {
  Serial.println("📱 Testing GSM module...");
  
  if (!isGSMReady()) {
    Serial.println("📱 GSM not ready for testing");
    return;
  }
  
  // Send test SMS
  String testMessage = "🧪 Smart Pet Feeder TEST: GSM module working. Time: ";
  testMessage += String(millis() / 1000);
  testMessage += "s";
  
  sendCustomSMS(TEST_PHONE_NUMBER, testMessage.c_str(), SMS_PRIORITY_LOW);
}

bool checkNetworkConnection() {
  return sendATCommand("AT+CREG?", "+CREG: 0,1", 3000) || 
         sendATCommand("AT+CREG?", "+CREG: 0,5", 3000);
}

void processGSMResponse() {
  // Process any incoming GSM responses (for future enhancements)
  while(gsmSerial.available()) {
    String response = gsmSerial.readString();
    // For now, just clear the buffer
    // Future: Parse incoming SMS, delivery reports, etc.
  }
}

String formatPhoneNumber(const char* number) {
  String formatted = String(number);
  
  // Ensure Philippines format starts with +63
  if (formatted.startsWith("09")) {
    formatted = "+63" + formatted.substring(1);
  } else if (formatted.startsWith("9") && formatted.length() == 10) {
    formatted = "+63" + formatted;
  }
  
  return formatted;
}

// ===========================================
// PRIORITY-BASED SMS SYSTEM FUNCTIONS
// ===========================================

SMSPriority getSMSPriority(SMSAlertType alertType) {
  switch (alertType) {
    case SMS_AUTO_FEED:
    case SMS_MANUAL_FEED:
    case SMS_FEEDING_ERROR:
      return SMS_PRIORITY_HIGH;
      
    case SMS_DAILY_RESET:
    case SMS_BOWL_EMPTY_ALERT:
      return SMS_PRIORITY_MEDIUM;
      
    case SMS_SYSTEM_STATUS:
    default:
      return SMS_PRIORITY_LOW;
  }
}

void queueSMS(const char* phoneNumber, const char* message, SMSPriority priority) {
  // Check if queue is full
  if (queueCount >= MAX_SMS_QUEUE) {
    // Queue full - check if we can replace a lower priority item
    int lowestPriorityIndex = -1;
    SMSPriority lowestPriority = SMS_PRIORITY_HIGH;
    
    for (int i = 0; i < MAX_SMS_QUEUE; i++) {
      int index = (queueHead + i) % MAX_SMS_QUEUE;
      if (smsQueue[index].priority > lowestPriority) {
        lowestPriority = smsQueue[index].priority;
        lowestPriorityIndex = index;
      }
    }
    
    // If new message has higher priority, replace lower priority message
    if (priority < lowestPriority) {
      Serial.printf("📱 SMS Queue: Replacing low priority message with priority %d\n", priority);
      smsQueue[lowestPriorityIndex].phoneNumber = String(phoneNumber);
      smsQueue[lowestPriorityIndex].message = String(message);
      smsQueue[lowestPriorityIndex].priority = priority;
      smsQueue[lowestPriorityIndex].queueTime = millis();
      return;
    } else {
      Serial.println("📱 SMS Queue: Full, message dropped (lower priority)");
      return;
    }
  }
  
  // Add to queue
  smsQueue[queueTail].phoneNumber = String(phoneNumber);
  smsQueue[queueTail].message = String(message);
  smsQueue[queueTail].priority = priority;
  smsQueue[queueTail].queueTime = millis();
  
  queueTail = (queueTail + 1) % MAX_SMS_QUEUE;
  queueCount++;
  
  Serial.printf("📱 SMS Queued (Priority %d): %s\n", priority, message);
}

bool processSMSQueue() {
  if (queueCount == 0 || smsInProgress || currentGSMStatus != GSM_SMS_READY) {
    return false;
  }
  
  // Find highest priority message in queue
  int highestPriorityIndex = -1;
  SMSPriority highestPriority = static_cast<SMSPriority>(SMS_PRIORITY_LOW + 1); // Lower number = higher priority
  
  for (int i = 0; i < queueCount; i++) {
    int index = (queueHead + i) % MAX_SMS_QUEUE;
    if (smsQueue[index].priority < highestPriority) {
      highestPriority = smsQueue[index].priority;
      highestPriorityIndex = index;
    }
  }
  
  if (highestPriorityIndex == -1) return false;
  
  // Check priority-based rate limiting
  unsigned long currentTime = millis();
  unsigned long minInterval = 30000; // Default 30 seconds
  unsigned long *lastSendTime = &lastSMSSendTime;
  
  switch (highestPriority) {
    case SMS_PRIORITY_HIGH:
      minInterval = 10000; // High priority: 10 seconds minimum
      lastSendTime = &lastHighPrioritySMS;
      break;
    case SMS_PRIORITY_MEDIUM:
      minInterval = 30000; // Medium priority: 30 seconds minimum
      lastSendTime = &lastMediumPrioritySMS;
      break;
    case SMS_PRIORITY_LOW:
      minInterval = 120000; // Low priority: 2 minutes minimum
      lastSendTime = &lastLowPrioritySMS;
      break;
  }
  
  if (currentTime - *lastSendTime < minInterval) {
    return false; // Rate limited
  }
  
  // Send the SMS
  SMSQueueItem item = smsQueue[highestPriorityIndex];
  Serial.printf("📱 Sending queued SMS (Priority %d): %s\n", item.priority, item.message.c_str());
  
  sendCustomSMSInternal(item.phoneNumber.c_str(), item.message.c_str());
  
  // Remove from queue by shifting
  for (int i = highestPriorityIndex; i != queueTail; i = (i + 1) % MAX_SMS_QUEUE) {
    int nextIndex = (i + 1) % MAX_SMS_QUEUE;
    smsQueue[i] = smsQueue[nextIndex];
  }
  queueCount--;
  queueTail = (queueTail - 1 + MAX_SMS_QUEUE) % MAX_SMS_QUEUE;
  
  *lastSendTime = currentTime;
  return true;
}

void sendCustomSMS(const char* phoneNumber, const char* message, SMSPriority priority) {
  // Queue SMS with specified priority
  queueSMS(phoneNumber, message, priority);
}

void sendCustomSMS(const char* phoneNumber, const char* message) {
  // Default to medium priority for backward compatibility
  sendCustomSMS(phoneNumber, message, SMS_PRIORITY_MEDIUM);
}

void sendHighPrioritySMS(const char* phoneNumber, const char* message) {
  queueSMS(phoneNumber, message, SMS_PRIORITY_HIGH);
}