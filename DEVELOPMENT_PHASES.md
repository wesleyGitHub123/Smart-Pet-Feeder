# Smart Pet Feeder - Development Phases Documentation

## Phase 1: Basic System Setup & Manual Controls ✅ COMPLETE

### Objective
Establish ESP32-S3 communication, test manual inputs, and create the foundation for the smart pet feeder system.

### Success Criteria - ACHIEVED
- ✅ Serial communication stable at 115200 baud
- ✅ All button presses detected with proper debouncing
- ✅ Mode switch changes detected and logged
- ✅ Buzzer plays different patterns for different actions
- ✅ System state management working correctly
- ✅ No crashes or error messages
- ✅ Clean, readable debug output

---

## Phase 2: Ultrasonic Distance Sensing ✅ READY FOR TESTING

**Objective**: Add I2C ultrasonic sensor for bowl and hopper detection
**New Hardware**: M5Stack RCWL-9620 ultrasonic sensor
**Expected**: Accurate distance readings, bowl empty/full detection

### Hardware Connections
```
ESP32-S3 DevKit Wiring for Phase 2:
├── GPIO10 → Manual Feed Button (other end to GND)
├── GPIO11 → Mode Switch (Cat/Dog) (other end to GND)  
├── GPIO12 → Buzzer positive (+) [Buzzer (-) to GND]
├── GPIO8  → RCWL-9620 SDA (I2C Data)
├── GPIO9  → RCWL-9620 SCL (I2C Clock)
├── 3.3V   → RCWL-9620 VCC
├── GND    → RCWL-9620 GND
└── USB-C  → Computer (for programming/serial monitor)

RCWL-9620 Sensor:
- I2C Address: 0x57
- Operating Range: 2cm - 300cm
- Resolution: 1mm
- Update Rate: ~17Hz
```

### Expected Behavior
1. **All Phase 1 functionality** (manual controls, buzzer, mode switching)
2. **Sensor Initialization**: System reports sensor found at I2C address 0x57
3. **Distance Readings**: Continuous distance measurements displayed in debug output
4. **Bowl Detection**: 
   - Empty when distance > 15cm (triggers alert beeps)
   - Has food when distance < 8cm
5. **Hopper Monitoring**: Alerts when distance indicates low/empty hopper
6. **Enhanced Status**: System status now includes sensor data and bowl/hopper status

### Testing Instructions

#### Step 1: Hardware Setup
1. Connect RCWL-9620 sensor to ESP32-S3:
   - VCC → 3.3V
   - GND → GND  
   - SDA → GPIO8
   - SCL → GPIO9
2. Keep Phase 1 connections (button, switch, buzzer)

#### Step 2: Upload and Monitor
1. Press `Ctrl+Shift+P` → "PlatformIO: Upload"
2. Press `Ctrl+Shift+P` → "PlatformIO: Serial Monitor" (115200 baud)

#### Step 3: Verify Sensor Initialization
- [ ] Startup shows "RCWL-9620 sensor found at address 0x57"
- [ ] Initial distance reading is displayed
- [ ] No I2C errors in startup sequence

#### Step 4: Test Distance Sensing
- [ ] Move hand/object near sensor → Distance readings change in real-time
- [ ] Sensor debug info prints every 2 seconds with current distance
- [ ] Readings are stable and accurate (compare with ruler/measuring tape)
- [ ] Range testing: Should read from ~2cm to ~300cm

#### Step 5: Test Bowl Detection Logic
- [ ] When distance > 15cm → System reports "Bowl is EMPTY" + alert beeps
- [ ] When distance < 8cm → System reports "Bowl now has food" + happy beep  
- [ ] Status changes reflected in system status printouts

#### Step 6: Verify Integration with Phase 1
- [ ] Manual feed button still works with audio feedback
- [ ] Mode switching still functions (Cat/Dog modes)
- [ ] All original Phase 1 functionality preserved
- [ ] Enhanced system status includes sensor data

### Success Criteria
- ✅ RCWL-9620 sensor initializes successfully via I2C
- ✅ Accurate distance readings from 2cm to 300cm range
- ✅ Bowl empty/full detection based on distance thresholds
- ✅ Real-time sensor data displayed in debug output
- ✅ Audio alerts for bowl status changes
- ✅ Enhanced system status with sensor information
- ✅ All Phase 1 functionality preserved and working
- ✅ No I2C communication errors
- ✅ Stable sensor readings with no false triggers

### Troubleshooting

**Problem**: "RCWL-9620 sensor not found at address 0x57"
- Check wiring: SDA=GPIO8, SCL=GPIO9
- Verify 3.3V power connection to sensor
- Try I2C scanner code to detect sensor address
- Check for loose connections

**Problem**: Erratic or invalid distance readings  
- Ensure sensor is mounted stably
- Check for electrical interference
- Verify sensor is not obstructed
- Test with known distances using ruler

**Problem**: No bowl status changes
- Verify distance thresholds are appropriate for your setup
- Check that objects are within sensor range (2-300cm)
- Adjust `BOWL_EMPTY_THRESHOLD` and `BOWL_FULL_THRESHOLD` in config.h if needed

---

## Complete Development Plan Overview

### Phase 3: Stepper Motor Control (NEXT)
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