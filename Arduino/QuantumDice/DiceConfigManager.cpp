/*
 * DiceConfigManager - Implementation
 */

#include "DiceConfigManager.hpp"

#include "defines.hpp"

#include <LittleFS.h>
#include <WiFi.h>


// Constructor
DiceConfigManager::DiceConfigManager() : _verbose(false) {
    _lastError[0] = '\0';
    strcpy((char *)_configPath, "/config.txt");
    initDefaultConfig();
}

// Initialize LittleFS and optionally load config
auto DiceConfigManager::begin(const char *configPath, bool formatOnFail) -> bool {
    if (!LittleFS.begin(formatOnFail)) {
        setError("LittleFS mount failed");
        return false;
    }

    if (_verbose) {
        infoln("LittleFS mounted successfully");
    }

    // If no explicit path provided, try auto-detection
    if (configPath == nullptr) {
        if (_verbose) {
            infoln("No config path specified, searching for *_config.txt...");
        }

        if (findConfigFile((char *)_configPath, sizeof(_configPath))) {
            if (_verbose) {
                infof("Auto-detected config file: %s\n", (char *)_configPath);
            }
        } else {
            // No config file found or multiple found
            if (_verbose) {
                infoln("No unique config file found, using defaults");
            }
            strcpy((char *)_configPath, "/config.txt"); // Set default for save operations
            setDefaults();
            return true; // Not a critical error
        }
    } else {
        // Explicit path provided
        strncpy((char *)_configPath, configPath, sizeof(_configPath) - 1);
        _configPath[sizeof(_configPath) - 1] = '\0';
    }

    // Try to load existing config, otherwise use defaults
    if (!load()) {
        if (_verbose) {
            infoln("Config file not loaded, using defaults");
        }
        setDefaults();
        return true; // Not a critical error
    }

    return true;
}

// Load configuration from file
auto DiceConfigManager::load() -> bool {
    return load((char *)_configPath);
}

auto DiceConfigManager::load(const char *filename) -> bool {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        setError("Failed to open config file");
        return false;
    }

    char line[128];
    int  lineNum = 0;
    bool success = true;

    while (file.available()) {
        int len   = file.readBytesUntil('\n', line, sizeof(line) - 1);
        line[len] = '\0';
        lineNum++;

        // Remove carriage return if present
        if (len > 0 && line[len - 1] == '\r') {
            line[len - 1] = '\0';
        }

        trim((char *)line);

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        // Find the '=' separator
        char *separator = strchr((char *)line, '=');
        if (separator == nullptr) {
            if (_verbose) {
                infof("Line %d: Invalid format (no '=')\n", lineNum);
            }
            continue;
        }

        // Split into key and value
        *separator  = '\0';
        char *key   = (char *)line;
        char *value = separator + 1;

        trim(key);

        // Strip inline comments from value (everything after #)
        char *comment = strchr(value, '#');
        if (comment != nullptr) {
            *comment = '\0'; // Terminate string at comment
        }

        trim(value);

        // Parse based on key
        if (strcmp(key, "diceId") == 0) {
            strncpy((char *)_config.diceId, value, sizeof(_config.diceId) - 1);
            _config.diceId[sizeof(_config.diceId) - 1] = '\0';
        } else if (strcmp(key, "x_background") == 0) {
            _config.x_background = (uint16_t)strtoul(value, nullptr, 0);
        } else if (strcmp(key, "y_background") == 0) {
            _config.y_background = (uint16_t)strtoul(value, nullptr, 0);
        } else if (strcmp(key, "z_background") == 0) {
            _config.z_background = (uint16_t)strtoul(value, nullptr, 0);
        } else if (strcmp(key, "entang_colors") == 0) {
            // Parse comma-separated color values
            _config.entang_colors_count = 0;
            char *token                 = strtok(value, ",");
            while (token != nullptr && _config.entang_colors_count < 8) {
                trim(token);
                _config.entang_colors[_config.entang_colors_count]
                  = (uint16_t)strtoul(token, nullptr, 0);
                _config.entang_colors_count++;
                token = strtok(nullptr, ",");
            }
            if (_verbose && _config.entang_colors_count > 0) {
                infof("Loaded %d entanglement colors\n", _config.entang_colors_count);
            }
        } else if (strcmp(key, "rssiLimit") == 0) {
            _config.rssiLimit = (int8_t)strtol(value, nullptr, 0);
        } else if (strcmp(key, "isSMD") == 0) {
            _config.isSMD = parseBool(value);
        } else if (strcmp(key, "isNano") == 0) {
            _config.isNano = parseBool(value);
        } else if (strcmp(key, "alwaysSeven") == 0) {
            _config.alwaysSeven = parseBool(value);
        } else if (strcmp(key, "randomSwitchPoint") == 0) {
            _config.randomSwitchPoint = (uint8_t)strtol(value, nullptr, 0);
        } else if (strcmp(key, "tumbleConstant") == 0) {
            _config.tumbleConstant = strtod(value, nullptr);
        } else if (strcmp(key, "deepSleepTimeout") == 0) {
            _config.deepSleepTimeout = strtoul(value, nullptr, 0);
        } else if (strcmp(key, "checksum") == 0) {
            _config.checksum = (uint8_t)strtol(value, nullptr, 0);
        } else {
            if (_verbose) {
                infof("Line %d: Unknown key '%s'\n", lineNum, key);
            }
        }
    }

    file.close();

    // Validate checksum if not 0
    if (_config.checksum != 0) {
        if (!validateChecksum(_config)) {
            setError("Checksum validation failed");
            if (_verbose) {
                infoln("Warning: Checksum validation failed!");
            }
            success = false;
        }
    }

    if (_verbose && success) {
        infoln("Config loaded successfully");
    }

    return success;
}

// Save configuration to file
auto DiceConfigManager::save() -> bool {
    return save((char *)_configPath);
}

auto DiceConfigManager::save(const char *filename) -> bool {
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
    file.println();

    // Display Colors
    file.println("# Display Colors (16-bit RGB565 format)");
    file.printf("x_background=%u\n", _config.x_background);
    file.printf("y_background=%u\n", _config.y_background);
    file.printf("z_background=%u\n", _config.z_background);

    // Write entanglement colors as comma-separated list
    file.print("entang_colors=");
    for (uint8_t i = 0; i < _config.entang_colors_count; i++) {
        file.printf("%u", _config.entang_colors[i]);
        if (i < _config.entang_colors_count - 1) {
            file.print(",");
        }
    }
    file.println();
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
        infoln("Config saved successfully");
    }

    return true;
}

// Reset to default values
void DiceConfigManager::setDefaults() {
    initDefaultConfig();
    if (_verbose) {
        infoln("Configuration reset to defaults");
    }
}

// Validate current configuration
auto DiceConfigManager::validate() -> bool {
    bool valid = true;

    // Check dice ID is not empty
    if (strlen((char *)_config.diceId) == 0) {
        if (_verbose) {
            infoln("Validation error: diceId is empty");
        }
        valid = false;
    }

    // Check randomSwitchPoint is in range
    if (_config.randomSwitchPoint > 100) {
        if (_verbose) {
            infoln("Validation error: randomSwitchPoint > 100");
        }
        valid = false;
    }

    // Check tumbleConstant is positive
    if (_config.tumbleConstant <= 0) {
        if (_verbose) {
            infoln("Validation error: tumbleConstant <= 0");
        }
        valid = false;
    }

    // Validate checksum
    if (_config.checksum != 0 && !validateChecksum(_config)) {
        if (_verbose) {
            infoln("Validation error: checksum mismatch");
        }
        valid = false;
    }

    return valid;
}

// Get configuration
auto DiceConfigManager::getConfig() -> DiceConfig & {
    return _config;
}

// Set configuration
void DiceConfigManager::setConfig(const DiceConfig &newConfig) {
    _config = newConfig;
}

// Individual setters
void DiceConfigManager::setDiceId(const char *id) {
    strncpy((char *)_config.diceId, id, sizeof(_config.diceId) - 1);
    _config.diceId[sizeof(_config.diceId) - 1] = '\0';
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
    infoln("=== Dice Configuration ===");
    infof("Dice ID: %s\n", _config.diceId);
    infof("X Background: 0x%04X (%u)\n", _config.x_background, _config.x_background);
    infof("Y Background: 0x%04X (%u)\n", _config.y_background, _config.y_background);
    infof("Z Background: 0x%04X (%u)\n", _config.z_background, _config.z_background);
    infof("Entangle Colors (%d): ", _config.entang_colors_count);
    for (uint8_t i = 0; i < _config.entang_colors_count; i++) {
        infof("0x%04X", _config.entang_colors[i]);
        if (i < _config.entang_colors_count - 1) {
            info(", ");
        }
    }
    infoln("");
    infof("RSSI Limit: %d dBm\n", _config.rssiLimit);
    infof("Is SMD: %s\n", _config.isSMD ? "true" : "false");
    infof("Is Nano: %s\n", _config.isNano ? "true" : "false");
    infof("Always Seven: %s\n", _config.alwaysSeven ? "true" : "false");
    infof("Random Switch Point: %u%%\n", _config.randomSwitchPoint);
    infof("Tumble Constant: %.2f\n", _config.tumbleConstant);
    infof("Deep Sleep Timeout: %u ms\n", _config.deepSleepTimeout);
    infof("Checksum: 0x%02X\n", _config.checksum);
    infoln("==========================");
}

void DiceConfigManager::printMacAddress(const uint8_t *mac) {
    infof("%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

auto DiceConfigManager::macToString(const uint8_t *mac) -> String {
    char buffer[18];
    auto res = snprintf((char *)buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
                        mac[1], mac[2], mac[3], mac[4], mac[5]);
    return {(char *)buffer};
}

auto DiceConfigManager::getLastError() -> const char * {
    return (char *)_lastError;
}

void DiceConfigManager::setVerbose(bool enabled) {
    _verbose = enabled;
}

auto DiceConfigManager::getConfigPath() -> const char * {
    return (char *)_configPath;
}

// Private methods
auto DiceConfigManager::findConfigFile(char *foundPath, size_t maxLen) -> bool {
    File root = LittleFS.open("/");
    if (!root) {
        if (_verbose) {
            infoln("Failed to open root directory");
        }
        return false;
    }

    if (!root.isDirectory()) {
        if (_verbose) {
            infoln("Root is not a directory");
        }
        return false;
    }

    int  matchCount     = 0;
    char firstMatch[64] = {0};

    File file = root.openNextFile();
    while (file) {
        String filename = String(file.name());

        // Check if filename matches pattern: *_config.txt
        if (filename.endsWith("_config.txt")) {
            matchCount++;

            if (_verbose) {
                infof("Found config file: %s\n", filename.c_str());
            }

            // Store first match
            if (matchCount == 1) {
                strncpy((char *)firstMatch, filename.c_str(), sizeof(firstMatch) - 1);
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
            auto res = snprintf(foundPath, maxLen, "/%s", (char *)firstMatch);
        } else {
            strncpy(foundPath, (char *)firstMatch, maxLen - 1);
            foundPath[maxLen - 1] = '\0';
        }
        return true;
    }
    if (matchCount == 0) {
        setError("No *_config.txt file found");
        if (_verbose) {
            infoln("No files matching *_config.txt pattern found");
        }
    } else {
        setError("Multiple *_config.txt files found");
        if (_verbose) {
            infof("Error: Found %d config files. Only one allowed.\n", matchCount);
        }
    }

    return false;
}

auto DiceConfigManager::parseMacAddress(const char *str, uint8_t *mac) -> bool {
    int values[6];
    if (sscanf(str, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4],
               &values[5])
        == 6) {
        for (int i = 0; i < 6; i++) {
            mac[i] = (uint8_t)values[i];
        }
        return true;
    }
    return false;
}

auto DiceConfigManager::parseBool(const char *str) -> bool {
    return strcasecmp(str, "true") == 0 || strcmp(str, "1") == 0;
}

void DiceConfigManager::trim(char *str) {
    // Trim leading space
    char *start = str;
    while (isspace((unsigned char)*start) != 0) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && (isspace((unsigned char)*end) != 0)) {
        end--;
    }
    end[1] = '\0';
}

void DiceConfigManager::calculateChecksum(DiceConfig &config) {
    auto   *ptr = (uint8_t *)&config;
    uint8_t sum = 0;

    // XOR all bytes except the checksum field itself
    for (size_t i = 0; i < sizeof(DiceConfig) - 1; i++) {
        sum ^= ptr[i];
    }

    config.checksum = sum;
}

auto DiceConfigManager::validateChecksum(const DiceConfig &config) -> bool {
    DiceConfig temp             = config;
    uint8_t    originalChecksum = temp.checksum;
    calculateChecksum(temp);
    return temp.checksum == originalChecksum;
}

void DiceConfigManager::setError(const char *error) {
    strncpy((char *)_lastError, error, sizeof(_lastError) - 1);
    _lastError[sizeof(_lastError) - 1] = '\0';
    if (_verbose) {
        infof("Error: %s\n", error);
    }
}

void DiceConfigManager::initDefaultConfig() {
    // Default values
    strcpy((char *)_config.diceId, "DEFAULT");

    // Default colors (RGB565)
    _config.x_background = 0x0; // Black
    _config.y_background = 0x0; // Black
    _config.z_background = 0x0; // Black

    // Default entanglement colors: Yellow, Green, Cyan, Magenta
    _config.entang_colors[0]    = 0xFFE0; // Yellow
    _config.entang_colors[1]    = 0x07E0; // Green
    _config.entang_colors[2]    = 0x07FF; // Cyan
    _config.entang_colors[3]    = 0xF81F; // Magenta
    _config.entang_colors_count = 4;

    // Default RSSI
    _config.rssiLimit = -35;

    // Default hardware config
    _config.isSMD       = true;
    _config.isNano      = false;
    _config.alwaysSeven = false;

    // Default operational parameters
    _config.randomSwitchPoint = 50;
    _config.tumbleConstant    = 45;     //=cos(45)
    _config.deepSleepTimeout  = 300000; // 5 minutes

    // Checksum (will be calculated on save)
    _config.checksum = 0;
}

// ============================================================================
// GLOBAL CONFIG ACCESS IMPLEMENTATION
// ============================================================================

// Define the global config instance
DiceConfig currentConfig;

// Internal static config manager for global access
static DiceConfigManager globalConfigManager;

auto loadGlobalConfig(bool verbose) -> bool {
    globalConfigManager.setVerbose(verbose);

    if (!globalConfigManager.begin()) {
        infoln("Failed to load global config!");
        return false;
    }

    if (verbose) {
        infof("Loaded global config from: %s\n", globalConfigManager.getConfigPath());
    }

    // Copy config to global instance
    currentConfig = globalConfigManager.getConfig();

    return true;
}

void printGlobalConfig() {
    infoln("=== Global Configuration ===");
    infof("Dice ID: %s\n", (char *)currentConfig.diceId);

    infof("X Background: 0x%04X (%u)\n", currentConfig.x_background, currentConfig.x_background);
    infof("Y Background: 0x%04X (%u)\n", currentConfig.y_background, currentConfig.y_background);
    infof("Z Background: 0x%04X (%u)\n", currentConfig.z_background, currentConfig.z_background);
    infof("Entangle Colors (%d): ", currentConfig.entang_colors_count);
    for (uint8_t i = 0; i < currentConfig.entang_colors_count; i++) {
        infof("0x%04X", currentConfig.entang_colors[i]);
        if (i < currentConfig.entang_colors_count - 1) {
            info(", ");
        }
    }
    infoln("");

    infof("RSSI Limit: %d dBm\n", currentConfig.rssiLimit);
    infof("Is SMD: %s\n", currentConfig.isSMD ? "true" : "false");
    infof("Is Nano: %s\n", currentConfig.isNano ? "true" : "false");
    infof("Always Seven: %s\n", currentConfig.alwaysSeven ? "true" : "false");
    infof("Random Switch Point: %u%%\n", currentConfig.randomSwitchPoint);
    infof("Tumble Constant: %.2f\n", currentConfig.tumbleConstant);
    infof("Deep Sleep Timeout: %u ms\n", currentConfig.deepSleepTimeout);
    infof("Checksum: 0x%02X\n", currentConfig.checksum);
    infoln("============================");
}

auto saveGlobalConfig() -> bool {
    // Update the internal config manager with global values
    globalConfigManager.setConfig(currentConfig);

    // Save to file
    if (!globalConfigManager.save()) {
        infoln("Failed to save global config!");
        infoln(globalConfigManager.getLastError());
        return false;
    }

    infoln("Global config saved successfully!");
    return true;
}

// ============================================================================
// AUTO-INITIALIZATION AND VALIDATION FUNCTIONS
// ============================================================================

auto ensureLittleFSAndConfig(bool verbose) -> bool {
    // Step 1: Ensure LittleFS is mounted (format if needed)
    if (verbose) {
        infoln("Mounting LittleFS...");
    }

    if (!LittleFS.begin(true)) { // true = format if mount fails
        infoln("Failed to mount/format LittleFS!");
        return false;
    }

    if (verbose) {
        infoln("LittleFS mounted successfully");
        infof("Total: %u bytes, Used: %u bytes\n", LittleFS.totalBytes(), LittleFS.usedBytes());
    }

    // Step 2: Check if any config file exists
    bool   configExists    = false;
    String foundConfigPath = "";

    File root = LittleFS.open("/");
    if (root && root.isDirectory()) {
        File file = root.openNextFile();
        while (file) {
            String filename = String(file.name());
            if (filename.endsWith("_config.txt") || filename == "/config.txt") {
                configExists    = true;
                foundConfigPath = filename;
                if (verbose) {
                    infof("Found existing config file: %s\n", filename.c_str());
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
            infoln("No config file found, creating default config.txt...");
        }

        if (!createDefaultConfigFile("/config.txt", verbose)) {
            infoln("Failed to create default config!");
            return false;
        }
    }

    // Step 4: Load the config into currentConfig
    return loadGlobalConfig(verbose);
}

auto createDefaultConfigFile(const char *filename, bool verbose) -> bool {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        if (verbose) {
            infof("Failed to create file: %s\n", filename);
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
    file.println("x_background=0");
    file.println("y_background=0");
    file.println("z_background=0");
    file.println("entang_colors=65504,2016,2047,63519");
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
        infof("Created default config file: %s\n", filename);
    }

    return true;
}

auto isInSetupMode() -> bool {
    // Check if using default ID
    return strcmp((char *)currentConfig.diceId, "DEFAULT") == 0;
}

// ============================================================================
// SAFE MODE BOOT FUNCTIONS
// ============================================================================

void setHardwareDefaults() {
    currentConfig.isNano = false; // DEVKIT
    currentConfig.isSMD  = true;  // SMD mounting
}

void enterSetupMode() {
    infoln("\n╔════════════════════════════════════════╗");
    infoln("║     SETUP MODE - CONFIG REQUIRED      ║");
    infoln("╚════════════════════════════════════════╝");
    infoln("");
    infoln("No valid configuration found!");
    infoln("Displays should show: CONFIG MODE");
    infoln("");

    // Get and display MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    infoln("═══════════════════════════════════════");
    infoln("DEVICE INFORMATION:");
    infoln("═══════════════════════════════════════");
    infof("  MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4],
          mac[5]);
    infof("  Hardware: %s\n", currentConfig.isNano ? "NANO" : "DEVKIT");
    infof("  Mounting: %s\n", currentConfig.isSMD ? "SMD" : "HDR");
    infoln("");

    infoln("═══════════════════════════════════════");
    infoln("NEXT STEPS:");
    infoln("═══════════════════════════════════════");
    infoln("");
    infoln("1. STOP the Serial Monitor");
    infoln("   └─> Click STOP button");
    infoln("   └─> Returns device to stub mode");
    infoln("");
    infoln("2. Open 'LittleFS Tools'");
    infoln("   └─> Click in left sidebar");
    infoln("   └─> Filesystem is ready");
    infoln("");
    infoln("3. Upload your config file:");
    infoln("   └─> Name: YOURNAME_config.txt");
    infoln("   └─> Must contain:");
    infoln("       • diceId=YOURNAME");
    infof("       • deviceA_mac=%02X:%02X:%02X:%02X:%02X:%02X (this device or peer)\n", mac[0],
          mac[1], mac[2], mac[3], mac[4], mac[5]);
    infoln("       • deviceB1_mac=XX:XX:XX:XX:XX:XX");
    infoln("       • deviceB2_mac=XX:XX:XX:XX:XX:XX");
    infoln("       • Other settings (see template)");
    infoln("");
    infoln("4. Restart device:");
    infoln("   Option A: START Serial Monitor again");
    infoln("   Option B: DISCONNECT and reconnect");
    infoln("");
    infoln("═══════════════════════════════════════");
    infoln("Waiting for config upload...");
    infoln("═══════════════════════════════════════");

    // Idle loop with periodic reminders
    unsigned long lastReminder  = 0;
    int           reminderCount = 0;

    while (true) {
        // Print reminder every 30 seconds
        if (millis() - lastReminder > 30000) {
            reminderCount++;
            infoln("");
            infof("⏳ Still waiting... (%d minutes)\n", reminderCount / 2);
            infoln("   Remember to STOP Serial Monitor before uploading!");

            lastReminder = millis();
        }

        delay(100);
    }
}
