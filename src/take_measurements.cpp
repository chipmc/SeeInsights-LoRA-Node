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

  delay (1000);         // Give the MAX17048 time to initialize (it takes 1 second) - does not work without this 
  // Shoudl test to see if this can be shorter
}

void take_measurements::loop() {
  // Put loop code here
}

bool take_measurements::takeMeasurements() { 

    // Temperature and humidity inside the enclosure
    take_measurements::getTemperatureHumidity();

    take_measurements::batteryState();

    take_measurements::isItSafeToCharge();

    return 1;

}


bool take_measurements::getTemperatureHumidity() { 
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (! isnan(t)) { 
    Log.infoln("Failed to read temperature");
    return false;
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
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

  if (! isnan(voltage)) {
    Log.infoln("Failed to get battery voltage");
    return false;
  }

  if ((! isnan(percent))) {
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


bool recordCount() // This is where we check to see if an interrupt is set when not asleep or act on a tap that woke the device
{
  LED.on();                                                                            // Turn on the blue LED

  current.lastSampleTime = tm.getTime();
  current.hourlyCount = current.hourlyCount +1;                                              // Increment the PersonCount
  current.dailyCount = current.dailyCount +1;                                               // Increment the PersonCount
  Log.info("Count, hourly: %i. daily: %i",current.hourlyCount,current.dailyCount);
  
  delay(500);
  LED.off();

  return true;
}