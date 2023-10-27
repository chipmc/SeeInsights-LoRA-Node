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
  pinMode(STATUS,OUTPUT);
  pinMode(RFM95_CS,OUTPUT);
  pinMode(RFM95_DIO0,INPUT_PULLUP);
  pinMode(RFM95_RST, OUTPUT);
  pinMode(EN,OUTPUT);
  digitalWrite(EN,LOW);
  pinMode(LED_PWR,OUTPUT);
  digitalWrite(LED_PWR,LOW);
  pinMode(INT,INPUT);
  pinMode(WAKE, INPUT_PULLUP);
  pinMode(I2C_INT,INPUT);
  pinMode(I2C_EN,OUTPUT);              // Not sure if we can use this - messes with Boron i2c bus
  digitalWrite(I2C_EN, LOW);           // Turns on the module
  
  //Establish a random seed based on the tank level pin
  randomSeed(analogRead(Test1));
}

void pinout::loop() {
    // Put your code to run during the application thread loop here
}
