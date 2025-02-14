#include "take_measurements.h"
#include "Assets/Asset.h"

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

void take_measurements::setup(uint8_t sensorType) {

  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Log.infoln("SHT31 initialization failed");
    // Likely need to do some error handling here
  }

  if (!maxlipo.begin()) {
    Log.infoln("MAX17048 initialization failed!");
  }
  else {                                                          // Iniitalization was successful - now we need to configure the device
    // TODO:: test to see if this can be shorter
    delay (100);                                                  // Give the MAX17048 time to initialize (it takes 1 second) - does not work without this 
    maxlipo.setAlertVoltages(3.7 , 4.2);                          // Set the alert voltages to 3.6V and 4.2V - https://blog.ampow.com/lipo-voltage-chart/

    // Next we need to check to see if the battery alert flag needs to be cleared
    if (digitalRead(gpio.BATTINT) == LOW) {                       // If the interrupt is active low there is an alert (need to determine what the alert is for)
      if (maxlipo.cellVoltage() > 3.7) {
        maxlipo.clearAlertFlag(0x00);                             // If the voltage is above 3.7V then we can clear the alert flag
        current.batteryState = 1;                                 // This is the state where the battery is 10% or more
      }
      else {
        current.batteryState = 0;                                 // This is the state where the battery is less than 10%
      }
    }

    byte activeAlert = maxlipo.getAlertStatus();                  // Get the alert status

    Log.infoln("Battery alert value of %d which is %s and battery interrupt is %s battery voltage at %FV and charge at %F%%", activeAlert, (activeAlert | 0b00000010)? "active" : "not active", (digitalRead(gpio.BATTINT)) ? "HIGH" : "LOW", maxlipo.cellVoltage(), maxlipo.cellPercent());

  }

  if(asset.setup(sensorType)){
    Log.infoln("External asset initialized");

  } else {
    if (TofSensor::instance().setup()) {
      Log.infoln("VL53L1X initialized");
    }
  }

  PeopleCounter::instance().setup();
}

bool take_measurements::loop() {
    if (TofSensor::instance().loop()) {                        // If there is new data from the sensor ...
      return PeopleCounter::instance().loop();                 // ... then check to see if we need to update the counts.
    }
    
    if(sysStatus.sensorType == 13)                             // If we are an Accelerometer sensor...
      if(Asset::instance().readData()) {                        
        return true;
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
    currentStatusData::instance().currentDataChanged = true; // This is a flag that will be used to indicate that the data has changed and needs to be saved
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


bool take_measurements::batteryState() {                              // This function returns false if it cannot get valid readings

  if (current.batteryState == 0) {                                    // In this state, the battery has been measured as less than 10%
    maxlipo.quickStart();
    delay(2000);                                                      // We will give ourselves a couple seconds to let the fuel guage wake fully
  }

  if (digitalRead(gpio.BATTINT) == LOW) {                             // If the interrupt is active low there is an alert (need to determine what the alert is for)
    byte activeAlert = maxlipo.getAlertStatus();                      // Get the alert status
    Log.infoln("Battery alert value of %d which is %s and battery interrupt is %s battery voltage at %FV and charge at %F%%", activeAlert, (activeAlert | 0b00000010)? "active" : "not active", (digitalRead(gpio.BATTINT)) ? "HIGH" : "LOW", maxlipo.cellVoltage(), maxlipo.cellPercent());
    if (maxlipo.cellVoltage() < 3.7) current.batteryState = 0;        // This is the state where the battery is less than 10%
  }
  else {                                                              // If the interrupt high then we are above 3.7V 
    if (maxlipo.cellVoltage() >=3.7) {
      maxlipo.clearAlertFlag(0x00);                                   // Clear all the alert flags
      current.batteryState = 1;                                       // This is the state where the battery is 10% or more
    }
  }

  float voltage = maxlipo.cellVoltage();
  if (voltage > 4.2) {
    Log.infoln("Failed to get valid battery voltage");
    return false;
  }

  float percent = maxlipo.cellPercent();                               // There is no error checking in the Adafruit lib - so, +100% is an invalid result
  if (percent < 0.00 || percent > 101.0) {
    current.batteryState = 2;                                          // This indicates that we did not get a valid state of charge measurement
    Log.infoln("Failed to get battery percent charge - %F%%", percent);
    return false;
  }

  current.stateOfCharge = percent;                                     // This stores the value in 8bits (we don't need the float)
  Log.infoln("Batt Voltage: %FV and %F%% charged ", voltage, percent);
  return true;
}



bool take_measurements::isItSafeToCharge()                             // Returns a true or false if the battery is in a safe charging range.
{
  // Need to see if we even need this function
  return true;
}
