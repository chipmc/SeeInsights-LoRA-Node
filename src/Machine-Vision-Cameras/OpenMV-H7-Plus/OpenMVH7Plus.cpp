#include "OpenMVH7Plus.h"

// [static]
OpenMVH7Plus* OpenMVH7Plus::_instance = nullptr;

OpenMVH7Plus& OpenMVH7Plus::instance() {
    if (!_instance) {
        _instance = new OpenMVH7Plus();
    }
    return *_instance;
}

OpenMVH7Plus::OpenMVH7Plus() {}

OpenMVH7Plus::~OpenMVH7Plus() {}

bool OpenMVH7Plus::setup() {
    // Implementation of setup
    return true; 
}

bool OpenMVH7Plus::capture() {
    // Implementation of capture
    return true; 
}