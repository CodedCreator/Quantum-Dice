# Quantum Dice Init Tool - User Manual

## Overview

This tool provides a comprehensive setup interface for initializing ESP32-S3 based Quantum Dice devices. It handles MAC address retrievaland device configuration storage.

Once uploaded open Serial monitor

---

## Menu Items Reference

### Main Menu Options

| Option | Function | Purpose | Permanent? |
|--------|----------|---------|------------|
| **1** | Get MAC Address | Retrieves and formats the device's unique MAC address for configuration | No |
| **2** | Configure EEPROM Settings | Interactive device configuration setup and storage | No (can reconfigure) |
| **3** | Clear EEPROM | Erases all EEPROM data (configuration) | No (requires confirmation) |

| **M** | Show Menu | Redisplays the main menu | No |

---

## First-Time Setup Workflow

Follow this step-by-step guide to initialize a new device from scratch (empty EEPROM).

---

### Step 1: Get MAC Address

**Purpose:** Retrieve the unique MAC address needed for device identification and configuration.

1. Press **`1`** in the main menu
2. Wait for WiFi initialization (1-2 seconds)
3. You'll see output similar to:

```
   --- Getting MAC Address ---
   Original MAC Address: D0:CF:13:36:40:88
```

1. **Copy** the formatted MAC address line
2. **Save it** for use in Step 2 (Configuration)
3. Press **`M`** to return to menu

**Notes:**

- Each device has a unique MAC address
- You'll need this for Device A, B1, and B2 configurations

---

### Step 2: Configure Device Settings in EEPROM

**Purpose:** Set up device-specific parameters including MAC addresses, display colors, and operational settings.

1. Press **`2`** in the main menu
2. For empty EEPROM, you'll see:

```
   ⚠ No valid configuration found in EEPROM
   Let's configure your device!
   
   Creating new configuration...
```

1. **Interactive Field-by-Field Configuration:**

   The tool will prompt for each setting with a default value shown in brackets:

```
   ----------------------------------------
   Dice ID [TEST1]: 
```

- maximum of 5 characters
- **Press ENTER** to accept the default value
- **Type a new value** and press ENTER to change it
- Important fields: Dice ID, Device A MAC, Device B MAC. The remaining fields the defaults are ok
- When using the Quantum Dice for **Teleportation** demonstration, the MAC Address of the 3rd dice is needed at Device B2 MAC.

1. **Configuration Fields:**

   | Field | Description | Example | Notes |
   |-------|-------------|---------|-------|
   | **Dice ID** | Unique identifier (max 5 chars) | `DICE_A` | Alpha-numeric |
   | **Device A MAC** | Primary device MAC address | `D0:CF:13:36:40:88` | From Step 1 |
   | **Device B1 MAC** | Secondary device 1 MAC | `D0:CF:13:33:58:5C` | From Step 1 |
   | **Device B2 MAC** | Secondary device 2 MAC | `DC:DA:0C:21:02:44` | From Step 1 |
   | **X Background Color** | Display color (hex) | `0x0000` | 16-bit RGB565 |
   | **Y Background Color** | Display color (hex) | `0x0000` | 16-bit RGB565 |
   | **Z Background Color** | Display color (hex) | `0x0000` | 16-bit RGB565 |
   | **Entanglement AB1 Color** | Connection color (hex) | `0xFFE0` | Yellow |
   | **Entanglement AB2 Color** | Connection color (hex) | `0x07E0` | Green |
   | **RSSI Limit** | Signal strength threshold for entanglement (dBm) | `-35` | -100 to 0. |
   | **Is NANO board?** | Board type | `N` | Y=NANO, N=DEVKIT |
   | **Is SMD screen?** | Screen type | `Y` | Y=SMD, N=HDR |
   | **Always Seven mode?** | Debug mode | `N` | Y/N |
   | **Random Switch Point** | Randomness threshold (0-100) | `50` | 0-100 |
   | **Tumble Constant** | Minimum amount of rolling | `0.2` | 0.0-10.0 |
   | **Deep Sleep Timeout** | Auto-sleep time (seconds) | `300` | 10-3600 |

2. **MAC Address Entry:**

   Enter MAC addresses in the format: `AA:BB:CC:DD:EE:FF`

   Example:

```
   Device A MAC [D0:CF:13:36:40:88]
   Enter MAC (format: AA:BB:CC:DD:EE:FF) or press ENTER:
   D0:CF:13:36:40:88
   ✓ MAC set to: D0:CF:13:36:40:88
```

1. **Configuration Summary:**

   After entering all fields, review the complete configuration:

```
   ========================================
     Configuration Summary
   ========================================
   
   Dice ID: DICE_A
   Device A MAC:  D0:CF:13:36:40:88
   Device B1 MAC: D0:CF:13:33:58:5C
   ...
   
   Write this configuration to EEPROM?
   Type 'Y' to confirm or any other key to cancel:
```

1. Type **`Y`** to save to EEPROM
2. Verification will run automatically:

```
   ✓ Configuration written to EEPROM successfully!
   Verifying...
   ✓ Verification successful!
```

1. Press **`M`** to return to menu

**Notes:**

- Configuration can be changed anytime (not permanent)
- Existing values will be shown as defaults on subsequent edits
- All fields are validated before writing to EEPROM
- Invalid entries will show a warning and use the previous value

---

## Verification and Testing

After completing all setup steps, verify everything is working:

### Verify Configuration

1. Press **`2`** (Configure EEPROM Settings)
2. Review the displayed configuration
3. Press **`N`** to keep current settings
4. All values should match your intended configuration

---

## Troubleshooting

### Configuration Validation Failed

- Check MAC address format (must be 12 hex digits)
- Verify RSSI limit is between -100 and 0
- Ensure Dice ID is not empty and < 16 characters
- Check numeric ranges for all parameters

### Double-Enter Issues

- Use Arduino Serial Monitor with "Newline" line ending
- Wait for prompt before entering next value
- If a field is skipped, restart configuration

---

## Safety Notes

⚠️ **CRITICAL WARNINGS:**

1. **EEPROM Operations:**
   - Clear EEPROM erases **ALL** data (calibration + config)
   - Always backup important configurations
   - Clearing requires recalibration and reconfiguration

2. **Power During Operations:**
   - Keep device powered during EEPROM writes
   - Do not disconnect during ATECC508a locking
   - Stable USB power required throughout setup

---
