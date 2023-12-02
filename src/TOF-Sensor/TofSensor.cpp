// Time of Flight Sensor Class
// Author: Chip McClelland
// Date: May 2023
// License: GPL3
// This is the class for the ST Micro VL53L1X Time of Flight Sensor
// We are using the Sparkfun library which has some shortcomgings
// - It does not implement distance mode medium
// - It does not give access to the factory calibration of the optical center

#include "ErrorCodes.h"
#include "MyData.h"
#include "TofSensorConfig.h"
#include "PeopleCounterConfig.h"
#include "TofSensor.h"
#include <ArduinoLog.h>     // https://github.com/thijse/Arduino-Log

/** Detection **/
int detectionBaselines[2] = {0, 0};                 // Upper and lower values that define a baseline "range" of signal strength that is considered as "nothing" (Max, Min)
int lastDetectionSignal = 0;                        // The most recent Detection zone Signal

/** Occupancy **/
uint8_t occupancyOpticalCenters[2] = {OCCUPANCY_FRONT_ZONE_CENTER, OCCUPANCY_BACK_ZONE_CENTER}; // Array of optical centers for the Occupancy zones (zone 1 (ones) and zone 2 (twos))
int zoneSignalPerSpad[2][2] = {{0, 0}, {0, 0}};     // Stores the average Signal strength of all SPADs in bursts of 2 readings for each zone to reduce noise (zone 1 (ones) and zone 2 (twos))
int occupancyBaselines[2][2] = {{0, 0}, {0, 0}};    // Upper and lower values that define a baseline "range" of signal strength that is considered as "nothing" ((zone 1(Min, Max)) , (zone 2(Min, Max)))
int lastSignal[2] = {0, 0};                         // The most recent Signal (zone 1 (ones) and zone 2 (twos))
int lastAmbient[2] = {0, 0};                        // The most recent Ambient (zone 1 (ones) and zone 2 (twos))
int occupancyState = 0;                             // The current occupancy state (occupied or not, zone 1 (ones) and zone 2 (twos))
int ready = 0;                                      // "ready" flag

int numberOfRetries = 0;                            // If the device retries three times in a row, we will reset with a sysStatus.alertCodeNode = 3


TofSensor *TofSensor::_instance;

// [static]
TofSensor &TofSensor::instance() {
  if (!_instance) {
      _instance = new TofSensor();
  }
  return *_instance;
}

TofSensor::TofSensor() {
}

TofSensor::~TofSensor() {
}

SFEVL53L1X myTofSensor;

bool TofSensor::setup(){
  if(myTofSensor.begin() != 0){
    if(numberOfRetries == 3){                      // if 3 retries, return false
      sysStatus.alertCodeNode = 3;  
      return false;
    }else {
      sysStatus.alertCodeNode = 3;
    }
    Log.infoln("Sensor did not initialize - retry in 10 seconds");
    delay(10000);
    numberOfRetries++;
  } else {
    Log.infoln("Sensor init successfully");
  }
  
  // Here is where we set the device properties
  myTofSensor.setSigmaThreshold(45);                // Default is 45 - this will make it harder to get a valid result - Range 1 - 16383
  myTofSensor.setSignalThreshold(1500);             // Default is 1500 raising value makes it harder to get a valid results- Range 1-16383
  myTofSensor.setTimingBudgetInMs(20);              // Was 20mSec
  
  Log.infoln("Calibrating TOF Sensor");

  while (TofSensor::measure() == SENSOR_BUFFRER_NOT_FULL) {delay(10);}; // Wait for the buffer to fill up
  
  if (TofSensor::performOccupancyCalibration() && TofSensor::performDetectionCalibration()) Log.infoln("Calibration Complete");
  else {
    Log.infoln("Initial calibration failed - waiting 10 seconds and resetting");
    delay(10000);
    sysStatus.alertCodeNode = 3;                    // Set a reset alert code and return false
    return false;
  }

  // myTofSensor.setDistanceModeShort();            // Once initialized, we are focused on the top half of the door
 
  return true;
}

bool TofSensor::performOccupancyCalibration() {
  if(TofSensor::measure() == SENSOR_TIMEOUT_ERROR){
    return false;
  }
  occupancyBaselines[0][0] = lastSignal[0];     // Assign the first reading as max AND min for each zone
  occupancyBaselines[0][1] = lastSignal[0];
  occupancyBaselines[1][0] = lastSignal[1] + 5;     
  occupancyBaselines[1][1] = lastSignal[1] + 5;
  for (int i=0; i<NUM_OCCUPANCY_CALIBRATION_LOOPS; i++) {                     // Loop through a set number of times ... 
    if(TofSensor::measure() == SENSOR_TIMEOUT_ERROR){
      return false;
    }  
    if(zoneSignalPerSpad[0][0] >= CALIBRATION_SIGNAL_RETRY_THRESHOLD          // ... if any measurements exceed the retry threshold while calibrating, retry calibration
        || zoneSignalPerSpad[1][0] >= CALIBRATION_SIGNAL_RETRY_THRESHOLD 
        || zoneSignalPerSpad[0][1] >= CALIBRATION_SIGNAL_RETRY_THRESHOLD 
        || zoneSignalPerSpad[1][1] >= CALIBRATION_SIGNAL_RETRY_THRESHOLD){
      Log.infoln("Occupancy zone not clear - try again");
      delay(20);
      TofSensor::performOccupancyCalibration();
    }
    if(zoneSignalPerSpad[0][0] < occupancyBaselines[0][0]) occupancyBaselines[0][0] = zoneSignalPerSpad[0][0];  // ... check if any of the readings are less than the minimum baseline for their zone
    if(zoneSignalPerSpad[0][1] < occupancyBaselines[0][0]) occupancyBaselines[0][0] = zoneSignalPerSpad[0][1];
    if(zoneSignalPerSpad[1][0] < occupancyBaselines[1][0]) occupancyBaselines[1][0] = zoneSignalPerSpad[1][0];    
    if(zoneSignalPerSpad[1][1] < occupancyBaselines[1][0]) occupancyBaselines[1][0] = zoneSignalPerSpad[1][1];
    
    if(zoneSignalPerSpad[0][0] > occupancyBaselines[0][1]) occupancyBaselines[0][1] = zoneSignalPerSpad[0][0];  // ... check if any of the readings are greater than the maximum baseline for their zone
    if(zoneSignalPerSpad[0][1] > occupancyBaselines[0][1]) occupancyBaselines[0][1] = zoneSignalPerSpad[0][1];
    if(zoneSignalPerSpad[1][0] > occupancyBaselines[1][1]) occupancyBaselines[1][1] = zoneSignalPerSpad[1][0];
    if(zoneSignalPerSpad[1][1] > occupancyBaselines[1][1]) occupancyBaselines[1][1] = zoneSignalPerSpad[1][1];
  }

  if(occupancyBaselines[0][0] < 5) occupancyBaselines[0][0] = 5;   // Make sure we have a bit of room at the bottom of the range for black hair
  if(occupancyBaselines[1][0] < 5) occupancyBaselines[1][0] = 5;   

  Log.infoln("Target zone is clear with zone1 range at {MIN: %ikcps/SPAD, MAX: %ikcps/SPAD} and zone2 range at {MIN: %ikcps/SPAD, MAX: %ikcps/SPAD}",occupancyBaselines[0][0],occupancyBaselines[0][1],occupancyBaselines[1][0],occupancyBaselines[1][1]);
  return true;
}

bool TofSensor::performDetectionCalibration() {
  TofSensor::detect();
  detectionBaselines[0] = lastDetectionSignal + 8;        // Assign the first reading as max AND min of baseline
  detectionBaselines[1] = lastDetectionSignal + 8; 
  for (int i=0; i<NUM_DETECTION_CALIBRATION_LOOPS; i++) {       // Loop through a set number of times ... 
    if(TofSensor::detect() == SENSOR_TIMEOUT_ERROR){
      return false;
    } 
    if(lastDetectionSignal >= CALIBRATION_SIGNAL_RETRY_THRESHOLD){          // ... if any measurements exceed the retry threshold while calibrating, retry calibration)
      Log.infoln("Detection zone not clear - try again");
      delay(20);
      TofSensor::performDetectionCalibration();
    }
    if(lastDetectionSignal < detectionBaselines[0]) detectionBaselines[0] = lastDetectionSignal;    // ... and update the min and max readings each time.
    if(lastDetectionSignal > detectionBaselines[1]) detectionBaselines[1] = lastDetectionSignal;
  }
  detectionBaselines[0] -= 3;                               
  detectionBaselines[1] += 3;
  if(detectionBaselines[0] < 5) detectionBaselines[0] = 5;   // Make sure we have a bit of room at the bottom of the range for black hair. TODO:: MAYBE MAKE SPADS SMALLER OR USE DISTANCE

  Log.infoln("Detection zone is clear with range {MIN: %ikcps/SPAD, MAX: %ikcps/SPAD} ",detectionBaselines[0],detectionBaselines[1]);
  return true;
}

int TofSensor::loop(){       // This function will update the current detection or occupancy for each zone. Switches modes based on interrupt triggers. Returns error codes also.
  int result = 0;
  if(current.detectionMode){
    result = TofSensor::detect();
    current.detectionState = (lastDetectionSignal > detectionBaselines[1] || lastDetectionSignal < detectionBaselines[0]) ? 1 : 0;
  } else {
    occupancyState = 0;
    result = TofSensor::measure();
    occupancyState += ((zoneSignalPerSpad[0][0] > (occupancyBaselines[0][1]) && zoneSignalPerSpad[0][1] > (occupancyBaselines[0][1]))
                      || (zoneSignalPerSpad[0][0] <= (occupancyBaselines[0][0]) && zoneSignalPerSpad[0][1] <= (occupancyBaselines[0][0]))) ? 1 : 0;
  
    occupancyState += ((zoneSignalPerSpad[1][0] > (occupancyBaselines[1][1]) && zoneSignalPerSpad[1][1] > (occupancyBaselines[1][1]))
                      || (zoneSignalPerSpad[1][0] <= (occupancyBaselines[1][0]) && zoneSignalPerSpad[1][1] <= (occupancyBaselines[1][0]))) ? 2 : 0;
  }
  return result;     // Relay the result of taking our samples.
}

int TofSensor::measure(){
  ready = 0;                                                                           
  unsigned long startedRanging;
  for (int samples = 0; samples < 4; samples++){                                        // Take 4 samples...
    int zone = samples % 2;                                                              // ... 2 for each zone.    
    myTofSensor.stopRanging();
    myTofSensor.clearInterrupt();
    myTofSensor.setROI(OCCUPANCY_ROWS_OF_SPADS,OCCUPANCY_COLUMNS_OF_SPADS,occupancyOpticalCenters[zone]);    // Set the Region of Interest for the two zones
    delay(1);
    myTofSensor.startRanging();
    startedRanging = millis();
    while(!myTofSensor.checkForDataReady()) {                                            // Time out if something is wrong with the sensor
      if (millis() - startedRanging > SENSOR_TIMEOUT) {
        Log.infoln("Sensor timed out - retrying in 10 seconds");
        delay(10000);
        return SENSOR_TIMEOUT_ERROR;
      }
    }
    lastSignal[zone] = myTofSensor.getSignalPerSpad();                                   // Retrieve the average signal of a SPAD in the region of interest for this zone.
    // lastAmbient[zone] = myTofSensor.getAmbientPerSpad();                              // Retrieve the average ambient signal of a SPAD in the region of interest for this zone. (unused)
    zoneSignalPerSpad[zone][samples < 2 ? 0 : 1] = lastSignal[zone];                     // Set the measured signal strength to the zoneSignalPerSpad array. Again, 2 measurements for each zone per measure() call.
    if(zoneSignalPerSpad[zone][samples < 2 ? 0 : 1] == 65535){                           // If the reading suggests a data transfer or memory issue ...
      samples--;                                                                         // ... ignore this reading ...
      continue;                                                                          // ... and try again.
    }
    #if DEBUG_COUNTER
     if(samples >= 2){                                                                   // Log both zones' measurement tuples for this loop.
      zone == 0 ? Log.infoln("{1st=%ikbps/SPAD, 2nd=%ikbps/SPAD}", zoneSignalPerSpad[zone][0], zoneSignalPerSpad[zone][1]) : Log.infoln("                                    {1st=%ikbps/SPAD, 2nd=%ikbps/SPAD}", zoneSignalPerSpad[zone][0], zoneSignalPerSpad[zone][1]);
     }
    #endif
  }
  return(++ready);
}

int TofSensor::detect(){                                                              // Take a measurement in the detection zone (unused)
  ready = 0;
  unsigned long startedRanging;
  myTofSensor.stopRanging();
  myTofSensor.clearInterrupt();
  myTofSensor.setROI(DETECTION_ROWS_OF_SPADS, DETECTION_COLUMNS_OF_SPADS, DETECTION_ZONE_CENTER);
  delay(1);
  myTofSensor.startRanging();
  startedRanging = millis();
  while(!myTofSensor.checkForDataReady()) {
    if (millis() - startedRanging > SENSOR_TIMEOUT) {
      Log.infoln("Sensor Timed out");
      return SENSOR_TIMEOUT_ERROR;
    }
  }
  lastDetectionSignal = myTofSensor.getSignalPerSpad();
  #if DEBUG_COUNTER
      Log.infoln("Detection Zone: %ikbps/SPAD", lastDetectionSignal);
  #endif
  return(++ready);
}

int TofSensor::getDetectionZone() {
  return lastDetectionSignal;
}

int TofSensor::getOccupancyZone1() {
  return lastSignal[0];
}

int TofSensor::getOccupancyZone2() {
  return lastSignal[1];
}

int TofSensor::getOccupancyZone1Ambient() {
  return lastAmbient[0];
}

int TofSensor::getOccupancyZone2Ambient() {
  return lastAmbient[1];
}

int TofSensor::getOccupancyState() {
  return occupancyState;
}

bool TofSensor::recalibrate() {
  // if (TofSensor::performOccupancyCalibration() && TofSensor::performDetectionCalibration()){Log.infoln("Recalibrated"); return true;}
  if (TofSensor::performOccupancyCalibration()){Log.infoln("Recalibrated"); return true;}
  else {
    Log.infoln("Recalibration failed - waiting 10 seconds and resetting");
    delay(10000);
    sysStatus.alertCodeNode = 3;                    // Set a reset alert code and return false
    return false;
  }
}



