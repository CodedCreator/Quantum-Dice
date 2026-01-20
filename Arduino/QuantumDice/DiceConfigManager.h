/*
 * DiceConfigManager - Configuration management library for ESP32 Dice
 * Handles loading, saving, and validating configuration from LittleFS
 * 
 * Author: Auto-generated
 * License: MIT
 */

#ifndef DICE_CONFIG_MANAGER_H
#define DICE_CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>

// Configuration structure
struct DiceConfig {
  char diceId[16];              // "TEST1", "BART1", etc.
  uint8_t deviceA_mac[6];       // MAC address of device A
  uint8_t deviceB1_mac[6];      // MAC address of device B1
  uint8_t deviceB2_mac[6];      // MAC address of device B2
  uint16_t x_background;        // Display background colors
  uint16_t y_background;
  uint16_t z_background;
  uint16_t entang_ab1_color;
  uint16_t entang_ab2_color;
  int8_t rssiLimit;             // RSSI limit for entanglement detection
  bool isSMD;                   // true for SMD, false for HDR
  bool isNano;                  // true for NANO, false for DEVKIT
  bool alwaysSeven;             // Force dice to always produce 7
  uint8_t randomSwitchPoint;    // Threshold for random value (0-100)
  float tumbleConstant;         // Number of tumbles to detect tumbling
  uint32_t deepSleepTimeout;    // Deep sleep timeout in milliseconds
  uint8_t checksum;             // Simple checksum for validation
};

class DiceConfigManager {
public:
  // Constructor
  DiceConfigManager();
  
  // Initialize LittleFS and load config
  bool begin(const char* configPath = nullptr, bool formatOnFail = true);
  
  // Load configuration from file
  bool load();
  bool load(const char* filename);
  
  // Save configuration to file
  bool save();
  bool save(const char* filename);
  
  // Reset to default values
  void setDefaults();
  
  // Validate current configuration
  bool validate();
  
  // Get/Set configuration
  DiceConfig& getConfig();
  void setConfig(const DiceConfig& newConfig);
  
  // Individual field setters (convenience methods)
  void setDiceId(const char* id);
  void setDeviceAMac(const uint8_t* mac);
  void setDeviceB1Mac(const uint8_t* mac);
  void setDeviceB2Mac(const uint8_t* mac);
  void setRssiLimit(int8_t limit);
  void setIsSMD(bool value);
  void setIsNano(bool value);
  void setAlwaysSeven(bool value);
  
  // Utility functions
  void printConfig();
  void printMacAddress(const uint8_t* mac);
  String macToString(const uint8_t* mac);
  
  // Get last error message
  const char* getLastError();
  
  // Enable/disable verbose logging
  void setVerbose(bool enabled);
  
  // Get the currently loaded config filename
  const char* getConfigPath();

private:
  DiceConfig _config;
  char _configPath[64];
  char _lastError[128];
  bool _verbose;
  
  // Auto-detection helper
  bool findConfigFile(char* foundPath, size_t maxLen);
  
  // Internal parsing functions
  bool parseMacAddress(const char* str, uint8_t* mac);
  bool parseBool(const char* str);
  void trim(char* str);
  void calculateChecksum(DiceConfig& config);
  bool validateChecksum(const DiceConfig& config);
  void setError(const char* error);
  
  // Default configuration values
  void initDefaultConfig();
};

// ============================================================================
// GLOBAL CONFIG ACCESS (EEPROM-style pattern)
// ============================================================================
// Global config instance - accessible from all libraries
extern DiceConfig currentConfig;

// Helper functions for global config management
bool loadGlobalConfig(bool verbose = false);
void printGlobalConfig();
bool saveGlobalConfig();

// Auto-initialization function (formats LittleFS and creates default config if needed)
bool ensureLittleFSAndConfig(bool verbose = false);

// Validation functions
bool isValidMacAddress(const uint8_t* mac);
bool hasValidMacAddresses();
bool isInSetupMode();

// Internal helper (exposed for advanced use)
bool createDefaultConfigFile(const char* filename, bool verbose = false);

// ============================================================================
// SAFE MODE BOOT FUNCTIONS
// ============================================================================
// Set minimal hardware defaults (isNano, isSMD only)
void setHardwareDefaults();

// Enter safe mode and wait for config upload
void enterSetupMode();

#endif // DICE_CONFIG_MANAGER_H
