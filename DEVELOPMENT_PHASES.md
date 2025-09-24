# Smart Pet Feeder - Development Phases Documentation

## Phase 1: Basic System Setup & Manual Controls ✅ READY FOR TESTING

### Objective
Establish ESP32-S3 communication, test manual inputs, and create the foundation for the smart pet feeder system.

### Hardware Connections
```
ESP32-S3 DevKit Wiring:
├── GPIO10 → Manual Feed Button (other end to GND)
├── GPIO11 → Mode Switch (Cat/Dog) (other end to GND)
├── GPIO12 → Buzzer positive (+) [Buzzer (-) to GND]
└── USB-C → Computer (for programming/serial monitor)

Notes:
- Use ESP32's internal pullup resistors (configured in code)
- Buttons are active LOW (pressed = LOW, released = HIGH)
- Mode switch: LOW = Dog Mode, HIGH = Cat Mode
```

### Expected Behavior
1. **Startup**: Serial monitor shows welcome message and startup beep sequence
2. **Manual Feed Button**: Press to trigger simulated feeding with audio feedback
3. **Mode Switch**: Toggle between Cat mode (high beep) and Dog mode (low beeps)
4. **Status Updates**: System prints status every 5 seconds
5. **Debug Info**: All actions logged to serial monitor

### Testing Instructions

#### Step 1: Upload and Monitor
1. In VS Code, press `Ctrl+Shift+P`
2. Type "PlatformIO: Upload" and select it
3. After upload, press `Ctrl+Shift+P` again
4. Type "PlatformIO: Serial Monitor" and select it
5. Set baud rate to 115200

#### Step 2: Verify Basic Operation
- [ ] Startup sequence plays (ascending beeps)
- [ ] Welcome message appears in serial monitor
- [ ] System status prints every 5 seconds
- [ ] No error messages or exceptions

#### Step 3: Test Manual Controls
- [ ] Press feed button → Should see "MANUAL FEED BUTTON PRESSED!" + short beep
- [ ] Toggle mode switch → Should see mode change message + different beep patterns
  - Cat mode: Two short high-pitched beeps
  - Dog mode: Three medium-pitched beeps

#### Step 4: Verify State Management
- [ ] System state changes from IDLE → MANUAL_FEEDING → IDLE
- [ ] Mode correctly shows CAT or DOG based on switch position
- [ ] No button bouncing or multiple triggers from single press

### Success Criteria
- ✅ Serial communication stable at 115200 baud
- ✅ All button presses detected with proper debouncing
- ✅ Mode switch changes detected and logged
- ✅ Buzzer plays different patterns for different actions
- ✅ System state management working correctly
- ✅ No crashes or error messages
- ✅ Clean, readable debug output

### Troubleshooting
**Problem**: No serial output
- Check USB cable and ESP32 connection
- Verify correct COM port in PlatformIO monitor
- Try pressing the BOOT button while uploading

**Problem**: Button not responding
- Check wiring: Button between GPIO pin and GND
- Verify internal pullups are working (pins should read HIGH when not pressed)

**Problem**: Multiple triggers from single button press
- This indicates switch bouncing - should be handled by debounce code
- Try different button or check for loose connections

---

## Complete Development Plan Overview

### Phase 2: Ultrasonic Distance Sensing (NEXT)
**Objective**: Add I2C ultrasonic sensor for bowl and hopper detection
**New Hardware**: M5Stack RCWL-9620 ultrasonic sensor
**Expected**: Accurate distance readings, bowl empty/full detection

### Phase 3: Stepper Motor Control  
**Objective**: Control food dispensing with precise motor movements
**New Hardware**: NEMA 17 stepper + DRV8825 driver
**Expected**: Precise portion control, calibrated dispensing

### Phase 4: Integrated Feeding Logic
**Objective**: Combine bowl detection with automatic feeding
**New Hardware**: None (integration phase)
**Expected**: Automatic feeding when bowl is empty, prevent overfeeding

### Phase 5: Alert System
**Objective**: Local and remote notifications
**New Hardware**: SIM800L GSM module
**Expected**: Buzzer alerts, SMS notifications for empty hopper

### Phase 6: System Polish & Reliability
**Objective**: Error handling, configuration, robust operation
**New Hardware**: None (software enhancement)
**Expected**: Production-ready system with error recovery

---

## Next Steps
1. **Complete Phase 1 testing** using the instructions above
2. **Report results** - What works? Any issues?
3. **Move to Phase 2** - Add ultrasonic sensor for distance detection

Each phase builds upon the previous, ensuring we always have a working system to fall back to.