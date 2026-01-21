#include <LittleFS.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   LittleFS Config File Test Sketch     ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // ═══════════════════════════════════════════════════════════════════
  // Step 1: Mount LittleFS (format if mount fails)
  // ═══════════════════════════════════════════════════════════════════
  Serial.println("Step 1: Mounting LittleFS...");
  
  if (!LittleFS.begin(false)) {
    Serial.println("✗ Mount failed - formatting filesystem...");
    
    if (LittleFS.begin(true)) {  // true = format if needed
      Serial.println("✓ LittleFS formatted and mounted successfully!");
    } else {
      Serial.println("✗ CRITICAL: Format failed!");
      Serial.println("Device may have hardware issue.");
      while(1) delay(1000);  // Halt
    }
  } else {
    Serial.println("✓ LittleFS mounted successfully!");
  }
  
  // Show filesystem info
  Serial.printf("Total space: %u bytes\n", LittleFS.totalBytes());
  Serial.printf("Used space:  %u bytes\n", LittleFS.usedBytes());
  Serial.printf("Free space:  %u bytes\n\n", LittleFS.totalBytes() - LittleFS.usedBytes());
  
  // ═══════════════════════════════════════════════════════════════════
  // Step 2: Check for config files
  // ═══════════════════════════════════════════════════════════════════
  Serial.println("Step 2: Searching for *_config.txt files...");
  
  File root = LittleFS.open("/");
  if (!root) {
    Serial.println("✗ Failed to open root directory!");
    while(1) delay(1000);
  }
  
  if (!root.isDirectory()) {
    Serial.println("✗ Root is not a directory!");
    while(1) delay(1000);
  }
  
  // List all files and find config files
  int configCount = 0;
  String configFiles[10];  // Support up to 10 config files
  
  Serial.println("\n--- Files in LittleFS ---");
  File file = root.openNextFile();
  while (file) {
    String filename = String(file.name());
    Serial.printf("  %s (%u bytes)\n", filename.c_str(), file.size());
    
    // Check if this is a config file
    if (filename.endsWith("_config.txt")) {
      if (configCount < 10) {
        configFiles[configCount] = filename;
        configCount++;
      }
    }
    
    file.close();
    file = root.openNextFile();
  }
  root.close();
  Serial.println("-------------------------\n");
  
  // ═══════════════════════════════════════════════════════════════════
  // Step 3: Report config file status
  // ═══════════════════════════════════════════════════════════════════
  Serial.println("Step 3: Config file check results:");
  Serial.println();
  
  if (configCount == 0) {
    Serial.println("⚠️  NO CONFIG FILES FOUND");
    Serial.println("Status: No *_config.txt files detected");
    Serial.println("Action: Upload a config file named YOURNAME_config.txt");
  } 
  else if (configCount == 1) {
    Serial.println("✓ ONE CONFIG FILE FOUND");
    Serial.printf("File: %s\n", configFiles[0].c_str());
    Serial.println("Status: OK - Device will use this config");
  } 
  else {
    Serial.println("⚠️  MULTIPLE CONFIG FILES FOUND");
    Serial.printf("Count: %d files\n", configCount);
    Serial.println("\nConfig files detected:");
    for (int i = 0; i < configCount; i++) {
      Serial.printf("  %d. %s\n", i+1, configFiles[i].c_str());
    }
    Serial.println("\nStatus: AMBIGUOUS - Device may use defaults");
    Serial.println("Action: Delete extra config files, keep only one");
  }
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║          Test Complete                 ║");
  Serial.println("╚════════════════════════════════════════╝");
}

void loop() {
  // Nothing to do
  delay(1000);
}

/*
---

## 📋 **What This Sketch Does:**

### **Step 1: Mount LittleFS**
- Tries to mount with `LittleFS.begin(false)`
- If fails → formats with `LittleFS.begin(true)`
- Shows total/used/free space

### **Step 2: List All Files**
- Opens root directory
- Lists every file with size
- Identifies files ending with `_config.txt`

### **Step 3: Report Status**
- **No config files** → Warning message
- **One config file** → OK message with filename
- **Multiple config files** → Warning with list

---

## 📊 **Example Output:**

### **Scenario: First Boot (Empty Filesystem)**
```
╔════════════════════════════════════════╗
║   LittleFS Config File Test Sketch    ║
╚════════════════════════════════════════╝

Step 1: Mounting LittleFS...
✗ Mount failed - formatting filesystem...
✓ LittleFS formatted and mounted successfully!
Total space: 10420224 bytes
Used space:  0 bytes
Free space:  10420224 bytes

Step 2: Searching for *_config.txt files...

--- Files in LittleFS ---
-------------------------

Step 3: Config file check results:

⚠️  NO CONFIG FILES FOUND
Status: No *_config.txt files detected
Action: Upload a config file named YOURNAME_config.txt

╔════════════════════════════════════════╗
║          Test Complete                 ║
╚════════════════════════════════════════╝
```

### **Scenario: One Config File**
```
Step 1: Mounting LittleFS...
✓ LittleFS mounted successfully!
Total space: 10420224 bytes
Used space:  2048 bytes
Free space:  10418176 bytes

Step 2: Searching for *_config.txt files...

--- Files in LittleFS ---
  /DICE01_config.txt (856 bytes)
-------------------------

Step 3: Config file check results:

✓ ONE CONFIG FILE FOUND
File: /DICE01_config.txt
Status: OK - Device will use this config

╔════════════════════════════════════════╗
║          Test Complete                 ║
╚════════════════════════════════════════╝
```

### **Scenario: Multiple Config Files**
```
Step 1: Mounting LittleFS...
✓ LittleFS mounted successfully!
Total space: 10420224 bytes
Used space:  4096 bytes
Free space:  10416128 bytes

Step 2: Searching for *_config.txt files...

--- Files in LittleFS ---
  /DICE01_config.txt (856 bytes)
  /DICE02_config.txt (856 bytes)
  /TEST_config.txt (856 bytes)
-------------------------

Step 3: Config file check results:

⚠️  MULTIPLE CONFIG FILES FOUND
Count: 3 files

Config files detected:
  1. /DICE01_config.txt
  2. /DICE02_config.txt
  3. /TEST_config.txt

Status: AMBIGUOUS - Device may use defaults
Action: Delete extra config files, keep only one

╔════════════════════════════════════════╗
║          Test Complete                 ║
╚════════════════════════════════════════╝

*/