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

// Configuration structure
struct DiceConfig {
    char     diceId[16];   // "TEST1", "BART1", etc.
    uint16_t x_background; // Display background colors
    uint16_t y_background;
    uint16_t z_background;
    uint16_t entang_colors[8];    // Available entanglement colors (RGB565)
    uint8_t  entang_colors_count; // Number of colors in array
    int8_t   rssiLimit;           // RSSI limit for entanglement detection
    bool     isSMD;               // true for SMD, false for HDR
    bool     isNano;              // true for NANO, false for DEVKIT
    bool     alwaysSeven;         // Force dice to always produce 7
    uint8_t  randomSwitchPoint;   // Threshold for random value (0-100)
    float    tumbleConstant;      // Number of tumbles to detect tumbling
    uint32_t deepSleepTimeout;    // Deep sleep timeout in milliseconds
    uint8_t  checksum;            // Simple checksum for validation
};

class DiceConfigManager {
  public:
    // Constructor
    DiceConfigManager();

    // Initialize LittleFS and load config
    auto begin(const char *configPath = nullptr, bool formatOnFail = true) -> bool;

    // Load configuration from file
    auto load() -> bool;
    auto load(const char *filename) -> bool;

    // Save configuration to file
    auto save() -> bool;
    auto save(const char *filename) -> bool;

    // Reset to default values
    void setDefaults();

    // Validate current configuration
    auto validate() -> bool;

    // Get/Set configuration
    auto getConfig() -> DiceConfig &;
    void setConfig(const DiceConfig &newConfig);

    // Individual field setters (convenience methods)
    void setDiceId(const char *id);
    void setDeviceAMac(const uint8_t *mac);
    void setDeviceB1Mac(const uint8_t *mac);
    void setDeviceB2Mac(const uint8_t *mac);
    void setRssiLimit(int8_t limit);
    void setIsSMD(bool value);
    void setIsNano(bool value);
    void setAlwaysSeven(bool value);

    // Utility functions
    void        printConfig();
    static void printMacAddress(const uint8_t *mac);
    static auto macToString(const uint8_t *mac) -> String;

    // Get last error message
    auto getLastError() -> const char *;

    // Enable/disable verbose logging
    void setVerbose(bool enabled);

    // Get the currently loaded config filename
    auto getConfigPath() -> const char *;

  private:
    DiceConfig _config;
    char       _configPath[64];
    char       _lastError[128];
    bool       _verbose;

    // Auto-detection helper
    auto findConfigFile(char *foundPath, size_t maxLen) -> bool;

    // Internal parsing functions
    static auto parseMacAddress(const char *str, uint8_t *mac) -> bool;
    static auto parseBool(const char *str) -> bool;
    static void trim(char *str);
    static void calculateChecksum(DiceConfig &config);
    static auto validateChecksum(const DiceConfig &config) -> bool;
    void        setError(const char *error);

    // Default configuration values
    void initDefaultConfig();
};

// ============================================================================
// GLOBAL CONFIG ACCESS (EEPROM-style pattern)
// ============================================================================
// Global config instance - accessible from all libraries
extern DiceConfig currentConfig;

// Helper functions for global config management
auto loadGlobalConfig(bool verbose = false) -> bool;
void printGlobalConfig();
auto saveGlobalConfig() -> bool;

// Auto-initialization function (formats LittleFS and creates default config if needed)
auto ensureLittleFSAndConfig(bool verbose = false) -> bool;

// Validation functions
auto isValidMacAddress(const uint8_t *mac) -> bool;
auto hasValidMacAddresses() -> bool;
auto isInSetupMode() -> bool;

// Internal helper (exposed for advanced use)
auto createDefaultConfigFile(const char *filename, bool verbose = false) -> bool;

// ============================================================================
// SAFE MODE BOOT FUNCTIONS
// ============================================================================
// Set minimal hardware defaults (isNano, isSMD only)
void setHardwareDefaults();

// Enter safe mode and wait for config upload
void enterSetupMode();

#endif // DICE_CONFIG_MANAGER_H
