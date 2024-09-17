// Presence Class
// Author: Chip McClelland
// Date: May 2024
// License: GPL3
// In this class, we look at the occpancy values and determine what the occupancy count should be 
// Note, this code is limited to a single sensor with two zones
// Note, this code assumes that Zone 1 is the inner (relative to room we are measureing occupancy for) and Zone 2 is outer

#include "Config.h"
#include "Presence.h"

Presence *Presence::_instance;

// [static]
Presence &Presence::instance() {
    if (!_instance) {
        _instance = new Presence();
    }
    return *_instance;
}

Presence::Presence() {
}

Presence::~Presence() {
}

void Presence::setup() {
 
}

bool Presence::loop(){
  return true;

}

