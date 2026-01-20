/*
 * DiceConfigManager - Implementation
 */

#include "DiceConfigManager.h"
#include <WiFi.h>

// Constructor
DiceConfigManager::DiceConfigManager() {
  _verbose = false;
  _lastError[0] = '\0';
  strcpy(_configPath, "/config.txt");
  initDefaultConfig();
}

// Initialize LittleFS and optionally load config
bool DiceConfigManager::begin(const char* configPath, bool formatOnFail) {
  if (!LittleFS.begin(formatOnFail)) {
    setError("LittleFS mount failed");
    return false;
  }
  
  if (_verbose) {
    Serial.println("LittleFS mounted successfully");
  }
  
  // If no explicit path provided, try auto-detection
  if (configPath == nullptr) {
    if (_verbose) {
      Serial.println("No config path specified, searching for *_config.txt...");
    }
    
    if (findConfigFile(_configPath, sizeof(_configPath))) {
      if (_verbose) {
        Serial.printf("Auto-detected config file: %s\n", _configPath);
      }
    } else {
      // No config file found or multiple found
      if (_verbose) {
        Serial.println("No unique config file found, using defaults");
      }
      strcpy(_configPath, "/config.txt"); // Set default for save operations
      setDefaults();
      return true; // Not a critical error
    }
  } else {
    // Explicit path provided
    strncpy(_configPath, configPath, sizeof(_configPath) - 1);
    _configPath[sizeof(_configPath) - 1] = '\0';
  }
  
  // Try to load existing config, otherwise use defaults
  if (!load()) {
    if (_verbose) {
      Serial.println("Config file not loaded, using defaults");
    }
    setDefaults();
    return true; // Not a critical error
  }
  
  return true;
}

// Load configuration from file
bool DiceConfigManager::load() {
  return load(_configPath);
}

bool DiceConfigManager::load(const char* filename) {
  File file = LittleFS.open(filename, "r");
  if (!file) {
    setError("Failed to open config file");
    return false;
  }

  char line[128];
  int lineNum = 0;
  bool success = true;
  
  while (file.available()) {
    int len = file.readBytesUntil('\n', line, sizeof(line) - 1);
    line[len] = '\0';
    lineNum++;
    
    // Remove carriage return if present
    if (len > 0 && line[len - 1] == '\r') {
      line[len - 1] = '\0';
    }
    
    trim(line);
    
    // Skip empty lines and comments
    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }
    
    // Find the '=' separator
    char* separator = strchr(line, '=');
    if (!separator) {
      if (_verbose) {
        Serial.printf("Line %d: Invalid format (no '=')\n", lineNum);
      }
      continue;
    }
    
    // Split into key and value
    *separator = '\0';
    char* key = line;
    char* value = separator + 1;
    
    trim(key);
    
    // Strip inline comments from value (everything after #)
    char* comment = strchr(value, '#');
    if (comment) {
      *comment = '\0';  // Terminate string at comment
    }
    
    trim(value);
    
    // Parse based on key
    if (strcmp(key, "diceId") == 0) {
      strncpy(_config.diceId, value, sizeof(_config.diceId) - 1);
      _config.diceId[sizeof(_config.diceId) - 1] = '\0';
    }
    else if (strcmp(key, "deviceA_mac") == 0) {
      if (!parseMacAddress(value, _config.deviceA_mac)) {
        if (_verbose) {
          Serial.printf("Line %d: Invalid MAC address format\n", lineNum);
        }
      }
    }
    else if (strcmp(key, "deviceB1_mac") == 0) {
      parseMacAddress(value, _config.deviceB1_mac);
    }
    else if (strcmp(key, "deviceB2_mac") == 0) {
      parseMacAddress(value, _config.deviceB2_mac);
    }
    else if (strcmp(key, "x_background") == 0) {
      _config.x_background = (uint16_t)strtoul(value, NULL, 0);
    }
    else if (strcmp(key, "y_background") == 0) {
      _config.y_background = (uint16_t)strtoul(value, NULL, 0);
    }
    else if (strcmp(key, "z_background") == 0) {
      _config.z_background = (uint16_t)strtoul(value, NULL, 0);
    }
    else if (strcmp(key, "entang_ab1_color") == 0) {
      _config.entang_ab1_color = (uint16_t)strtoul(value, NULL, 0);
    }
    else if (strcmp(key, "entang_ab2_color") == 0) {
      _config.entang_ab2_color = (uint16_t)strtoul(value, NULL, 0);
    }
    else if (strcmp(key, "rssiLimit") == 0) {
      _config.rssiLimit = (int8_t)atoi(value);
    }
    else if (strcmp(key, "isSMD") == 0) {
      _config.isSMD = parseBool(value);
    }
    else if (strcmp(key, "isNano") == 0) {
      _config.isNano = parseBool(value);
    }
    else if (strcmp(key, "alwaysSeven") == 0) {
      _config.alwaysSeven = parseBool(value);
    }
    else if (strcmp(key, "randomSwitchPoint") == 0) {
      _config.randomSwitchPoint = (uint8_t)atoi(value);
    }
    else if (strcmp(key, "tumbleConstant") == 0) {
      _config.tumbleConstant = atof(value);
    }
    else if (strcmp(key, "deepSleepTimeout") == 0) {
      _config.deepSleepTimeout = strtoul(value, NULL, 0);
    }
    else if (strcmp(key, "checksum") == 0) {
      _config.checksum = (uint8_t)atoi(value);
    }
    else {
      if (_verbose) {
        Serial.printf("Line %d: Unknown key '%s'\n", lineNum, key);
      }
    }
  }
  
  file.close();
  
  // Validate checksum if not 0
  if (_config.checksum != 0) {
    if (!validateChecksum(_config)) {
      setError("Checksum validation failed");
      if (_verbose) {
        Serial.println("Warning: Checksum validation failed!");
      }
      success = false;
    }
  }
  
  if (_verbose && success) {
    Serial.println("Config loaded successfully");
  }
  
  return success;
}

// Save configuration to file
bool DiceConfigManager::save() {
  return save(_configPath);
}

bool DiceConfigManager::save(const char* filename) {
  // Calculate checksum before saving
  calculateChecksum(_config);
  
  File file = LittleFS.open(filename, "w");
  if (!file) {
    setError("Failed to open config file for writing");
    return false;
  }
  
  // Write header
  file.println("# Dice Configuration File");
  file.println("# Auto-generated - Edit with care");
  file.println();
  
  // Device Identification
  file.println("# Device Identification");
  file.printf("diceId=%s\n", _config.diceId);
  file.println();
  
  // MAC Addresses
  file.println("# Device MAC Addresses (format: AA:BB:CC:DD:EE:FF)");
  file.printf("deviceA_mac=%s\n", macToString(_config.deviceA_mac).c_str());
  file.printf("deviceB1_mac=%s\n", macToString(_config.deviceB1_mac).c_str());
  file.printf("deviceB2_mac=%s\n", macToString(_config.deviceB2_mac).c_str());
  file.println();
  
  // Display Colors
  file.println("# Display Colors (16-bit RGB565 format)");
  file.printf("x_background=%u\n", _config.x_background);
  file.printf("y_background=%u\n", _config.y_background);
  file.printf("z_background=%u\n", _config.z_background);
  file.printf("entang_ab1_color=%u\n", _config.entang_ab1_color);
  file.printf("entang_ab2_color=%u\n", _config.entang_ab2_color);
  file.println();
  
  // RSSI Settings
  file.println("# RSSI Settings");
  file.printf("rssiLimit=%d\n", _config.rssiLimit);
  file.println();
  
  // Hardware Configuration
  file.println("# Hardware Configuration");
  file.printf("isSMD=%s\n", _config.isSMD ? "true" : "false");
  file.printf("isNano=%s\n", _config.isNano ? "true" : "false");
  file.printf("alwaysSeven=%s\n", _config.alwaysSeven ? "true" : "false");
  file.println();
  
  // Operational Parameters
  file.println("# Operational Parameters");
  file.printf("randomSwitchPoint=%u\n", _config.randomSwitchPoint);
  file.printf("tumbleConstant=%.2f\n", _config.tumbleConstant);
  file.printf("deepSleepTimeout=%u\n", _config.deepSleepTimeout);
  file.println();
  
  // Checksum
  file.println("# Checksum (auto-calculated)");
  file.printf("checksum=%u\n", _config.checksum);
  
  file.close();
  
  if (_verbose) {
    Serial.println("Config saved successfully");
  }
  
  return true;
}

// Reset to default values
void DiceConfigManager::setDefaults() {
  initDefaultConfig();
  if (_verbose) {
    Serial.println("Configuration reset to defaults");
  }
}

// Validate current configuration
bool DiceConfigManager::validate() {
  bool valid = true;
  
  // Check dice ID is not empty
  if (strlen(_config.diceId) == 0) {
    if (_verbose) Serial.println("Validation error: diceId is empty");
    valid = false;
  }
  
  // Check randomSwitchPoint is in range
  if (_config.randomSwitchPoint > 100) {
    if (_verbose) Serial.println("Validation error: randomSwitchPoint > 100");
    valid = false;
  }
  
  // Check tumbleConstant is positive
  if (_config.tumbleConstant <= 0) {
    if (_verbose) Serial.println("Validation error: tumbleConstant <= 0");
    valid = false;
  }
  
  // Validate checksum
  if (_config.checksum != 0 && !validateChecksum(_config)) {
    if (_verbose) Serial.println("Validation error: checksum mismatch");
    valid = false;
  }
  
  return valid;
}

// Get configuration
DiceConfig& DiceConfigManager::getConfig() {
  return _config;
}

// Set configuration
void DiceConfigManager::setConfig(const DiceConfig& newConfig) {
  _config = newConfig;
}

// Individual setters
void DiceConfigManager::setDiceId(const char* id) {
  strncpy(_config.diceId, id, sizeof(_config.diceId) - 1);
  _config.diceId[sizeof(_config.diceId) - 1] = '\0';
}

void DiceConfigManager::setDeviceAMac(const uint8_t* mac) {
  memcpy(_config.deviceA_mac, mac, 6);
}

void DiceConfigManager::setDeviceB1Mac(const uint8_t* mac) {
  memcpy(_config.deviceB1_mac, mac, 6);
}

void DiceConfigManager::setDeviceB2Mac(const uint8_t* mac) {
  memcpy(_config.deviceB2_mac, mac, 6);
}

void DiceConfigManager::setRssiLimit(int8_t limit) {
  _config.rssiLimit = limit;
}

void DiceConfigManager::setIsSMD(bool value) {
  _config.isSMD = value;
}

void DiceConfigManager::setIsNano(bool value) {
  _config.isNano = value;
}

void DiceConfigManager::setAlwaysSeven(bool value) {
  _config.alwaysSeven = value;
}

// Print configuration
void DiceConfigManager::printConfig() {
  Serial.println("=== Dice Configuration ===");
  Serial.printf("Dice ID: %s\n", _config.diceId);
  Serial.print("Device A MAC: "); printMacAddress(_config.deviceA_mac);
  Serial.print("Device B1 MAC: "); printMacAddress(_config.deviceB1_mac);
  Serial.print("Device B2 MAC: "); printMacAddress(_config.deviceB2_mac);
  Serial.printf("X Background: 0x%04X (%u)\n", _config.x_background, _config.x_background);
  Serial.printf("Y Background: 0x%04X (%u)\n", _config.y_background, _config.y_background);
  Serial.printf("Z Background: 0x%04X (%u)\n", _config.z_background, _config.z_background);
  Serial.printf("Entangle AB1 Color: 0x%04X (%u)\n", _config.entang_ab1_color, _config.entang_ab1_color);
  Serial.printf("Entangle AB2 Color: 0x%04X (%u)\n", _config.entang_ab2_color, _config.entang_ab2_color);
  Serial.printf("RSSI Limit: %d dBm\n", _config.rssiLimit);
  Serial.printf("Is SMD: %s\n", _config.isSMD ? "true" : "false");
  Serial.printf("Is Nano: %s\n", _config.isNano ? "true" : "false");
  Serial.printf("Always Seven: %s\n", _config.alwaysSeven ? "true" : "false");
  Serial.printf("Random Switch Point: %u%%\n", _config.randomSwitchPoint);
  Serial.printf("Tumble Constant: %.2f\n", _config.tumbleConstant);
  Serial.printf("Deep Sleep Timeout: %u ms\n", _config.deepSleepTimeout);
  Serial.printf("Checksum: 0x%02X\n", _config.checksum);
  Serial.println("==========================");
}

void DiceConfigManager::printMacAddress(const uint8_t* mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

String DiceConfigManager::macToString(const uint8_t* mac) {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buffer);
}

const char* DiceConfigManager::getLastError() {
  return _lastError;
}

void DiceConfigManager::setVerbose(bool enabled) {
  _verbose = enabled;
}

const char* DiceConfigManager::getConfigPath() {
  return _configPath;
}

// Private methods
bool DiceConfigManager::findConfigFile(char* foundPath, size_t maxLen) {
  File root = LittleFS.open("/");
  if (!root) {
    if (_verbose) {
      Serial.println("Failed to open root directory");
    }
    return false;
  }
  
  if (!root.isDirectory()) {
    if (_verbose) {
      Serial.println("Root is not a directory");
    }
    return false;
  }
  
  int matchCount = 0;
  char firstMatch[64] = {0};
  
  File file = root.openNextFile();
  while (file) {
    String filename = String(file.name());
    
    // Check if filename matches pattern: *_config.txt
    if (filename.endsWith("_config.txt")) {
      matchCount++;
      
      if (_verbose) {
        Serial.printf("Found config file: %s\n", filename.c_str());
      }
      
      // Store first match
      if (matchCount == 1) {
        strncpy(firstMatch, filename.c_str(), sizeof(firstMatch) - 1);
        firstMatch[sizeof(firstMatch) - 1] = '\0';
      }
    }
    
    file = root.openNextFile();
  }
  
  root.close();
  
  // Return success only if exactly one match found
  if (matchCount == 1) {
    // Ensure path starts with /
    if (firstMatch[0] != '/') {
      snprintf(foundPath, maxLen, "/%s", firstMatch);
    } else {
      strncpy(foundPath, firstMatch, maxLen - 1);
      foundPath[maxLen - 1] = '\0';
    }
    return true;
  } else if (matchCount == 0) {
    setError("No *_config.txt file found");
    if (_verbose) {
      Serial.println("No files matching *_config.txt pattern found");
    }
  } else {
    setError("Multiple *_config.txt files found");
    if (_verbose) {
      Serial.printf("Error: Found %d config files. Only one allowed.\n", matchCount);
    }
  }
  
  return false;
}

bool DiceConfigManager::parseMacAddress(const char* str, uint8_t* mac) {
  int values[6];
  if (sscanf(str, "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; i++) {
      mac[i] = (uint8_t)values[i];
    }
    return true;
  }
  return false;
}

bool DiceConfigManager::parseBool(const char* str) {
  if (strcasecmp(str, "true") == 0 || strcmp(str, "1") == 0) {
    return true;
  }
  return false;
}

void DiceConfigManager::trim(char* str) {
  // Trim leading space
  char* start = str;
  while (isspace((unsigned char)*start)) start++;
  
  if (start != str) {
    memmove(str, start, strlen(start) + 1);
  }
  
  // Trim trailing space
  char* end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)) end--;
  end[1] = '\0';
}

void DiceConfigManager::calculateChecksum(DiceConfig& config) {
  uint8_t* ptr = (uint8_t*)&config;
  uint8_t sum = 0;
  
  // XOR all bytes except the checksum field itself
  for (size_t i = 0; i < sizeof(DiceConfig) - 1; i++) {
    sum ^= ptr[i];
  }
  
  config.checksum = sum;
}

bool DiceConfigManager::validateChecksum(const DiceConfig& config) {
  DiceConfig temp = config;
  uint8_t originalChecksum = temp.checksum;
  calculateChecksum(temp);
  return temp.checksum == originalChecksum;
}

void DiceConfigManager::setError(const char* error) {
  strncpy(_lastError, error, sizeof(_lastError) - 1);
  _lastError[sizeof(_lastError) - 1] = '\0';
  if (_verbose) {
    Serial.printf("Error: %s\n", error);
  }
}

void DiceConfigManager::initDefaultConfig() {
  // Default values
  strcpy(_config.diceId, "DEFAULT");
  
  // Default MAC addresses (all zeros)
  memset(_config.deviceA_mac, 0, 6);
  memset(_config.deviceB1_mac, 0, 6);
  memset(_config.deviceB2_mac, 0, 6);
  
  // Default colors (RGB565)
  _config.x_background = 0x0;      // Black
  _config.y_background = 0x0;      // Black
  _config.z_background = 0x0;      // Black
  _config.entang_ab1_color = 0xFFE0;  // Yellow
  _config.entang_ab2_color = 0x07E0;  // Green
  
  // Default RSSI
  _config.rssiLimit = -35;
  
  // Default hardware config
  _config.isSMD = true;
  _config.isNano = false;
  _config.alwaysSeven = false;
  
  // Default operational parameters
  _config.randomSwitchPoint = 50;
  _config.tumbleConstant = 45; //=cos(45)
  _config.deepSleepTimeout = 300000;  // 5 minutes
  
  // Checksum (will be calculated on save)
  _config.checksum = 0;
}

// ============================================================================
// GLOBAL CONFIG ACCESS IMPLEMENTATION
// ============================================================================

// Define the global config instance
DiceConfig currentConfig;

// Internal static config manager for global access
static DiceConfigManager _globalConfigManager;

bool loadGlobalConfig(bool verbose) {
  _globalConfigManager.setVerbose(verbose);
  
  if (!_globalConfigManager.begin()) {
    Serial.println("Failed to load global config!");
    return false;
  }
  
  if (verbose) {
    Serial.printf("Loaded global config from: %s\n", _globalConfigManager.getConfigPath());
  }
  
  // Copy config to global instance
  currentConfig = _globalConfigManager.getConfig();
  
  return true;
}

void printGlobalConfig() {
  Serial.println("=== Global Configuration ===");
  Serial.printf("Dice ID: %s\n", currentConfig.diceId);
  
  Serial.print("Device A MAC: ");
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                currentConfig.deviceA_mac[0], currentConfig.deviceA_mac[1], 
                currentConfig.deviceA_mac[2], currentConfig.deviceA_mac[3],
                currentConfig.deviceA_mac[4], currentConfig.deviceA_mac[5]);
  
  Serial.print("Device B1 MAC: ");
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                currentConfig.deviceB1_mac[0], currentConfig.deviceB1_mac[1],
                currentConfig.deviceB1_mac[2], currentConfig.deviceB1_mac[3],
                currentConfig.deviceB1_mac[4], currentConfig.deviceB1_mac[5]);
  
  Serial.print("Device B2 MAC: ");
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                currentConfig.deviceB2_mac[0], currentConfig.deviceB2_mac[1],
                currentConfig.deviceB2_mac[2], currentConfig.deviceB2_mac[3],
                currentConfig.deviceB2_mac[4], currentConfig.deviceB2_mac[5]);
  
  Serial.printf("X Background: 0x%04X (%u)\n", currentConfig.x_background, currentConfig.x_background);
  Serial.printf("Y Background: 0x%04X (%u)\n", currentConfig.y_background, currentConfig.y_background);
  Serial.printf("Z Background: 0x%04X (%u)\n", currentConfig.z_background, currentConfig.z_background);
  Serial.printf("Entangle AB1 Color: 0x%04X (%u)\n", currentConfig.entang_ab1_color, currentConfig.entang_ab1_color);
  Serial.printf("Entangle AB2 Color: 0x%04X (%u)\n", currentConfig.entang_ab2_color, currentConfig.entang_ab2_color);
  
  Serial.printf("RSSI Limit: %d dBm\n", currentConfig.rssiLimit);
  Serial.printf("Is SMD: %s\n", currentConfig.isSMD ? "true" : "false");
  Serial.printf("Is Nano: %s\n", currentConfig.isNano ? "true" : "false");
  Serial.printf("Always Seven: %s\n", currentConfig.alwaysSeven ? "true" : "false");
  Serial.printf("Random Switch Point: %u%%\n", currentConfig.randomSwitchPoint);
  Serial.printf("Tumble Constant: %.2f\n", currentConfig.tumbleConstant);
  Serial.printf("Deep Sleep Timeout: %u ms\n", currentConfig.deepSleepTimeout);
  Serial.printf("Checksum: 0x%02X\n", currentConfig.checksum);
  Serial.println("============================");
}

bool saveGlobalConfig() {
  // Update the internal config manager with global values
  _globalConfigManager.setConfig(currentConfig);
  
  // Save to file
  if (!_globalConfigManager.save()) {
    Serial.println("Failed to save global config!");
    Serial.println(_globalConfigManager.getLastError());
    return false;
  }
  
  Serial.println("Global config saved successfully!");
  return true;
}

// ============================================================================
// AUTO-INITIALIZATION AND VALIDATION FUNCTIONS
// ============================================================================

bool ensureLittleFSAndConfig(bool verbose) {
  // Step 1: Ensure LittleFS is mounted (format if needed)
  if (verbose) {
    Serial.println("Mounting LittleFS...");
  }
  
  if (!LittleFS.begin(true)) {  // true = format if mount fails
    Serial.println("Failed to mount/format LittleFS!");
    return false;
  }
  
  if (verbose) {
    Serial.println("LittleFS mounted successfully");
    Serial.printf("Total: %u bytes, Used: %u bytes\n", 
                  LittleFS.totalBytes(), LittleFS.usedBytes());
  }
  
  // Step 2: Check if any config file exists
  bool configExists = false;
  String foundConfigPath = "";
  
  File root = LittleFS.open("/");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      String filename = String(file.name());
      if (filename.endsWith("_config.txt") || filename == "/config.txt") {
        configExists = true;
        foundConfigPath = filename;
        if (verbose) {
          Serial.printf("Found existing config file: %s\n", filename.c_str());
        }
        file.close();
        break;
      }
      file.close();
      file = root.openNextFile();
    }
    root.close();
  }
  
  // Step 3: If no config exists, create a default one
  if (!configExists) {
    if (verbose) {
      Serial.println("No config file found, creating default config.txt...");
    }
    
    if (!createDefaultConfigFile("/config.txt", verbose)) {
      Serial.println("Failed to create default config!");
      return false;
    }
  }
  
  // Step 4: Load the config into currentConfig
  return loadGlobalConfig(verbose);
}

bool createDefaultConfigFile(const char* filename, bool verbose) {
  File file = LittleFS.open(filename, "w");
  if (!file) {
    if (verbose) {
      Serial.printf("Failed to create file: %s\n", filename);
    }
    return false;
  }
  
  // Write default configuration with helpful comments
  file.println("# ==========================================");
  file.println("# QUANTUM DICE DEFAULT CONFIGURATION");
  file.println("# ⚠️  IMPORTANT: Upload your custom config!");
  file.println("# ==========================================");
  file.println();
  file.println("# Device Identification - CHANGE THIS!");
  file.println("diceId=DEFAULT");
  file.println();
  file.println("# ⚠️  CRITICAL: Update with your actual MAC addresses!");
  file.println("# Device CANNOT operate without valid MAC addresses");
  file.println("# These are required for ESP-NOW entanglement");
  file.println("# Get MACs from serial output or device label");
  file.println("deviceA_mac=00:00:00:00:00:00");
  file.println("deviceB1_mac=00:00:00:00:00:00");
  file.println("deviceB2_mac=00:00:00:00:00:00");
  file.println();
  file.println("# Display Colors (RGB565) - defaults work fine");
  file.println("x_background=63488");
  file.println("y_background=2016");
  file.println("z_background=31");
  file.println("entang_ab1_color=65535");
  file.println("entang_ab2_color=0");
  file.println();
  file.println("# RSSI Settings");
  file.println("rssiLimit=-70");
  file.println();
  file.println("# Hardware Configuration");
  file.println("isSMD=false");
  file.println("isNano=false");
  file.println("alwaysSeven=false");
  file.println();
  file.println("# Operational Parameters");
  file.println("randomSwitchPoint=50");
  file.println("tumbleConstant=2.5");
  file.println("deepSleepTimeout=300000");
  file.println();
  file.println("# Checksum (auto-calculated on save)");
  file.println("checksum=0");
  
  file.close();
  
  if (verbose) {
    Serial.printf("Created default config file: %s\n", filename);
  }
  
  return true;
}

bool isValidMacAddress(const uint8_t* mac) {
  // Check if MAC is not all zeros
  for (int i = 0; i < 6; i++) {
    if (mac[i] != 0x00) {
      return true;  // At least one non-zero byte
    }
  }
  return false;  // All zeros = invalid
}

bool hasValidMacAddresses() {
  return isValidMacAddress(currentConfig.deviceA_mac) &&
         isValidMacAddress(currentConfig.deviceB1_mac) &&
         isValidMacAddress(currentConfig.deviceB2_mac);
}

bool isInSetupMode() {
  // Check if using default ID
  if (strcmp(currentConfig.diceId, "DEFAULT") == 0) {
    return true;
  }
  
  // Check if MAC addresses are valid
  if (!hasValidMacAddresses()) {
    return true;
  }
  
  return false;
}

// ============================================================================
// SAFE MODE BOOT FUNCTIONS
// ============================================================================

void setHardwareDefaults() {
  currentConfig.isNano = false;  // DEVKIT
  currentConfig.isSMD = true;    // SMD mounting
}

void enterSetupMode() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║     SETUP MODE - CONFIG REQUIRED      ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  Serial.println("No valid configuration found!");
  Serial.println("Displays should show: CONFIG MODE");
  Serial.println();
  
  // Get and display MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.println("═══════════════════════════════════════");
  Serial.println("DEVICE INFORMATION:");
  Serial.println("═══════════════════════════════════════");
  Serial.printf("  MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("  Hardware: %s\n", currentConfig.isNano ? "NANO" : "DEVKIT");
  Serial.printf("  Mounting: %s\n", currentConfig.isSMD ? "SMD" : "HDR");
  Serial.println();
  
  Serial.println("═══════════════════════════════════════");
  Serial.println("NEXT STEPS:");
  Serial.println("═══════════════════════════════════════");
  Serial.println();
  Serial.println("1. STOP the Serial Monitor");
  Serial.println("   └─> Click STOP button");
  Serial.println("   └─> Returns device to stub mode");
  Serial.println();
  Serial.println("2. Open 'LittleFS Tools'");
  Serial.println("   └─> Click in left sidebar");
  Serial.println("   └─> Filesystem is ready");
  Serial.println();
  Serial.println("3. Upload your config file:");
  Serial.println("   └─> Name: YOURNAME_config.txt");
  Serial.println("   └─> Must contain:");
  Serial.println("       • diceId=YOURNAME");
  Serial.printf("       • deviceA_mac=%02X:%02X:%02X:%02X:%02X:%02X (this device or peer)\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println("       • deviceB1_mac=XX:XX:XX:XX:XX:XX");
  Serial.println("       • deviceB2_mac=XX:XX:XX:XX:XX:XX");
  Serial.println("       • Other settings (see template)");
  Serial.println();
  Serial.println("4. Restart device:");
  Serial.println("   Option A: START Serial Monitor again");
  Serial.println("   Option B: DISCONNECT and reconnect");
  Serial.println();
  Serial.println("═══════════════════════════════════════");
  Serial.println("Waiting for config upload...");
  Serial.println("═══════════════════════════════════════");
  
  // Idle loop with periodic reminders
  unsigned long lastReminder = 0;
  int reminderCount = 0;
  
  while(true) {
    // Print reminder every 30 seconds
    if (millis() - lastReminder > 30000) {
      reminderCount++;
      Serial.println();
      Serial.printf("⏳ Still waiting... (%d minutes)\n", reminderCount / 2);
      Serial.println("   Remember to STOP Serial Monitor before uploading!");
      
      lastReminder = millis();
    }
    
    delay(100);
  }
}