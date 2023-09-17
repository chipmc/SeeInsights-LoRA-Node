#include "pinout.h"

pinout *pinout::_instance;

// [static]
pinout &pinout::instance() {
    if (!_instance) {
        _instance = new pinout();
    }
    return *_instance;
}

pinout::pinout() {
}

pinout::~pinout() {
}

void pinout::setup() {
  //Setup pins to be ins or outs. 
  pinMode(PB_LT, OUTPUT);
  pinMode(TankPower, OUTPUT);
  pinMode(VacPwr, OUTPUT);
  pinMode(VacSel, OUTPUT);
  pinMode(deepRst, OUTPUT);
  pinMode(Relay1, OUTPUT);
  pinMode(Relay2, OUTPUT);
  pinMode(CE, OUTPUT);
  pinMode(DIP1, INPUT_PULLUP);
  pinMode(DIP2, INPUT_PULLUP);
  pinMode(DIP3, INPUT_PULLUP);
  pinMode(DIP4, INPUT_PULLUP);
  pinMode(DIP5, INPUT_PULLUP);
  pinMode(DIP6, INPUT_PULLUP);
  pinMode(RFM95_CS,OUTPUT);
  pinMode(RFM95_DIO0,INPUT_PULLUP);
  pinMode(RFM95_RST, OUTPUT);
  pinMode(IRQ, INPUT_PULLUP);
  
  //Establish a random seeq based on the tank level pin
  randomSeed(analogRead(TankPin));
}

void pinout::loop() {
    // Put your code to run during the application thread loop here
}
