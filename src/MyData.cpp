#include "MyData.h"

//Define necassary subclasses used within this singleton class:
ExternalEEPROM myMem;

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
    //The memory specs can be set before begin() to skip the auto-detection delay and write wear
    //24XX02 - 2048 bit / 256 bytes - 1 address byte, 8 byte page size
 //   myMem.setAddressBytes(1);

    myMem.setPageSizeBytes(8);
    myMem.setMemorySizeBytes(256);
    if (myMem.begin() == false)
    {
        Log.infoln("No memory detected. Freezing.");
        while (1);
    }
    Log.infoln("Memory detected!");
    Log.infoln("Mem size in bytes: %i ", myMem.length());

    uint8_t versionNumber;
    myMem.get(0,versionNumber);

    if (versionNumber != STRUCTURES_VERSION) {
        Log.infoln("Structure changed from %i to %i",versionNumber,STRUCTURES_VERSION);
        sysStatusData::initialize();
    }
    else {
        myMem.get(10, sysStatus);
     }
    // sysStatusData::printSysData();

}

void sysStatusData::loop() {
    static unsigned long lastChecked = 0;
    if (millis() - lastChecked > 10000) {                // Check for data changes every ten seconds while awake
        lastChecked = millis();

       if (sysStatusData::sysDataChanged) {
           sysStatusData::storeSysData();
           sysStatusData::sysDataChanged = false;
       }
    }
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
    sysStatus.deviceID = UNIQUE_DEVICE_ID;
    sysStatus.structuresVersion = STRUCTURES_VERSION;
    sysStatus.magicNumber = 27617;
    sysStatus.firmwareRelease = 255;                   // This value is set in the main program
    sysStatus.resetCount = 0;
    sysStatus.frequencyMinutes = 1;
    sysStatus.alertCodeNode=1;
    sysStatus.alertTimestampNode = 0;
    sysStatus.openHours = true;

    Log.infoln("Saving new system values, node number %i and magic number %i reporing every %i minutes", sysStatus.nodeNumber, sysStatus.magicNumber, sysStatus.frequencyMinutes);
    myMem.put(0,sysStatus.structuresVersion);
    sysStatusData::storeSysData();

    sysStatusData::printSysData();

}

void sysStatusData::storeSysData() {
    Log.infoln("sysStatus data changed, writing to EEPROM");
    myMem.put(10,sysStatus);
}

void sysStatusData::printSysData() {
    Log.infoln("System Data");
    Log.infoln("Magic Number: %d", sysStatus.magicNumber);
    Log.infoln("Node Number: %d", sysStatus.nodeNumber);
    Log.infoln("Device ID: %d", sysStatus.deviceID);
    Log.infoln("Structures Version: %d", sysStatus.structuresVersion);
    Log.infoln("Firmware Release: %d", sysStatus.firmwareRelease);
    Log.infoln("Reset Count: %d", sysStatus.resetCount);
    Log.infoln("Frequency Minutes: %d", sysStatus.frequencyMinutes);
    Log.infoln("Alert Code Node: %d", sysStatus.alertCodeNode);
    Log.infoln("Alert Timestamp Node: %u", sysStatus.alertTimestampNode);
    Log.infoln("Open Hours: %t", sysStatus.openHours);
    Log.infoln("sensorType: %d", sysStatus.sensorType);
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
  // Assume that the system setup has already run
  currentStatusData::initialize();

  // currentStatusData::printCurrentData();

}

void currentStatusData::loop() {
}

void currentStatusData::resetEverything() {                          // The device is waking up in a new day or is a new install

  current.occupancyState = 0;
  current.messageCount = 0;
  current.successCount = 0;
  sysStatus.resetCount = 0;                                          // Reset the reset count as well
  current.dailyCount = 0;                                            // Reset the counts in FRAM as well
  current.hourlyCount = 0;
  current.lastSampleTime = 0;

  currentData.storeCurrentData();
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
    Log.infoln("current data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void currentStatusData::initialize() {

    myMem.get(90,current);
    if (current.hourlyCount > current.dailyCount || current.successCount > current.messageCount) {
        Log.infoln("Current values not right - resetting");
        currentStatusData::resetEverything();
    }
    Log.infoln("Loading current values, hourly %i and daily %i", current.hourlyCount, current.dailyCount);

}

void currentStatusData::storeCurrentData() {
    Log.infoln("Storing current data to EEPROM");
    currentData.currentDataChanged = false;
    myMem.put(90,current);
}

void currentStatusData::printCurrentData() {
    Log.infoln("Current Data");
    Log.infoln("Hourly Count: %i", current.hourlyCount);
    Log.infoln("Daily Count: %i", current.dailyCount);
    Log.infoln("Last Sample Time: %i", current.lastSampleTime);
    Log.infoln("Message Count: %i", current.messageCount);
    Log.infoln("Success Count: %i", current.successCount);
}


