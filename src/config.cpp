#include "config.h"

//Define necassary subclasses used within this singleton class:
JC_EEPROM EEPROM(JC_EEPROM::kbits_2, 1, 8); // Class instance for EEPROM  - device size, number of devices, page size

config *config::_instance;

// [static]
config &config::instance() {
    if (!_instance) {
        _instance = new config();
    }
    return *_instance;
}

config::config() {
}

config::~config() {
}

void config::setup() {
    Log.traceln("PROGRAM: SeeInsights-LoRa-Node");
    if (EEPROM.begin(JC_EEPROM::twiClock100kHz) != 0 ) {
        //there was a problem with EEPROM
        Log.errorln("There was an issue with EEPROM");
    }

    devID = 1; //Initalize DevID to 1. This will be updated with a successful join request. 
    devFeatures = EEPROM.read(20);
    pcbVersion = EEPROM.read(21);
    pcbLiPoFuel = EEPROM.read(23);
    pcbI2CMUX = EEPROM.read(24);
    EEPROM.read(30,encryptkey,16);
    EEPROM.read(50,(uint8_t*)devSerial,10);

    //Read the PPM adjustment from EEPROM. This is used to adjust the RTC based off of bench calibration during device commissioning. 
    PPM_Adj = ((EEPROM.read(71) << 8) | EEPROM.read(70));

    if (digitalRead(gpio.DIP1) == LOW){
    cfg.meshNode = true;
    }
    else{
        cfg.meshNode = false;
    }

    //Log the Device ID, Product ID, and Device Features
    Log.traceln("");
    Log.traceln("************************* CONFIG **************************");

    // Can ad information about the device here
    
    Log.traceln("");
    Log.traceln("************************* CONFIG END**************************");

    //After we read the pins, we can then turn it into Input Pulldown to reduce current leakage
    pinMode(gpio.DIP1, INPUT_PULLDOWN);
    pinMode(gpio.DIP2, INPUT_PULLDOWN);
    pinMode(gpio.DIP3, INPUT_PULLDOWN);
    pinMode(gpio.DIP4, INPUT_PULLDOWN);
    pinMode(gpio.DIP5, INPUT_PULLDOWN);
    pinMode(gpio.DIP6, INPUT_PULLDOWN);
}

void config::loop() {
    // Put your code to run during the application thread loop here
}
