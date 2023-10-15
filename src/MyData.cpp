#include "MyData.h"

//Define necassary subclasses used within this singleton class:
JC_EEPROM EEPROM(JC_EEPROM::kbits_2, 1, 8); // Class instance for EEPROM  - device size, number of devices, page size


// *******************  SysStatus Storage Object **********************
//
// ********************************************************************

sysStatusData *sysStatusData::_instance;

uint8_t writeStatus = 0;

// [static]
sysStatusData &sysStatusData::instance() {
    if (!_instance) {
        _instance = new sysStatusData();
    }
    return *_instance;
}

sysStatusData::sysStatusData() {   
}

sysStatusData::~sysStatusData() {
}

void sysStatusData::setup() {
    Log.traceln("Initializing EEPROM");
    if (EEPROM.begin(JC_EEPROM::twiClock100kHz) != 0 ) {
        Log.errorln("There was an issue with EEPROM");
    }
    sysStatus.structuresVersion = EEPROM.read(0);
    if (sysStatus.structuresVersion != STRUCTURES_VERSION) {
        Log.infoln("Invalid structure number %D - resetting system values", sysStatus.structuresVersion);
        sysStatusData::initialize();
    }
    else {
        Log.traceln("Valid structure number, loading from EEPROM");
        writeStatus = EEPROM.read(10,reinterpret_cast<uint8_t*>(&sysStatus),sizeof(sysStatus));
        if (writeStatus != 0) Log.infoln("EEPROM write error %d",writeStatus);
        Log.infoln("System valies loaded, node number %D and magic number %D reporing every %D minutes", sysStatus.nodeNumber, sysStatus.magicNumber, sysStatus.frequencyMinutes);
        // Likely need to add an error handler here if this happens often
    }

    Log.infoln("Quick test - storing 4 in slot 01");
    EEPROM.write(1,4);
    Log.infoln("The value retrieved is %D",EEPROM.read(01));
    
}

void sysStatusData::loop() {
 // Loop code here
}

bool sysStatusData::validate(size_t dataSize) {
    bool valid = true;

/*
        if (sysStatus.get_frequencyMinutes() <=0 || sysStatus.get_frequencyMinutes() > 60) {
            Log.info("data not valid frequency minutes =%d", sysStatus.get_frequencyMinutes());
            valid = false;
        }
        else if (sysStatus.get_nodeNumber() < 1 || sysStatus.get_nodeNumber() > 11) {
            Log.info("data not valid node number =%d", sysStatus.get_nodeNumber());
            valid = false;
        }

        */
    Log.info("sysStatus data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void sysStatusData::initialize() {
 
    Log.infoln("data initialized");

    // Initialize the default value to 10 if the structure is reinitialized.
    // Be careful doing this, because when MyData is extended to add new fields,
    // the initialize method is not called! This is only called when first
    // initialized.

    Log.infoln("Loading system defaults");              // Letting us know that defaults are being loaded
    sysStatus.nodeNumber = 11;
    sysStatus.structuresVersion = STRUCTURES_VERSION;
    sysStatus.magicNumber = 27617;
    sysStatus.firmwareRelease = 255;                   // This value is set in the main program
    sysStatus.resetCount = 0;
    sysStatus.frequencyMinutes = 60;
    sysStatus.alertCodeNode=1;
    sysStatus.alertTimestampNode = 0;
    sysStatus.openHours = true;

    Log.infoln("Saving new system values, node number %D and magic number %D reporing every %D minutes", sysStatus.nodeNumber, sysStatus.magicNumber, sysStatus.frequencyMinutes);
    writeStatus = EEPROM.write(10,reinterpret_cast<uint8_t*>(&sysStatus),sizeof(sysStatus));
    if (writeStatus != 0) Log.infoln("EEPROM write error %d",writeStatus);
    // Likely need to add an error handler here if this happens often

}

void sysStatusData::storeSysData() {

}

// *****************  Current Status Storage Object *******************
// Offset of 100 bytes - make room for SysStatus
// ********************************************************************

currentStatusData *currentStatusData::_instance;

// [static]
currentStatusData &currentStatusData::instance() {
    if (!_instance) {
        _instance = new currentStatusData();
    }
    return *_instance;
}

currentStatusData::currentStatusData() {

}

currentStatusData::~currentStatusData() {
}

void currentStatusData::setup() {
  // Setup code here
}

void currentStatusData::loop() {
  // Loop code here
}

void currentStatusData::resetEverything() {                             // The device is waking up in a new day or is a new install

  current.messageCount = 0;
  current.successCount = 0;
  sysStatus.resetCount = 0;                                          // Reset the reset count as well
  current.dailyCount = 0;                                            // Reset the counts in FRAM as well
  current.hourlyCount = 0;
  current.lastSampleTime = 0;

  writeStatus = EEPROM.write(50,reinterpret_cast<uint8_t*>(&current),sizeof(current));
  if (writeStatus != 0) Log.infoln("EEPROM write error %d",writeStatus);
  // Likely need to add an error handler here if this happens often

}

bool currentStatusData::validate(size_t dataSize) {
    bool valid = true;
    if (valid) {
        /*
        if (current.get_hourlyCount() < 0 || current.get_hourlyCount()  > 1024) {
            Log.info("current data not valid hourlyCount=%d" , current.get_hourlyCount());
            valid = false;
        }*/
    }
    Log.info("current data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void currentStatusData::initialize() {
 
    Log.infoln("Loading current values from EEPROM");
    EEPROM.read(50,reinterpret_cast<uint8_t*>(&current),sizeof(current));
    if (current.hourlyCount > current.dailyCount || current.successCount > current.messageCount) {
        Log.infoln("Current values not right - resetting");
        currentStatusData::resetEverything();
    }

}

void storeCurrentData() {

    Log.infoln("Storing current data to EEPROM");
    writeStatus = EEPROM.write(50,reinterpret_cast<uint8_t*>(&current),sizeof(current));
    if (writeStatus != 0) Log.infoln("EEPROM write error %d",writeStatus);
    // Likely need to add an error handler here if this happens often
}


