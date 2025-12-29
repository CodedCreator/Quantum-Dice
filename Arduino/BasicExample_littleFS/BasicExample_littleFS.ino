/*
 * DiceConfigManager - Basic Example
 * 
 * This example demonstrates how to:
 * 1. Initialize the config manager with auto-detection
 * 2. Load configuration from LittleFS (finds *_config.txt automatically)
 * 3. Access configuration values
 * 4. Modify configuration
 * 5. Save configuration back to LittleFS
 * 
 * AUTO-DETECTION FEATURE:
 * - Upload a file named like: TEST1_config.txt, BART1_config.txt, etc.
 * - Library automatically finds and loads it
 * - If multiple *_config.txt files exist, it will use defaults
 * - If zero *_config.txt files exist, it will use defaults
 * 
 * Hardware: ESP32-S3 or any ESP32 with LittleFS support
 */

#include <DiceConfigManager.h>

// Create config manager instance
DiceConfigManager configManager;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== DiceConfigManager Example ===\n");
  
  // Enable verbose output for debugging
  configManager.setVerbose(true);
  
  // Initialize with auto-detection (no path specified)
  // This will search for any file matching *_config.txt pattern
  // Examples: TEST1_config.txt, BART1_config.txt, MYDICE_config.txt
  if (!configManager.begin()) {
    Serial.println("Failed to initialize config manager!");
    return;
  }
  
  // Show which config file was loaded
  Serial.printf("Loaded config from: %s\n\n", configManager.getConfigPath());
  
  Serial.println("\n--- Current Configuration ---");
  configManager.printConfig();
  
  // Example: Access configuration values
  DiceConfig& config = configManager.getConfig();
  Serial.println("\n--- Accessing Individual Values ---");
  Serial.printf("Dice ID: %s\n", config.diceId);
  Serial.printf("RSSI Limit: %d dBm\n", config.rssiLimit);
  Serial.printf("Is SMD: %s\n", config.isSMD ? "Yes" : "No");
  Serial.printf("Deep Sleep Timeout: %u ms\n", config.deepSleepTimeout);
 /* 
  // Example: Modify configuration
  Serial.println("\n--- Modifying Configuration ---");
  configManager.setDiceId("ESP32-001");
  config.rssiLimit = -65;
  config.alwaysSeven = false;
  config.deepSleepTimeout = 600000; // 10 minutes
  
  Serial.println("Configuration modified!");
  
  // Validate configuration
  if (configManager.validate()) {
    Serial.println("Configuration is valid");
  } else {
    Serial.println("Configuration validation failed!");
    Serial.println(configManager.getLastError());
  }
  
  // Save modified configuration (saves back to auto-detected file)
  Serial.println("\n--- Saving Configuration ---");
  if (configManager.save()) {
    Serial.println("Configuration saved successfully!");
  } else {
    Serial.println("Failed to save configuration");
    Serial.println(configManager.getLastError());
  }
 */ 
  // Print final configuration
  Serial.println("\n--- Final Configuration ---");
  configManager.printConfig();
  
  Serial.println("\n=== Example Complete ===");
  Serial.println("\nNOTE: To use auto-detection:");
  Serial.println("  1. Upload a file named like: SETNAME_config.txt");
  Serial.println("  2. Make sure only ONE *_config.txt file exists");
  Serial.println("  3. The library will find and load it automatically");
}

void loop() {
  // Nothing to do here
  delay(1000);
}
