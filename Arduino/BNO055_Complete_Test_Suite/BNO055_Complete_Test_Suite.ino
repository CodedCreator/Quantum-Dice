/*
  BNO055_Complete_Test_Suite.ino
  
  Comprehensive test and demonstration of ALL functions in the BNO055_Motion library.
  
  This sketch demonstrates:
  - Initialization with verbose output
  - Motion and stability detection
  - Orientation detection (all 8 states)
  - Gyroscope readings (rad/s and deg/s)
  - Accelerometer readings
  - Calibration monitoring
  - Tumble detection with adjustable thresholds
  - Threshold customization
  - Axis remapping queries
  
  Interactive Commands via Serial Monitor:
  - '1' = Display current sensor readings
  - '2' = Test motion detection
  - '3' = Test orientation detection
  - '4' = Test gyroscope readings
  - '5' = Test tumble detection (reset reference)
  - '6' = Change tumble threshold
  - '7' = Change motion sensitivity
  - '8' = Display calibration status
  - '9' = Display axis remap configuration
  - '0' = Run full automated test
  - 'h' = Show help menu
  
  Hardware:
  - BNO055 IMU sensor connected via I2C
  
  Required Libraries:
  - Adafruit BNO055
  - Adafruit Unified Sensor
*/

#include "BNO055_Motion.h"

// Create sensor object
BNO055_Motion sensor;

// Test state tracking
bool referenceSet = false;
unsigned long testStartTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Wait for Serial to be ready
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 3000)) delay(10);
  
  printHeader();
  
  Serial.println("INITIALIZATION TEST");
  Serial.println("═══════════════════════════════════════════════");
  Serial.println();
  
  // Initialize with verbose output
  bool success = sensor.init(true);
  
  Serial.println();
  
  if (!success) {
    Serial.println("❌ INITIALIZATION FAILED!");
    Serial.println("Cannot continue. Check connections and reset.");
    while (1) {
      delay(1000);
      Serial.println("Waiting for reset...");
    }
  }
  
  Serial.println("✅ INITIALIZATION SUCCESSFUL!");
  Serial.println();
  
  // Initial sensor readings
  sensor.update();
  
  Serial.println("Initial Readings:");
  Serial.println("-----------------");
  displayAllReadings();
  
  Serial.println();
  printHelp();
}

void loop() {
  // Update sensor continuously
  sensor.update();
  
  // Check for serial commands
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // Clear any remaining characters
    while (Serial.available()) Serial.read();
    
    handleCommand(cmd);
  }
  
  delay(50);
}

// ============================================
// COMMAND HANDLERS
// ============================================

void handleCommand(char cmd) {
  Serial.println();
  Serial.println("═══════════════════════════════════════════════");
  
  switch (cmd) {
    case '1':
      testCurrentReadings();
      break;
    case '2':
      testMotionDetection();
      break;
    case '3':
      testOrientationDetection();
      break;
    case '4':
      testGyroscope();
      break;
    case '5':
      testTumbleDetection();
      break;
    case '6':
      changeTumbleThreshold();
      break;
    case '7':
      changeMotionSensitivity();
      break;
    case '8':
      testCalibration();
      break;
    case '9':
      testAxisRemap();
      break;
    case '0':
      runFullTest();
      break;
    case 'h':
    case 'H':
      printHelp();
      break;
    default:
      Serial.println("Unknown command. Press 'h' for help.");
      break;
  }
  
  Serial.println("═══════════════════════════════════════════════");
  Serial.println();
}

// ============================================
// TEST FUNCTIONS
// ============================================

void testCurrentReadings() {
  Serial.println("TEST 1: Current Sensor Readings");
  Serial.println("--------------------------------");
  Serial.println();
  
  displayAllReadings();
  
  Serial.println();
  Serial.println("✓ Test complete");
}

void testMotionDetection() {
  Serial.println("TEST 2: Motion Detection");
  Serial.println("------------------------");
  Serial.println();
  Serial.println("Testing moving() and stable() functions...");
  Serial.println("Keep sensor still, then move it around.");
  Serial.println("Press any key to stop test.");
  Serial.println();
  
  while (!Serial.available()) {
    sensor.update();
    
    Serial.print("State: ");
    if (sensor.moving()) {
      Serial.print("🔴 MOVING     ");
    } else if (sensor.stable()) {
      Serial.print("🟢 STABLE     ");
    } else {
      Serial.print("🟡 SETTLING   ");
    }
    
    Serial.print(" | Accel Δ: ");
    Serial.print(sensor.getAccelChange(), 3);
    Serial.print(" m/s² | Mag: ");
    Serial.print(sensor.getAccelMagnitude(), 2);
    Serial.println(" m/s²");
    
    delay(100);
  }
  
  // Clear input
  while (Serial.available()) Serial.read();
  
  Serial.println();
  Serial.println("✓ Motion detection test complete");
}

void testOrientationDetection() {
  Serial.println("TEST 3: Orientation Detection");
  Serial.println("-----------------------------");
  Serial.println();
  Serial.println("Testing orientation(), on_table(), and getOrientationString()...");
  Serial.println("Rotate sensor to different orientations.");
  Serial.println("Press any key to stop test.");
  Serial.println();
  
  BNO055_Orientation lastOrientation = ORIENTATION_UNKNOWN;
  
  while (!Serial.available()) {
    sensor.update();
    
    BNO055_Orientation currentOrientation = sensor.orientation();
    
    // Print when orientation changes
    if (currentOrientation != lastOrientation) {
      Serial.println("─────────────────────────────────────────");
      Serial.print("Orientation changed to: ");
      Serial.println(sensor.getOrientationString());
      Serial.print("Enum value: ");
      Serial.println(currentOrientation);
      Serial.print("On table: ");
      Serial.println(sensor.on_table() ? "YES" : "NO");
      Serial.println();
      lastOrientation = currentOrientation;
    }
    
    // Show current orientation periodically
    Serial.print("Current: ");
    Serial.print(sensor.getOrientationString());
    Serial.print(" | On table: ");
    Serial.print(sensor.on_table() ? "Y" : "N");
    Serial.print(" | Accel: [");
    Serial.print(sensor.accelX(), 1);
    Serial.print(", ");
    Serial.print(sensor.accelY(), 1);
    Serial.print(", ");
    Serial.print(sensor.accelZ(), 1);
    Serial.println("]");
    
    delay(100);
  }
  
  // Clear input
  while (Serial.available()) Serial.read();
  
  Serial.println();
  Serial.println("✓ Orientation detection test complete");
}

void testGyroscope() {
  Serial.println("TEST 4: Gyroscope Readings");
  Serial.println("---------------------------");
  Serial.println();
  Serial.println("Testing gyroX(), gyroY(), gyroZ()...");
  Serial.println("Rotate sensor around different axes.");
  Serial.println("Press any key to stop test.");
  Serial.println();
  
  while (!Serial.available()) {
    sensor.update();
    
    float gx = sensor.gyroX();
    float gy = sensor.gyroY();
    float gz = sensor.gyroZ();
    
    Serial.print("Gyro (rad/s): [");
    Serial.print(gx, 4);
    Serial.print(", ");
    Serial.print(gy, 4);
    Serial.print(", ");
    Serial.print(gz, 4);
    Serial.print("]  |  (°/s): [");
    Serial.print(gx * 57.2958, 2);
    Serial.print(", ");
    Serial.print(gy * 57.2958, 2);
    Serial.print(", ");
    Serial.print(gz * 57.2958, 2);
    Serial.println("]");
    
    delay(100);
  }
  
  // Clear input
  while (Serial.available()) Serial.read();
  
  Serial.println();
  Serial.println("✓ Gyroscope test complete");
}

void testTumbleDetection() {
  Serial.println("TEST 5: Tumble Detection");
  Serial.println("------------------------");
  Serial.println();
  Serial.println("Testing resetTumbleDetection(), tumbled(), getTumbleAngle()...");
  Serial.println();
  Serial.println("Setting reference orientation NOW...");
  
  sensor.resetTumbleDetection();
  referenceSet = true;
  
  Serial.print("Reference captured: [");
  Serial.print(sensor.accelX(), 2);
  Serial.print(", ");
  Serial.print(sensor.accelY(), 2);
  Serial.print(", ");
  Serial.print(sensor.accelZ(), 2);
  Serial.println("] m/s²");
  Serial.println();
  Serial.println("Now rotate sensor to test tumble detection.");
  Serial.println("Press any key to stop test.");
  Serial.println();
  
  bool wasTumbled = false;
  
  while (!Serial.available()) {
    sensor.update();
    
    bool isTumbled = sensor.tumbled();
    float angle = sensor.getTumbleAngle();
    
    if (isTumbled && !wasTumbled) {
      Serial.println();
      Serial.println("⚠️⚠️⚠️ TUMBLE DETECTED! ⚠️⚠️⚠️");
      Serial.println();
    }
    
    Serial.print(isTumbled ? "⚠️ TUMBLED  " : "✓ OK       ");
    Serial.print(" | Angle: ");
    Serial.print(angle, 1);
    Serial.print("° | ");
    printAngleBar(angle, 18);
    Serial.println();
    
    wasTumbled = isTumbled;
    delay(100);
  }
  
  // Clear input
  while (Serial.available()) Serial.read();
  
  Serial.println();
  Serial.println("✓ Tumble detection test complete");
}

void changeTumbleThreshold() {
  Serial.println("TEST 6: Change Tumble Threshold");
  Serial.println("--------------------------------");
  Serial.println();
  Serial.println("Testing setTumbleThreshold()...");
  Serial.println();
  Serial.println("Select threshold:");
  Serial.println("  1 = 15° (very sensitive)");
  Serial.println("  2 = 30° (sensitive)");
  Serial.println("  3 = 45° (default)");
  Serial.println("  4 = 60° (moderate)");
  Serial.println("  5 = 90° (loose)");
  Serial.println();
  Serial.print("Enter choice (1-5): ");
  
  while (!Serial.available()) delay(10);
  char choice = Serial.read();
  Serial.println(choice);
  
  // Clear remaining
  while (Serial.available()) Serial.read();
  
  float threshold;
  float angleDeg;
  
  switch (choice) {
    case '1': threshold = 0.966; angleDeg = 15; break;
    case '2': threshold = 0.866; angleDeg = 30; break;
    case '3': threshold = 0.707; angleDeg = 45; break;
    case '4': threshold = 0.500; angleDeg = 60; break;
    case '5': threshold = 0.000; angleDeg = 90; break;
    default:
      Serial.println("Invalid choice. Using default (45°).");
      threshold = 0.707;
      angleDeg = 45;
      break;
  }
  
  sensor.setTumbleThreshold(threshold);
  
  Serial.print("✓ Tumble threshold set to ");
  Serial.print(angleDeg, 0);
  Serial.print("° (");
  Serial.print(threshold, 3);
  Serial.println(")");
  Serial.println();
  Serial.println("Reset tumble reference to apply (press '5')");
}

void changeMotionSensitivity() {
  Serial.println("TEST 7: Change Motion Sensitivity");
  Serial.println("----------------------------------");
  Serial.println();
  Serial.println("Testing setMotionThreshold(), setStableThreshold(), setStableCount()...");
  Serial.println();
  Serial.println("Select preset:");
  Serial.println("  1 = Very Sensitive (small movements detected)");
  Serial.println("  2 = Sensitive");
  Serial.println("  3 = Normal (default)");
  Serial.println("  4 = Less Sensitive");
  Serial.println("  5 = Very Insensitive (large movements only)");
  Serial.println();
  Serial.print("Enter choice (1-5): ");
  
  while (!Serial.available()) delay(10);
  char choice = Serial.read();
  Serial.println(choice);
  
  // Clear remaining
  while (Serial.available()) Serial.read();
  
  switch (choice) {
    case '1': // Very sensitive
      sensor.setMotionThreshold(0.2);
      sensor.setStableThreshold(0.05);
      sensor.setStableCount(10);
      Serial.println("✓ Very Sensitive: 0.2 m/s² motion, 0.05 m/s² stable, 10 samples");
      break;
    case '2': // Sensitive
      sensor.setMotionThreshold(0.3);
      sensor.setStableThreshold(0.1);
      sensor.setStableCount(7);
      Serial.println("✓ Sensitive: 0.3 m/s² motion, 0.1 m/s² stable, 7 samples");
      break;
    case '3': // Normal (default)
      sensor.setMotionThreshold(0.5);
      sensor.setStableThreshold(0.15);
      sensor.setStableCount(5);
      Serial.println("✓ Normal: 0.5 m/s² motion, 0.15 m/s² stable, 5 samples");
      break;
    case '4': // Less sensitive
      sensor.setMotionThreshold(0.8);
      sensor.setStableThreshold(0.25);
      sensor.setStableCount(3);
      Serial.println("✓ Less Sensitive: 0.8 m/s² motion, 0.25 m/s² stable, 3 samples");
      break;
    case '5': // Very insensitive
      sensor.setMotionThreshold(1.5);
      sensor.setStableThreshold(0.5);
      sensor.setStableCount(2);
      Serial.println("✓ Very Insensitive: 1.5 m/s² motion, 0.5 m/s² stable, 2 samples");
      break;
    default:
      Serial.println("Invalid choice. No changes made.");
      break;
  }
  
  Serial.println();
  Serial.println("Test motion detection to see effects (press '2')");
}

void testCalibration() {
  Serial.println("TEST 8: Calibration Status");
  Serial.println("---------------------------");
  Serial.println();
  Serial.println("Testing getCalibration() and isCalibrated()...");
  Serial.println();
  
  uint8_t sys, gyro, accel, mag;
  sensor.getCalibration(&sys, &gyro, &accel, &mag);
  
  Serial.println("Calibration Values (0-3, 3 = fully calibrated):");
  Serial.print("  System:        ");
  Serial.print(sys);
  Serial.print("/3  ");
  printCalibrationBar(sys);
  Serial.println();
  
  Serial.print("  Gyroscope:     ");
  Serial.print(gyro);
  Serial.print("/3  ");
  printCalibrationBar(gyro);
  Serial.println();
  
  Serial.print("  Accelerometer: ");
  Serial.print(accel);
  Serial.print("/3  ");
  printCalibrationBar(accel);
  Serial.println();
  
  Serial.print("  Magnetometer:  ");
  Serial.print(mag);
  Serial.print("/3  ");
  printCalibrationBar(mag);
  Serial.println();
  
  Serial.println();
  Serial.print("isCalibrated(): ");
  if (sensor.isCalibrated()) {
    Serial.println("✓ YES - All sensors calibrated");
  } else {
    Serial.println("⚠️ NO - Calibration needed");
    Serial.println();
    Serial.println("To calibrate:");
    Serial.println("  1. Move sensor slowly in figure-8 pattern");
    Serial.println("  2. Tilt to all orientations");
    Serial.println("  3. Repeat until all values reach 3");
  }
  
  Serial.println();
  Serial.println("✓ Calibration test complete");
}

void testAxisRemap() {
  Serial.println("TEST 9: Axis Remap Configuration");
  Serial.println("---------------------------------");
  Serial.println();
  Serial.println("Testing getAxisRemap()...");
  Serial.println();
  
  uint8_t config, sign;
  sensor.getAxisRemap(&config, &sign);
  
  Serial.print("AXIS_MAP_CONFIG: 0x");
  Serial.print(config, HEX);
  Serial.print(" (");
  Serial.print(config, BIN);
  Serial.println(")");
  
  Serial.print("AXIS_MAP_SIGN:   0x");
  Serial.print(sign, HEX);
  Serial.print(" (");
  Serial.print(sign, BIN);
  Serial.println(")");
  
  Serial.println();
  Serial.println("Default configuration:");
  Serial.println("  Physical X → Software Z");
  Serial.println("  Physical Y → Software Y");
  Serial.println("  Physical Z → Software X");
  
  Serial.println();
  Serial.println("✓ Axis remap test complete");
}

void runFullTest() {
  Serial.println("TEST 0: Full Automated Test Suite");
  Serial.println("==================================");
  Serial.println();
  Serial.println("Running all tests automatically...");
  Serial.println();
  
  delay(1000);
  
  // Test 1
  Serial.println(">>> Running Test 1: Current Readings");
  displayAllReadings();
  Serial.println("✓ Test 1 complete");
  Serial.println();
  delay(2000);
  
  // Test 2
  Serial.println(">>> Running Test 2: Motion Detection (5 sec)");
  testStartTime = millis();
  while (millis() - testStartTime < 5000) {
    sensor.update();
    Serial.print(sensor.moving() ? "MOVING " : "STABLE ");
    delay(500);
  }
  Serial.println();
  Serial.println("✓ Test 2 complete");
  Serial.println();
  delay(2000);
  
  // Test 3
  Serial.println(">>> Running Test 3: Orientation");
  Serial.print("Current: ");
  Serial.println(sensor.getOrientationString());
  Serial.print("On table: ");
  Serial.println(sensor.on_table() ? "YES" : "NO");
  Serial.println("✓ Test 3 complete");
  Serial.println();
  delay(2000);
  
  // Test 4
  Serial.println(">>> Running Test 4: Gyroscope");
  Serial.print("Gyro (rad/s): [");
  Serial.print(sensor.gyroX(), 4);
  Serial.print(", ");
  Serial.print(sensor.gyroY(), 4);
  Serial.print(", ");
  Serial.print(sensor.gyroZ(), 4);
  Serial.println("]");
  Serial.println("✓ Test 4 complete");
  Serial.println();
  delay(2000);
  
  // Test 5
  Serial.println(">>> Running Test 5: Tumble Detection");
  sensor.resetTumbleDetection();
  Serial.println("Reference set");
  delay(1000);
  Serial.print("Angle from reference: ");
  Serial.print(sensor.getTumbleAngle(), 1);
  Serial.println("°");
  Serial.println("✓ Test 5 complete");
  Serial.println();
  delay(2000);
  
  // Test 6
  Serial.println(">>> Running Test 6: Calibration");
  uint8_t sys, gyro, accel, mag;
  sensor.getCalibration(&sys, &gyro, &accel, &mag);
  Serial.print("Calibration: Sys=");
  Serial.print(sys);
  Serial.print(" Gyro=");
  Serial.print(gyro);
  Serial.print(" Accel=");
  Serial.print(accel);
  Serial.print(" Mag=");
  Serial.println(mag);
  Serial.println("✓ Test 6 complete");
  Serial.println();
  delay(2000);
  
  // Test 7
  Serial.println(">>> Running Test 7: Axis Remap");
  uint8_t config, sign;
  sensor.getAxisRemap(&config, &sign);
  Serial.print("Config: 0x");
  Serial.print(config, HEX);
  Serial.print(", Sign: 0x");
  Serial.println(sign, HEX);
  Serial.println("✓ Test 7 complete");
  Serial.println();
  
  Serial.println("==================================");
  Serial.println("✅ ALL TESTS COMPLETE!");
  Serial.println("==================================");
}

// ============================================
// DISPLAY FUNCTIONS
// ============================================

void displayAllReadings() {
  Serial.println("Motion State:");
  Serial.print("  moving():         ");
  Serial.println(sensor.moving() ? "true" : "false");
  Serial.print("  stable():         ");
  Serial.println(sensor.stable() ? "true" : "false");
  Serial.println();
  
  Serial.println("Orientation:");
  Serial.print("  orientation():    ");
  Serial.println(sensor.orientation());
  Serial.print("  getOrientationString(): ");
  Serial.println(sensor.getOrientationString());
  Serial.print("  on_table():       ");
  Serial.println(sensor.on_table() ? "true" : "false");
  Serial.println();
  
  Serial.println("Accelerometer:");
  Serial.print("  accelX():         ");
  Serial.print(sensor.accelX(), 3);
  Serial.println(" m/s²");
  Serial.print("  accelY():         ");
  Serial.print(sensor.accelY(), 3);
  Serial.println(" m/s²");
  Serial.print("  accelZ():         ");
  Serial.print(sensor.accelZ(), 3);
  Serial.println(" m/s²");
  Serial.print("  getAccelMagnitude(): ");
  Serial.print(sensor.getAccelMagnitude(), 3);
  Serial.println(" m/s²");
  Serial.print("  getAccelChange(): ");
  Serial.print(sensor.getAccelChange(), 3);
  Serial.println(" m/s²");
  Serial.println();
  
  Serial.println("Gyroscope:");
  Serial.print("  gyroX():          ");
  Serial.print(sensor.gyroX(), 4);
  Serial.print(" rad/s (");
  Serial.print(sensor.gyroX() * 57.2958, 2);
  Serial.println(" °/s)");
  Serial.print("  gyroY():          ");
  Serial.print(sensor.gyroY(), 4);
  Serial.print(" rad/s (");
  Serial.print(sensor.gyroY() * 57.2958, 2);
  Serial.println(" °/s)");
  Serial.print("  gyroZ():          ");
  Serial.print(sensor.gyroZ(), 4);
  Serial.print(" rad/s (");
  Serial.print(sensor.gyroZ() * 57.2958, 2);
  Serial.println(" °/s)");
  Serial.println();
  
  if (referenceSet) {
    Serial.println("Tumble Detection:");
    Serial.print("  tumbled():        ");
    Serial.println(sensor.tumbled() ? "true" : "false");
    Serial.print("  getTumbleAngle(): ");
    Serial.print(sensor.getTumbleAngle(), 2);
    Serial.println("°");
    Serial.println();
  }
  
  uint8_t sys, gyro, accel, mag;
  sensor.getCalibration(&sys, &gyro, &accel, &mag);
  Serial.println("Calibration:");
  Serial.print("  getCalibration(): Sys=");
  Serial.print(sys);
  Serial.print(" Gyro=");
  Serial.print(gyro);
  Serial.print(" Accel=");
  Serial.print(accel);
  Serial.print(" Mag=");
  Serial.println(mag);
  Serial.print("  isCalibrated():   ");
  Serial.println(sensor.isCalibrated() ? "true" : "false");
}

void printHeader() {
  Serial.println();
  Serial.println("╔════════════════════════════════════════════════╗");
  Serial.println("║     BNO055 Motion Library - Complete Test     ║");
  Serial.println("║              Suite & Demo Sketch               ║");
  Serial.println("╚════════════════════════════════════════════════╝");
  Serial.println();
}

void printHelp() {
  Serial.println("INTERACTIVE TEST MENU:");
  Serial.println("======================");
  Serial.println();
  Serial.println("Press a key to run a test:");
  Serial.println();
  Serial.println("  1 - Display all current readings");
  Serial.println("  2 - Test motion detection (moving/stable)");
  Serial.println("  3 - Test orientation detection");
  Serial.println("  4 - Test gyroscope readings");
  Serial.println("  5 - Test tumble detection (set reference)");
  Serial.println("  6 - Change tumble threshold");
  Serial.println("  7 - Change motion sensitivity");
  Serial.println("  8 - Display calibration status");
  Serial.println("  9 - Display axis remap config");
  Serial.println("  0 - Run full automated test");
  Serial.println("  h - Show this help menu");
  Serial.println();
}

void printAngleBar(float angle, int width) {
  Serial.print("[");
  int bars = (int)(angle / 180.0 * width);
  if (bars > width) bars = width;
  
  for (int i = 0; i < bars; i++) {
    Serial.print("█");
  }
  for (int i = bars; i < width; i++) {
    Serial.print("░");
  }
  Serial.print("]");
}

void printCalibrationBar(uint8_t value) {
  Serial.print("[");
  for (int i = 0; i < 3; i++) {
    if (i < value) {
      Serial.print("■");
    } else {
      Serial.print("□");
    }
  }
  Serial.print("]");
}
