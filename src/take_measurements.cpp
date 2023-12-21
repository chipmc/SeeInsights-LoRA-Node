#include "take_measurements.h"

Adafruit_MAX17048 maxlipo;                  // Class instance for MAX17048 battery fuel gauge
Adafruit_SHT31 sht31 = Adafruit_SHT31();    // And the SHT31-D temperature and humidity sensor

// Battery conect information - https://docs.particle.io/reference/device-os/firmware/boron/#batterystate-
const char* batteryContext[7] = {"Unknown","Not Charging","Charging","Charged","Discharging","Fault","Diconnected"};

take_measurements *take_measurements::_instance;

// [static]
take_measurements &take_measurements::instance() {
    if (!_instance) {
        _instance = new take_measurements();
    }
    return *_instance;
}

take_measurements::take_measurements() {
}

take_measurements::~take_measurements() {
}

void take_measurements::setup() {
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Log.infoln("SHT31 initialization failed");
    // Likely need to do some error handling here
  }
  else Log.infoln("SHT31-D initializated with i2c address 0x44");

  if (!maxlipo.begin()) {
    Log.infoln("MAX17048 initialization failed!");
  }
  else Log.infoln("MAX17048 initialized with i2c address 0x%d", maxlipo.getChipID());

  if (TofSensor::instance().setup()) {
    Log.infoln("VL53L1X initialized");
  }
  else Log.infoln("VL53L1X initialization failed");

  PeopleCounter::instance().setup();

  delay (1000);         // Give the MAX17048 time to initialize (it takes 1 second) - does not work without this 
  // TODO:: test to see if this can be shorter
}

bool take_measurements::loop() {
    if (TofSensor::instance().loop()) {             // If there is new data from the sensor ...
      return PeopleCounter::instance().loop();                 // ... then check to see if we need to update the counts.
    }
    return false;
}

bool take_measurements::recalibrate() {
   if(TofSensor::instance().recalibrate()){
      return true;
   }
   else return false;
}

bool take_measurements::takeMeasurements() { 
    bool returnResult = false;
      
      if (!take_measurements::getTemperatureHumidity()) returnResult = false;  // Temperature and humidity inside the enclosure
      if (!take_measurements::batteryState()) returnResult = false;// Using the Fuel guage
      if (!take_measurements::isItSafeToCharge()) returnResult = false; // This will be a safety check - Thermister on charge controller should manage
      currentData.currentDataChanged = true; // This is a flag that will be used to indicate that the data has changed and needs to be saved
    return returnResult;

}


bool take_measurements::getTemperatureHumidity() { 
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (isnan(t)) { 
    Log.infoln("Failed to read temperature");
    return false;
  }
  
  if (isnan(h)) {  // check if 'is not a number'
    Log.infoln("Failed to read humidity");
    return false;
  }

  current.internalTempC = t;
  current.internalHumidity = h;
  Log.infoln("Temperature is %FC and Humidity is %F%%",t,h);
  return true;

}


bool take_measurements::batteryState() {
  float voltage = maxlipo.cellVoltage();
  float percent = maxlipo.cellPercent();

  if (isnan(voltage)) {
    Log.infoln("Failed to get battery voltage");
    return false;
  }

  if ((isnan(percent))) {
    Log.infoln("Failed to get battery percent charge");
  }

  current.stateOfCharge = percent;
  Log.infoln("Batt Voltage: %FV and %F%% charged ", voltage, percent);
  return true;
}


bool take_measurements::isItSafeToCharge()                             // Returns a true or false if the battery is in a safe charging range.
{
  // Need to see if we even need this function
  return true;
}
