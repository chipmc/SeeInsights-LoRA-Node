#include "sensors.h"


sensors *sensors::_instance;

// [static]
sensors &sensors::instance() {
    if (!_instance) {
        _instance = new sensors();
    }
    return *_instance;
}

sensors::sensors() {
}

sensors::~sensors() {
}


void sensors::setup() {
    // Setup code here
}

void sensors::loop() {
    // Put your code to run during the application thread loop here
}

