/*
  ESP32-S3 Unified Sensor Setup Tool with EEPROM Configuration
  Combines MAC Address retrieval and EEPROM configuration management
  
  Hardware Requirements:
  - ESP32-S3 board
  - SparkFun ATECC508a Cryptographic Co-processor (optional)
  - Adafruit BNO055 IMU sensor (optional)
  - I2C/Qwiic connections
  
  Usage:
  1. Upload sketch to ESP32-S3
  2. Open Serial Monitor at 115200 baud
  3. Follow menu prompts to perform initialization tasks


  Release Version: 1.0.0
*/

#include <Wire.h>
#include <WiFi.h>
#include <EEPROM.h>

// Conditional includes - comment out if sensors not present
#include <Adafruit_GC9A01A.h>  // For color constants

// ==================== EEPROM MEMORY LAYOUT ====================
// BNO calibration is not used anymore. EEPROM Address still present, but not used
#define EEPROM_SIZE 512
#define EEPROM_BNO_SENSOR_ID_ADDR 0
#define EEPROM_BNO_CALIBRATION_ADDR 4
#define EEPROM_CONFIG_ADDRESS 32  // Moved to avoid overlap with BNO calibration data
// Memory layout: [0-3: BNO ID][4-31: BNO Calibration][32+: Device Config]

// ==================== DICE CONFIGURATION STRUCTURE ====================
struct DiceConfig {
  char diceId[16];
  uint8_t deviceA_mac[6];
  uint8_t deviceB1_mac[6];
  uint8_t deviceB2_mac[6];
  uint16_t x_background;
  uint16_t y_background;
  uint16_t z_background;
  uint16_t entang_ab1_color;
  uint16_t entang_ab2_color;
  int8_t rssiLimit;
  bool isSMD;
  bool isNano;
  bool alwaysSeven;
  uint8_t randomSwitchPoint;
  float tumbleConstant;
  uint32_t deepSleepTimeout;
};

// Default configuration
const DiceConfig defaultConfig = {
  .diceId = "TEST1",
  .deviceA_mac = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
  .deviceB1_mac = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
  .deviceB2_mac = { 0xDC, 0xDA, 0x0C, 0x21, 0x02, 0x44 },
  .x_background = GC9A01A_BLACK,
  .y_background = GC9A01A_BLACK,
  .z_background = GC9A01A_BLACK,
  .entang_ab1_color = GC9A01A_YELLOW,
  .entang_ab2_color = GC9A01A_GREEN,
  .rssiLimit = -35,
  .isSMD = true,
  .isNano = false,
  .alwaysSeven = false,
  .randomSwitchPoint = 50,
  .tumbleConstant = 0.2,
  .deepSleepTimeout = 300000  // 5 minutes
};


// ==================== FUNCTION PROTOTYPES ====================
void displayMainMenu();
void getMacAddress();
void clearEEPROM();
void configureEEPROMSettings();
void parseMacAddress(String macStr, uint8_t* macList);


// EEPROM Configuration functions
bool validateConfig(const DiceConfig& config);
void printConfig(const DiceConfig& config, const char* title);
bool readEEPROMConfig(DiceConfig& config);
void writeEEPROMConfig(const DiceConfig& config);
String readSerialLine();
int readMacFromSerial(uint8_t* mac);
uint16_t readColorFromSerial();

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("  ESP32-S3 Unified Sensor Setup Tool");
  Serial.println("========================================");
  Serial.println();

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  displayMainMenu();
}

// ==================== MAIN LOOP ====================
void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read();

    // Clear any remaining characters in buffer
    delay(10);
    while (Serial.available() > 0) {
      Serial.read();
    }

    // Convert to uppercase
    if (input >= 'a' && input <= 'z') {
      input = input - 32;
    }

    Serial.println();

    switch (input) {
      case '1':
        getMacAddress();
        break;

      case '2':
        configureEEPROMSettings();
        break;

      case '3':
        clearEEPROM();
        break;

      case 'M':
        displayMainMenu();
        break;

      default:
        Serial.println("Invalid option. Please try again.");
        Serial.println("Type 'M' to show the menu.");
        break;
    }

    Serial.println();
  }
}

// ==================== MENU DISPLAY ====================
void displayMainMenu() {
  Serial.println("\n========================================");
  Serial.println("           MAIN MENU");
  Serial.println("======= Configuration options ==========");
  Serial.println("1. Get MAC Address");
  Serial.println("2. Configure Quantum Dice (both MAC Address needed!)");
  Serial.println("========== Advanced options ============");
  Serial.println("3. Clear EEPROM");
  Serial.println("========================================");
  Serial.println("M. Show this menu");
  Serial.println("========================================");
  Serial.println("Enter your choice (1-6 or M):");
}

// ==================== MAC ADDRESS ====================
void getMacAddress() {
  Serial.println("\n--- Getting MAC Address ---");

  // Initialize WiFi to get MAC address
  WiFi.mode(WIFI_STA);
  delay(1000);
  String macStr = WiFi.macAddress();

  // Print the original MAC address
  Serial.print("MAC Address: ");
  Serial.println(macStr);
  Serial.println("Copy - paste somewhere for further use");
  // Convert to hexadecimal array
  uint8_t macList[6];
  parseMacAddress(macStr, macList);

  // Turn off WiFi to save power
  WiFi.mode(WIFI_OFF);

  Serial.println("\nPress M for menu");
}

void parseMacAddress(String macStr, uint8_t* macList) {
  int j = 0;
  for (int i = 0; i < macStr.length(); i += 3) {
    macList[j] = strtol(macStr.substring(i, i + 2).c_str(), NULL, 16);
    j++;
  }
}



// ==================== CLEAR EEPROM ====================
void clearEEPROM() {
  Serial.println("\n--- Clear EEPROM ---");
  Serial.println();
  Serial.println("*** WARNING ***");
  Serial.println("This will erase ALL data stored in EEPROM!");
  Serial.println("Including: BNO055 calibration AND device configuration");
  Serial.println();
  Serial.println("Are you sure you want to clear EEPROM?");
  Serial.println("Type 'Y' to confirm or any other key to cancel:");

  // Wait for user input
  while (Serial.available() == 0) {
    delay(10);
  }

  char response = Serial.read();
  while (Serial.available() > 0) Serial.read();  // Clear buffer

  if (response == 'Y' || response == 'y') {
    Serial.println("\nClearing EEPROM...");

    // Write zeros to entire EEPROM space
    for (int i = 0; i < EEPROM_SIZE; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();

    Serial.println("✓ EEPROM cleared successfully!");
    Serial.println();
    Serial.println("All data has been erased.");
    Serial.println("Use option 3 to recalibrate BNO055");
    Serial.println("Use option 6 to reconfigure device settings");
  } else {
    Serial.println("\nOperation cancelled. EEPROM data preserved.");
  }

  Serial.println("\nPress M for menu");
}

// ==================== EEPROM CONFIGURATION ====================
void configureEEPROMSettings() {
  Serial.println("\n========================================");
  Serial.println("    EEPROM Configuration Setup");
  Serial.println("========================================\n");
  
  // Check if configuration already exists
  DiceConfig existingConfig;
  bool hasExisting = readEEPROMConfig(existingConfig);
  
  if (hasExisting) {
    Serial.println("✓ Current configuration found in EEPROM:\n");
    printConfig(existingConfig, "Current Configuration");
    Serial.println("\nOptions:");
    Serial.println("  N - Keep current configuration");
    Serial.println("  Y - Configure new settings");
    Serial.print("\nYour choice: ");
    
    while (Serial.available() == 0) delay(10);
    char response = Serial.read();
    
    // Consume any trailing newlines
    delay(50);
    while (Serial.available() > 0) {
      char c = Serial.peek();
      if (c == '\n' || c == '\r') {
        Serial.read();
      } else {
        break;
      }
    }
    
    Serial.println(response);
    
    if (response != 'Y' && response != 'y') {
      Serial.println("\nKeeping current configuration.");
      Serial.println("Press M for menu");
      return;
    }
  } else {
    Serial.println("⚠ No valid configuration found in EEPROM");
    Serial.println("Let's configure your device!\n");
  }
  
  // Create new configuration starting with appropriate defaults
  DiceConfig newConfig;
  DiceConfig displayDefaults;
  
  if (hasExisting) {
    // Use existing config as the base
    newConfig = existingConfig;
    displayDefaults = existingConfig;
    Serial.println("\nEditing existing configuration...");
  } else {
    // Use hardcoded defaults
    newConfig = defaultConfig;
    displayDefaults = defaultConfig;
    Serial.println("\nCreating new configuration...");
  }
  
  Serial.println("\n========================================");
  Serial.println("  Interactive Configuration");
  Serial.println("========================================");
  Serial.println("Press ENTER to accept default, or type new value");
  Serial.println("Press 'Q' at any time to quit without saving\n");
  
  // Dice ID
  Serial.println("----------------------------------------");
  Serial.printf("Dice ID [%s]: ", displayDefaults.diceId);
  String input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    strncpy(newConfig.diceId, input.c_str(), 15);
    newConfig.diceId[15] = '\0';
  }
  
  // Device A MAC
  Serial.println("\n----------------------------------------");
  Serial.print("Device A MAC [");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", displayDefaults.deviceA_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println("]");
  Serial.println("Enter MAC (format: AA:BB:CC:DD:EE:FF) or press ENTER:");
  int macResult = readMacFromSerial(newConfig.deviceA_mac);
  if (macResult == -1) {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (macResult == 0) {
    // Keep current/default
    memcpy(newConfig.deviceA_mac, displayDefaults.deviceA_mac, 6);
  }
  
  // Device B1 MAC
  Serial.println("\n----------------------------------------");
  Serial.print("Device B1 MAC [");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", displayDefaults.deviceB1_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println("]");
  Serial.println("Enter MAC (format: AA:BB:CC:DD:EE:FF) or press ENTER:");
  macResult = readMacFromSerial(newConfig.deviceB1_mac);
  if (macResult == -1) {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (macResult == 0) {
    memcpy(newConfig.deviceB1_mac, displayDefaults.deviceB1_mac, 6);
  }
  
  // Device B2 MAC
  Serial.println("\n----------------------------------------");
  Serial.print("Device B2 MAC [");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", displayDefaults.deviceB2_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println("]");
  Serial.println("Enter MAC (format: AA:BB:CC:DD:EE:FF) or press ENTER:");
  macResult = readMacFromSerial(newConfig.deviceB2_mac);
  if (macResult == -1) {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (macResult == 0) {
    memcpy(newConfig.deviceB2_mac, displayDefaults.deviceB2_mac, 6);
  }
  
  // X Background Color
  Serial.println("\n----------------------------------------");
  Serial.printf("X Background Color [0x%04X]: ", displayDefaults.x_background);
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.x_background = strtoul(input.c_str(), NULL, 16);
  }
  
  // Y Background Color
  Serial.printf("Y Background Color [0x%04X]: ", displayDefaults.y_background);
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.y_background = strtoul(input.c_str(), NULL, 16);
  }
  
  // Z Background Color
  Serial.printf("Z Background Color [0x%04X]: ", displayDefaults.z_background);
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.z_background = strtoul(input.c_str(), NULL, 16);
  }
  
  // Entanglement AB1 Color
  Serial.printf("Entanglement AB1 Color [0x%04X]: ", displayDefaults.entang_ab1_color);
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.entang_ab1_color = strtoul(input.c_str(), NULL, 16);
  }
  
  // Entanglement AB2 Color
  Serial.printf("Entanglement AB2 Color [0x%04X]: ", displayDefaults.entang_ab2_color);
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.entang_ab2_color = strtoul(input.c_str(), NULL, 16);
  }
  
  // RSSI Limit
  Serial.println("\n----------------------------------------");
  Serial.printf("RSSI Limit in dBm [%d]: ", displayDefaults.rssiLimit);
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.rssiLimit = input.toInt();
  }
  
  // Board Type
  Serial.println("\n----------------------------------------");
  Serial.printf("Is NANO board? (Y/N) [%s]: ", displayDefaults.isNano ? "Y" : "N");
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.isNano = (input[0] == 'Y' || input[0] == 'y');
  }
  
  // Screen Type
  Serial.printf("Is SMD screen? (Y/N) [%s]: ", displayDefaults.isSMD ? "Y" : "N");
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.isSMD = (input[0] == 'Y' || input[0] == 'y');
  }
  
  // Always Seven
  Serial.println("\n----------------------------------------");
  Serial.printf("Always Seven mode? (Y/N) [%s]: ", displayDefaults.alwaysSeven ? "Y" : "N");
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.alwaysSeven = (input[0] == 'Y' || input[0] == 'y');
  }
  
  // Random Switch Point
  Serial.printf("Random Switch Point (0-100) [%d]: ", displayDefaults.randomSwitchPoint);
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.randomSwitchPoint = input.toInt();
  }
  
  // Tumble Constant
  Serial.printf("Tumble Constant [%.2f]: ", displayDefaults.tumbleConstant);
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.tumbleConstant = input.toFloat();
  }
  
  // Deep Sleep Timeout
  Serial.println("\n----------------------------------------");
  Serial.printf("Deep Sleep Timeout in seconds [%d]: ", displayDefaults.deepSleepTimeout / 1000);
  input = readSerialLine();
  if (input == "QUIT_CONFIG") {
    Serial.println("\nConfiguration cancelled by user.");
    Serial.println("Press M for menu");
    return;
  }
  if (input.length() > 0) {
    newConfig.deepSleepTimeout = input.toInt() * 1000;
  }
  
  // Show summary and confirm
  Serial.println("\n========================================");
  Serial.println("  Configuration Summary");
  Serial.println("========================================\n");
  printConfig(newConfig, "New Configuration");
  
  Serial.println("\nWrite this configuration to EEPROM?");
  Serial.println("Type 'Y' to confirm or any other key to cancel:");
  
  while (Serial.available() == 0) delay(10);
  char response = Serial.read();
  
  // Consume any trailing newlines
  delay(50);
  while (Serial.available() > 0) {
    char c = Serial.peek();
    if (c == '\n' || c == '\r') {
      Serial.read();
    } else {
      break;
    }
  }
  
  if (response == 'Y' || response == 'y') {
    // Validate before writing
    if (validateConfig(newConfig)) {
      writeEEPROMConfig(newConfig);
      Serial.println("\n✓ Configuration written to EEPROM successfully!");
      
      // Verify by reading back
      Serial.println("\nVerifying...");
      DiceConfig verifyConfig;
      if (readEEPROMConfig(verifyConfig)) {
        Serial.println("✓ Verification successful!");
      } else {
        Serial.println("⚠ Verification failed - please check configuration");
      }
    } else {
      Serial.println("\n✗ Configuration validation failed!");
      Serial.println("Configuration was NOT written to EEPROM");
    }
  } else {
    Serial.println("\nConfiguration cancelled. EEPROM unchanged.");
  }
  
  Serial.println("\nPress M for menu");
}

// ==================== EEPROM HELPER FUNCTIONS ====================

String readSerialLine() {
  String input = "";
  unsigned long startTime = millis();
  bool gotNewline = false;
  
  // Wait for input with 30 second timeout
  while (millis() - startTime < 30000) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      
      if (c == '\n' || c == '\r') {
        if (input.length() > 0) {
          // Check if user wants to quit
          if (input == "Q" || input == "q") {
            gotNewline = true;
            input = "QUIT_CONFIG";
            break;
          }
          gotNewline = true;
          break;
        } else {
          // Empty input - just newline pressed
          gotNewline = true;
          // Continue reading briefly to consume any paired \r\n or \n\r
          unsigned long newlineTime = millis();
          while (millis() - newlineTime < 50) {
            if (Serial.available() > 0) {
              char nc = Serial.peek();
              if (nc == '\n' || nc == '\r') {
                Serial.read(); // Consume it
              } else {
                break;
              }
            }
            delay(5);
          }
          break;
        }
      } else if (c >= 32 && c <= 126) {  // Printable characters
        input += c;
        Serial.print(c);  // Echo character
      }
    }
    delay(10);
  }
  
  // If we got a newline, consume any remaining newline characters
  if (gotNewline) {
    delay(50);  // Wait for any trailing newlines
    while (Serial.available() > 0) {
      char c = Serial.peek();
      if (c == '\n' || c == '\r') {
        Serial.read();
      } else {
        break;
      }
    }
  }
  
  Serial.println();  // Newline after input
  return input;
}

int readMacFromSerial(uint8_t* mac) {
  // Returns: -1 = quit, 0 = use default, 1 = new MAC entered
  String input = readSerialLine();
  
  // Check if user wants to quit
  if (input == "QUIT_CONFIG") {
    return -1;
  }
  
  if (input.length() == 0) {
    return 0;  // Use default
  }
  
  // Remove colons and spaces
  input.replace(":", "");
  input.replace(" ", "");
  input.toUpperCase();
  
  // Check length
  if (input.length() != 12) {
    Serial.println("⚠ Invalid MAC format, using default");
    return 0;
  }
  
  // Parse hex values
  for (int i = 0; i < 6; i++) {
    String byteStr = input.substring(i * 2, i * 2 + 2);
    mac[i] = strtoul(byteStr.c_str(), NULL, 16);
  }
  
  Serial.print("✓ MAC set to: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  
  return 1;
}

bool validateConfig(const DiceConfig& config) {
  Serial.println("\nValidating configuration...");

  // Check if diceId is null-terminated and contains printable characters
  bool validId = false;
  for (int i = 0; i < 16; i++) {
    if (config.diceId[i] == '\0') {
      validId = (i > 0);
      break;
    }
    if (!isprint(config.diceId[i])) {
      Serial.println("  ✗ Invalid diceId characters");
      return false;
    }
  }

  if (!validId) {
    Serial.println("  ✗ Invalid diceId format");
    return false;
  }

  // Validate RSSI limit
  if (config.rssiLimit > 0 || config.rssiLimit < -100) {
    Serial.printf("  ✗ Invalid RSSI limit: %d\n", config.rssiLimit);
    return false;
  }

  // Validate randomSwitchPoint
  if (config.randomSwitchPoint > 100) {
    Serial.printf("  ✗ Invalid randomSwitchPoint: %d\n", config.randomSwitchPoint);
    return false;
  }

  // Validate tumbleConstant
  if (config.tumbleConstant <= 0 || config.tumbleConstant > 10.0) {
    Serial.printf("  ✗ Invalid tumbleConstant: %.2f\n", config.tumbleConstant);
    return false;
  }

  // Validate deepSleepTimeout
  if (config.deepSleepTimeout < 10000 || config.deepSleepTimeout > 3600000) {
    Serial.printf("  ✗ Invalid deepSleepTimeout: %lu ms\n", config.deepSleepTimeout);
    return false;
  }

  Serial.println("✓ All validation checks passed");
  return true;
}

void printConfig(const DiceConfig& config, const char* title) {
  Serial.println("========================================");
  Serial.println(title);
  Serial.println("========================================");

  Serial.print("Dice ID: ");
  Serial.println(config.diceId);

  Serial.print("Device A MAC:  ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", config.deviceA_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  Serial.print("Device B1 MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", config.deviceB1_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  Serial.print("Device B2 MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", config.deviceB2_mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  Serial.println("\nDisplay Colors:");
  Serial.printf("  X Background:       0x%04X\n", config.x_background);
  Serial.printf("  Y Background:       0x%04X\n", config.y_background);
  Serial.printf("  Z Background:       0x%04X\n", config.z_background);
  Serial.printf("  Entanglement AB1:   0x%04X\n", config.entang_ab1_color);
  Serial.printf("  Entanglement AB2:   0x%04X\n", config.entang_ab2_color);

  Serial.println("\nHardware Configuration:");
  Serial.printf("  Board Type:         %s\n", config.isNano ? "NANO" : "DEVKIT");
  Serial.printf("  Screen Type:        %s\n", config.isSMD ? "SMD" : "HDR");

  Serial.println("\nOperational Parameters:");
  Serial.printf("  RSSI Limit:         %d dBm\n", config.rssiLimit);
  Serial.printf("  Always Seven:       %s\n", config.alwaysSeven ? "Yes" : "No");
  Serial.printf("  Random Switch:      %d\n", config.randomSwitchPoint);
  Serial.printf("  Tumble Constant:    %.2f\n", config.tumbleConstant);
  Serial.printf("  Sleep Timeout:      %lu ms (%.1f min)\n",
                config.deepSleepTimeout,
                config.deepSleepTimeout / 60000.0);

  Serial.println("========================================");
}

bool readEEPROMConfig(DiceConfig& config) {
  EEPROM.get(EEPROM_CONFIG_ADDRESS, config);
  return validateConfig(config);
}

void writeEEPROMConfig(const DiceConfig& config) {
  Serial.println("\nWriting configuration to EEPROM...");

  // Write byte-by-byte for reliability
  const uint8_t* data = (const uint8_t*)&config;
  for (size_t i = 0; i < sizeof(DiceConfig); i++) {
    EEPROM.write(EEPROM_CONFIG_ADDRESS + i, data[i]);
  }

  // Commit to flash
  if (EEPROM.commit()) {
    Serial.println("✓ EEPROM commit successful");
  } else {
    Serial.println("✗ EEPROM commit failed!");
  }

  delay(100);  // Ensure write completes
}