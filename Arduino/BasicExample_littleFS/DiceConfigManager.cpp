/*
 * DiceConfigManager - Implementation
 */

#include "DiceConfigManager.h"

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
  _config.x_background = 0xF800;      // Red
  _config.y_background = 0x07E0;      // Green
  _config.z_background = 0x001F;      // Blue
  _config.entang_ab1_color = 0xFFFF;  // White
  _config.entang_ab2_color = 0x0000;  // Black
  
  // Default RSSI
  _config.rssiLimit = -70;
  
  // Default hardware config
  _config.isSMD = false;
  _config.isNano = false;
  _config.alwaysSeven = false;
  
  // Default operational parameters
  _config.randomSwitchPoint = 50;
  _config.tumbleConstant = 2.5;
  _config.deepSleepTimeout = 300000;  // 5 minutes
  
  // Checksum (will be calculated on save)
  _config.checksum = 0;
}
