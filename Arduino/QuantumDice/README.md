# Programming the QuantumDice by UTwente

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Technical Overview](#2-technical-overview)
3. [Arduino IDE Setup](#3-arduino-ide-setup)
   - 3.1 [Install Arduino IDE](#31-install-arduino-ide)
   - 3.2 [Install ESP32 Board Support](#32-install-esp32-board-support)
   - 3.3 [Configure Board Settings](#33-configure-board-settings)
   - 3.4 [Install Required Libraries](#34-install-required-libraries)
4. [First Time Setup](#4-first-time-setup)
   - 4.1 [Initial Upload - Blink Test](#41-initial-upload---blink-test)
   - 4.2 [Board Configuration](#42-board-configuration)
5. [Programming the QuantumDice](#5-programming-the-quantumdice)
   - 5.1 [Prepare for Upload](#51-prepare-for-upload)
   - 5.2 [Upload the Sketch](#52-upload-the-sketch)
   - 5.3 [Verify Operation](#53-verify-operation)

---

## 1. Introduction

This guide provides step-by-step instructions for programming and configuring the QuantumDice hardware. Before beginning, ensure you have a USB-C cable and have installed the Arduino IDE on your computer.

> **⚠️ CRITICAL SAFETY WARNING:** Always disconnect the 4-wire power cable before connecting the USB cable to prevent permanent damage to the board! For easier access to the boot and reset buttons and to avoid stress on the display cables, disconnect both the upper and lower screens from the ProcessorBoard during programming.

---

## 2. Technical Overview

The QuantumDice is built on an **ESP32-S3 N16R8 module** and includes several key components:

**Partitions:**
A custom partition setting is used to include a **littlFS** partition. Therefor the file `partitions.csv` must be added to the sketch folder. See [partitions.csv](partitions.csv)

**Display System:**

- Six round TFT displays with SPI interface
- Each display controlled via individual CS-pin connected to a digital port

**Sensors and Components:**

- **BNO055 IMU sensor**: Measures rotation and position of the dice. I2C protocol.
- ESP32 random generator
- Push button for user input
- Battery voltage monitoring system

**Communication:**

- ESP-NOW protocol enables peer-to-peer communication between paired dice
- Each dice stores the MAC addresses of its paired partners

**Data Storage:**

- Configuration data stored in littleFS
- Configuration file (called XXXXX_config.txt where XXXXX is the Set Name of the dice) must be uploaded to littleFs first.
- See example layout [text](../TEST1_config.txt)

**Reference Documentation:**

- [ESP32-S3 N16R8 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf)
- [BNO055 IMU Datasheet](https://nl.mouser.com/datasheet/3/1046/1/bst-bno055-ds000.pdf)

---

## 3. Arduino IDE Setup

### 3.1 Install Arduino IDE

Download and install the latest version of [Arduino IDE 2.x](https://docs.arduino.cc/software/ide/#ide-v2) for your operating system.

### 3.2 Install ESP32 Board Support

1. Open Arduino IDE
2. Navigate to **File > Preferences**
3. In the "Additional Boards Manager URLs" field, add:

   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

4. Click **OK** to save
5. Go to **Tools > Board > Boards Manager**
6. Search for "ESP32"
7. Install **"esp32 by Espressif Systems"**
8. Wait for installation to complete

> **⚠️ IMPORTANT:** Use ESP32 version 3.3.2 from the Board Manager for compatibility.

### 3.3 Configure Board Settings

Select the following settings from the **Tools** menu:

| Setting | Value |
|---------|-------|
| **Board** | ESP32S3 Dev Module |
| **USB CDC On Boot** | Enabled *(critical for native USB)* |
| **USB Mode** | USB-OTG (TinyUSB) or Hardware CDC and JTAG |
| **Flash Size** | 16MB (128Mb) |
| **PSRAM** | OPI PSRAM |
| **Partition Scheme** | custom |

### 3.4 Install Required Libraries

Install the following libraries via **Tools > Manage Libraries**:

- BNO055 by Adafruit
- Adafruit Unified Sensor
- Adafruit GFX
- Adafruit GC9A01A
- Button2

Your Arduino IDE is now ready for QuantumDice development.

---

## 4. First Time Setup

When connecting a fresh ESP32-S3 module (ESP32-S3N16R8) to Arduino IDE for the first time, you may see continuous reboot messages in the Serial monitor:

```
invalid header: 0xffffffff
invalid header: 0xffffffff
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x7 (TG0WDT_SYS_RST),boot:0x2b (SPI_FAST_FLASH_BOOT)
Saved PC:0x40048839
invalid header: 0xffffffff
...
```

This is **completely normal** for a fresh module from the supplier. Here's what's happening:

- **`invalid header: 0xffffffff`** - The bootloader is searching for valid firmware in flash memory but finding only erased flash (0xFFFFFFFF = all bits set high = empty state)
- **`rst:0x7 (TG0WDT_SYS_RST)`** - Watchdog timer reset triggered because no valid firmware was found to execute
- **Bootloop** - The module continuously resets because there's no program to run

The ESP32-S3 module has never been programmed and needs its first firmware upload.

### 4.1 Initial Upload - Blink Test

When programming an ESP32-S3 for the first time, a specific initialization procedure is required. We'll use the Blink_to_Partitions example for this [Blink with Partitions](../Blink_to_Partitions). This sketch is a copy of the build-in sketch, but now with the custom `partitions.csv` included

> **⚠️ REMINDER:** Disconnect the 4-wire power cable before connecting USB!

#### Step 1: Load the Blink Sketch

1. Go to **File > Examples > 01.Basics > Blink**
2. The Blink sketch will open in a new window
3. Leave the code as-is (default `LED_BUILTIN` is fine)

#### Step 2: Enter Download Mode

For the first upload, manually put the ESP32-S3 into download mode. See attached image for the locations of the **BOOT** and **RESET** button.
![alt text](../../images/bootAndReset.png)

1. **Hold down** the **BOOT** button
2. While holding BOOT, **press and release** the **RESET** button
3. **Release** the **BOOT** button

Your board is now in download mode and ready to receive code.

*Alternative method:* Hold the BOOT button while plugging in the USB cable.

#### Step 3: Select COM Port

1. Go to **Tools > Port**
2. Select the COM port for your ESP32-S3:
   - **Windows:** COM# (e.g., COM3)
   - **Mac:** /dev/cu.usbmodem# or /dev/tty.usbmodem#
   - **Linux:** /dev/ttyACM# or /dev/ttyUSB#

#### Step 4: Upload the Sketch

1. Click the **Upload** button (right arrow icon)
2. Wait for compilation and upload to complete
3. You should see "Connecting..." followed by upload progress
4. Once complete, **press the RESET button** on your board

The Blink sketch is now running (though the LED may not be visible on this board).

**After this initial upload, future uploads will not require the BOOT button procedure.**

### 4.2 Board Configuration

The board configuration is performed with a XXXX_config.txt file. Use enclosed [TEST1_config.txt](../TEST1_config.txt) as a starter.

Fill in the required data (mainly `diceId` and the `deviceA_mac`, `deviceB1_mac` and `deviceB2_mac` from the current ESP32 and the other ESP32 of the set)

Uploading the config file with
[ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/) tool (need Google Chrome to run). Instructions on how to upload see ....

---

## 5. Programming the QuantumDice

### 5.1 Prepare for Upload

> **⚠️ REMINDER:** Always disconnect the 4-wire power cable before connecting USB!

**Before programming:**

1. Remove the top and bottom (blue) cups from the dice
2. Disconnect the 4-wire power cable
3. Optionally disconnect the display FPC cables for easier access to buttons
4. Connect a USB-C cable to the underside of the ProcessorPCB

**Important:** If your board is unconfigured, complete the [Board Configuration](#42-board-configuration) steps first.

### 5.2 Upload the Sketch

1. Download the **QuantumDice.ino** sketch from GitHub
2. Save it to your Arduino default folder
3. Open the sketch in Arduino IDE
4. Connect the ProcessorPCB via USB-C cable
5. Select the correct communication port from **Tools > Port**
6. Click **Upload** to compile and upload the sketch
7. Wait for the upload to complete

### 5.3 Verify Operation

After uploading, open the **Serial Monitor** (baud rate: **115200**) to view debugging information.

**Expected Startup Sequence:**

The Serial Monitor will display:

- Loaded configuration (Dice ID, MAC addresses, colors, timing constants)
- Hardware pin configuration
- Display initialization
- BNO055 sensor detection and calibration status
- ESP-NOW initialization with MAC address
- State machine initialization
- Welcome sequence with logo displays

**Example Output:**

```text
/Users/xxxxxxx/Github/Quantum-Dice-by-UTwente/Arduino/QuantumDice/QuantumDice.ino Dec 29 2025 14:01:07
FW: 1.3.0
LittleFS mounted successfully
No config path specified, searching for *_config.txt...
Found config file: TEST1_config.txt
Auto-detected config file: /TEST1_config.txt
Config loaded successfully
Loaded global config from: /TEST1_config.txt
=== Global Configuration ===
Dice ID: TEST1
Device A MAC: D0:CF:13:36:17:DC
Device B1 MAC: AA:BB:CC:DD:EE:02
Device B2 MAC: DC:DA:0C:21:02:44
X Background: 0x0000 (0)
Y Background: 0x0000 (0)
Z Background: 0x0000 (0)
Entangle AB1 Color: 0xFFE0 (65504)
Entangle AB2 Color: 0x07E0 (2016)
RSSI Limit: -35 dBm
Is SMD: false
Is Nano: false
Always Seven: false
Random Switch Point: 50%
Tumble Constant: 45.00
Deep Sleep Timeout: 300000 ms
Checksum: 0x00
============================
Initializing hardware pins...
Hardware pins initialized successfully!

=== Hardware Pin Configuration ===
Board Type: DEVKIT
Screen Type: HDR

TFT Display Pins:
  CS:  GPIO10
  RST: GPIO48
  DC:  GPIO47

Screen CS Pins:
  Screen 0: GPIO4
  Screen 1: GPIO5
  Screen 2: GPIO6
  Screen 3: GPIO7
  Screen 4: GPIO15
  Screen 5: GPIO16

ADC Pin: GPIO2
==================================

Dice ID: TEST1
Board type: DEVKIT
Initializing displays...
Displays initialized successfully!
Qlab logo on screen: 14
Initializing BNO055... E (3521) i2c.master: I2C transaction unexpected nack detected
E (3522) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
E (3793) i2c.master: I2C transaction unexpected nack detected
E (3793) i2c.master: s_i2c_synchronous_transaction(945): I2C transaction failed
E (3795) i2c.master: i2c_master_transmit_receive(1248): I2C transaction failed
detected.
Applying axis remapping... done.
Applying thumble threshold... done.
Waiting for stable readings... OK (10.12 m/s² after 0 attempts)
Stabilizing baseline... done.
✓ BNO055 initialization complete!
ESP-NOW initialized successfully!
MAC Address is : D0:CF:13:36:17:DC
Self role: ROLE_A
ESP-NOW initialized successfully!
MAC Address is : D0:CF:13:36:17:DC
StateMachine Begin: Calling onEntry for initial state
------------ enter IDLE state -------------
Curent diceState: : DiceStates::SINGLE
Previous diceState: : DiceStates::SINGLE
Last Packet SDelivery Fail
WELCOME function called
Einstein on screen: 2
GODDICE function called
Qlab logo on screen: 4
QLAB_LOGO function called
UTwente logo on screen: 1
UT_LOGO function called
Einstein on screen: 3
GODDICE function called
Einstein on screen: 5
GODDICE function called
Setup complete!
==================================

stateMachine: CLASSIC_STATE
DiceState: DiceStates::CLASSIC


> **Note:** You may see some I2C error messages during initialization. These are typically harmless if the sensors are detected successfully afterward.

**Your QuantumDice is now programmed and ready to use!**

---

## Troubleshooting

**Upload fails:**

- Ensure BOOT button procedure was followed for first-time uploads
- Check USB cable connection and try a different cable
- Verify correct COM port is selected
- Confirm 4-wire power cable is disconnected

**Sensor errors:**

- Check I2C connections
- Verify sensors are properly seated

**Display issues:**

- Ensure FPC cables are properly connected
- Check CS pin assignments in code match your hardware version

---

*For additional support, refer to the QuantumDice GitHub repository or contact the UTwente development team.*
