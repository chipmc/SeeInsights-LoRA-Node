#include "sensors.h"

Adafruit_MAX17048 maxlipo;                  // Class instance for MAX17048 battery fuel gauge
Adafruit_SHT31 sht31 = Adafruit_SHT31();    // And the SHT31-D temperature and humidity sensor

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

  Log.traceln("");
  Log.traceln("************************* SENSORS **************************");

  Log.infoln("MAX17048 Initialization");
  if (!maxlipo.begin()) {
    Log.infoln("MAX17048 initialization failed!");
  }
  else Log.infoln("MAX17048 initialized with i2c address 0x%d", maxlipo.getChipID());

  delay (1000);         // Give the MAX17048 time to initialize (it takes 1 second) - does not work without this

  Log.infoln("Batt Voltage: %FV ", maxlipo.cellVoltage());
  Log.infoln("Batt Percent: %F%%", maxlipo.cellPercent());

  Log.infoln("SHT31-D Initialization");
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Log.infoln("SHT31 initialization failed");
  }
  else Log.infoln("SHT31-D initializated with i2c address 0x44");

  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (! isnan(t)) {  // check if 'is not a number'
    Log.infoln("Temp = %FC",t);
  } else { 
    Log.infoln("Failed to read temperature");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    Log.infoln("Humidity  = %F%%", h);
  } else { 
    Log.infoln("Failed to read humidity");
  }

  Log.traceln("");
  Log.traceln("************************* SENSORS END**************************");
}

int16_t sensors::readBattery(){
  return (int16_t)maxlipo.cellVoltage();
}

void sensors::loop() {
    // Put your code to run during the application thread loop here
}



 /*******************************************************************************
 * Function Name  : softDelay()
 *******************************************************************************/
/**
 * @brief soft delay let's us process Particle functions and service the sensor interrupts while pausing
 * 
 * @details takes a single unsigned long input in millis
 * 
 */
inline void softDelay(uint32_t t) {
  for (uint32_t ms = millis(); millis() - ms < t; delay(1));  //  safer than a delay()
}
