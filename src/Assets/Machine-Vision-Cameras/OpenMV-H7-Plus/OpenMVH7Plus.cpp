#include "OpenMVH7Plus.h"

// Declare instance as null
OpenMVH7Plus* OpenMVH7Plus::_instance = nullptr;

// Constructor
OpenMVH7Plus::OpenMVH7Plus() {}

// Destructor
OpenMVH7Plus::~OpenMVH7Plus() {}

// Singleton pattern [static]
OpenMVH7Plus& OpenMVH7Plus::instance() {
    if (!_instance) {
        _instance = new OpenMVH7Plus();
    }
    return *_instance;
}

bool OpenMVH7Plus::setup() {
    SerialConnection::instance().initialize();
    return true; 
}

bool OpenMVH7Plus::readData() {
    char response[32] = {0};  // Initialize the array with zeros for safety.
    if (SerialConnection::instance().receiveMessage(response, sizeof(response) - 1)) {
        response[sizeof(response) - 1] = '\0'; // Ensure null termination
        String str = response;
        long value = str.toInt();  // Check if within range
        if (value >= INT16_MIN && value <= INT16_MAX) {
            current.occupancyNet = static_cast<int16_t>(value);
            return true;
        } else {
            // Handle out-of-range values
            Serial.println("Value out of range for occupancyNet - something is wrong with camera");
            return false;
        }
    } 
    return false;
}