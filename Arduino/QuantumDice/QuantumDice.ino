#warning "Compile with Pin Numbering By GPIO (legacy)"
#warning "ESP version 3.3.2 ,board esp32/Arduino Nano ESP32 or esp32/ESP32S3 Dev Module

#include "defines.hpp"
#include "ImageLibrary/ImageLibrary.hpp"
#include "ScreenStateDefs.hpp"
#include "IMUhelpers.hpp"
#include "Screenfunctions.hpp"
#include "handyHelpers.hpp"
#include "StateMachine.hpp"
#include "DiceConfigManager.hpp"

StateMachine stateMachine;

#define UPDATE_INTERVAL 50  //loop functions
unsigned long previousMillisWatchDog = 0;

void setup() {
    // Make sure power switch keeps on - do this FIRST before anything else
    pinMode(REGULATOR_PIN, OUTPUT);
    digitalWrite(REGULATOR_PIN, LOW);

    // Initialize serial for debugging
    initSerial();  // delay(1000) included

    // Print version and configuration info
    Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
    Serial.print("FW: ");
    Serial.println(VERSION);

    // Load config
    if (!loadGlobalConfig(true)) {
        Serial.println("Config failed!");
    }
    printGlobalConfig();

    // intitialise the hardware pins and display addresses
    initHardwarePins();

    Serial.print(" - Dice ID: ");
    Serial.println(currentConfig.diceId);  // Use diceId from config
    Serial.print("Board type: ");
    Serial.println(currentConfig.isNano ? "NANO" : "DEVKIT");  // Use config instead of defines
    Serial.print("Connectie type isSMD: ");
    Serial.println(currentConfig.isSMD ? "SMD" : "HDR");  // Use config instead of defines

    // Initialize displays - now uses hwPins from loaded configuration
    initDisplays();

    // Show startup logo during setup
    displayQLab(ALL);


    // Initialize IMU sensor
    // This still works exactly the same
    IMUSensor *imuSensor;
    imuSensor = new BNO055IMUSensor();
    if (!imuSensor->init(true)) {  // Show initialization progress
        Serial.println("Failed to initialize sensor!");
    }
    if (currentConfig.isNano) imuSensor->setAxisRemap(0x06, 0x06);  //to be adapted.
                                                                    //imuSensor->setTumbleThreshold(currentConfig.tumbleConstant);    //0.2

    imuSensor->update();
    imuSensor->resetTumbleDetection();

    usleep(1000);

    welcomeInfo(screenselections::X0);
    voltageIndicator(screenselections::X0);
    displayQRcode(screenselections::X1);

    displayEinstein(screenselections::ZZ);
    displayUTlogo(screenselections::YY);

    usleep(1000);
    

    // Set IMU sensor in state machine
    stateMachine.setImuSensor(imuSensor);

    // Initialize button
    initButton();

    // Initialize the state machine - this sets up ESP-NOW with config MACs
    stateMachine.begin();

    Serial.println("Setup complete!");
    Serial.println("==================================\n");
}

void loop() {
    static unsigned long lastUpdateTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        button.loop();
        stateMachine.update();
        //   refreshScreens();
        //   sendWatchDog(); //sendWatchDog removed. Is called at every onEntry function after the states are set. More efficient.
    }
}
