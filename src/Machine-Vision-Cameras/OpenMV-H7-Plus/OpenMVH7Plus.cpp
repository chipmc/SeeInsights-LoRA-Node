#include "OpenMVH7Plus.h"
#include "Utils/Assets/Serial/SerialConnection.h"

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

bool OpenMVH7Plus::readCount() {
    char response[32];
    SerialConnection::instance().receiveMessage(response, sizeof(response));
    return true; 
}