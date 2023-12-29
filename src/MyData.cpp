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


bool sysStatusData::setup() {
    //The memory specs can be set before begin() to skip the auto-detection delay and write wear
    //24XX02 - 2048 bit / 256 bytes - 1 address byte, 8 byte page size
    //   myMem.setAddressBytes(1);

    myMem.setPageSizeBytes(8);
    myMem.setMemorySizeBytes(256);
    if (myMem.begin() == false)
    {
        Log.infoln("No memory detected. Freezing.");
        return false;
    }
    Log.infoln("Memory detected!");
    Log.infoln("Mem size in bytes: %i ", myMem.length());

    uint8_t versionNumber;
    myMem.get(0,versionNumber);

    // We will retrieve the system unique ID from the memory - this is something that 
    // is set by the gateway and is unique to each node.
    // The unique ID is made of a combination of two random bytes and two time bytes
    if (myMem.read(1) == 255 || myMem.read(2) == 255) {                            // If the first byte is 255, then the memory has not been initialized
        Log.infoln("This is a virgin node, need to get a unique ID from the gateway");
        sysStatus.uniqueID = 0xFFFFFFFF;  // Four byte number that will signal that we need a unique ID from the gateway
    }
    else {
        myMem.get(1,sysStatus.uniqueID);
    }

    if (versionNumber != STRUCTURES_VERSION) {
        Log.infoln("Structure changed from %i to %i",versionNumber,STRUCTURES_VERSION);
        sysStatusData::initialize();
    }
    else {
        myMem.get(10, sysStatus);
        Log.infoln("System Data retrieved from EEPROM with node number %i, uniqueID %u and magic number %i", sysStatus.nodeNumber, sysStatus.uniqueID, sysStatus.magicNumber);
    }
    // sysStatusData::printSysData();
    return true;
}

void sysStatusData::loop() {
    static unsigned long lastChecked = 0;
    if (millis() - lastChecked > 1000) {                // Check for data changes every ten seconds while awake - remember millis are only when awake
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

    // Initialize the default value if the structure is reinitialized.
    // Be careful doing this, because when MyData is extended to add new fields,
    // the initialize method is not called! This is only called when first
    // initialized.

    Log.infoln("Loading system defaults");              // Letting us know that defaults are being loaded
    sysStatus.nodeNumber = 255;
    sysStatus.structuresVersion = STRUCTURES_VERSION;
    sysStatus.magicNumber = 27617;
    sysStatus.firmwareRelease = 255;                   // This value is set in the main program
    sysStatus.resetCount = 0;
    sysStatus.nextConnection = 0;
    sysStatus.alertCodeNode=1;

    Log.infoln("Saving new system values, node number %i, uniqueID %u and magic number %i", sysStatus.nodeNumber, sysStatus.uniqueID, sysStatus.magicNumber);
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
    Log.infoln("Device ID: %u", sysStatus.uniqueID);
    Log.infoln("Structures Version: %d", sysStatus.structuresVersion);
    Log.infoln("Firmware Release: %d", sysStatus.firmwareRelease);
    Log.infoln("Reset Count: %d", sysStatus.resetCount);
    Log.infoln("Next connection: %d", sysStatus.nextConnection);
    Log.infoln("Alert Code Node: %d", sysStatus.alertCodeNode);
    Log.infoln("sensorType: %d", sysStatus.sensorType);
}

void sysStatusData::updateUniqueID() {
    myMem.put(1,sysStatus.uniqueID);
    Log.infoln("UniqueID updated to %u and stored in protected space", sysStatus.uniqueID);
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
    static unsigned long lastChecked = 0;
    if (millis() - lastChecked > 1000) {                // Check for data changes every ten seconds while awake - remember millis are only when awake
        lastChecked = millis();

       if (currentStatusData::currentDataChanged) {
           currentStatusData::storeCurrentData();
           currentStatusData::currentDataChanged = false;
       }
    }
}

void currentStatusData::resetEverything() {            // The device is waking up in a new day or is a new install

  current.occupancyState = 0;
  sysStatus.resetCount = 0;                                          // Reset the reset count as well
  current.occupancyGross = 0;                                            // Reset the counts in FRAM as well
  current.occupancyNet = 0;

  currentData.storeCurrentData();
}

bool currentStatusData::validate(size_t dataSize) {
    bool valid = true;
    if (valid) {
        if (current.occupancyNet > current.occupancyGross) {
            Log.infoln("data not valid occupancy net =%d", current.occupancyNet);
            valid = false;
        }
    }
    Log.infoln("current data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void currentStatusData::initialize() {

    myMem.get(90,current);
    if (current.occupancyNet > current.occupancyGross) {
        Log.infoln("Current values not right - resetting");
        currentStatusData::resetEverything();
    }
    Log.infoln("Loading current values, occupancy gross %i and occupancy net %i", current.occupancyGross, current.occupancyNet);

}

void currentStatusData::storeCurrentData() {
    Log.infoln("Storing current data to EEPROM");
    currentStatusData::currentDataChanged = false;
    myMem.put(90,current);
}

void currentStatusData::printCurrentData() {
    Log.infoln("Current Data");
    Log.infoln("OccupancyChange: %i", current.occupancyGross);
    Log.infoln("Occupancy: %i", current.occupancyNet);
    Log.infoln("Sensor Placement: %s", (sysStatus.placement) ? "Inside" : "Outside");
    Log.infoln("Multiple Entrances: %s", (sysStatus.multi) ? "Yes" : "No");
}


