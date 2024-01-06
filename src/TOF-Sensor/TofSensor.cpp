// Time of Flight Sensor Class
// Authors: Chip McClelland (See Insights, LLC)
//          Alex Bowen (Orbit Development, LLC)
// Date: May 2023
// License: GPL3
// This is the class for the ST Micro VL53L1X Time of Flight Sensor
// We are using the Sparkfun library which has some shortcomgings
// - It does not implement distance mode medium
// - It does not give access to the factory calibration of the optical center

#include "ErrorCodes.h"
#include "MyData.h"
#include "Config.h"
#include "TofSensor.h"
#include <ArduinoLog.h>     // https://github.com/thijse/Arduino-Log
#include <Wire.h>

/** Occupancy **/
uint8_t occupancyOpticalCenters[2] = {OCCUPANCY_FRONT_ZONE_CENTER, OCCUPANCY_BACK_ZONE_CENTER}; // Array of optical centers for the Occupancy zones (zone 1and zone 2)
uint16_t zoneDistances[2] = {0, 0};                      // Stores the measured distances of the last measurement (measures both zone 1 and zone 2)
uint16_t zoneBaselineDistances[2] = {0, 0};                  // Maximum distance measure captured during calibration PLUS a static value from Config.h to prevent floor interference.
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

VL53L1X myTofSensor;

bool TofSensor::setup(){  
  myTofSensor.setTimeout(500);
  if(!myTofSensor.init()){
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
  
  Log.infoln("Calibrating TOF Sensor");

  while (TofSensor::measure() == SENSOR_BUFFRER_NOT_FULL) {delay(10);}; // Wait for the buffer to fill up
  
  if (TofSensor::performOccupancyCalibration()) Log.infoln("Calibration Complete");
  else {
    Log.infoln("Initial calibration failed - waiting 10 seconds and resetting");
    delay(10000);
    sysStatus.alertCodeNode = 3;                    // Set a reset alert code and return false
    return false;
  }

  return true;
}

bool TofSensor::performOccupancyCalibration() {
  if(TofSensor::measure() == SENSOR_TIMEOUT_ERROR){   // update zoneDistances with initial measurement
    return false;
  }
  zoneBaselineDistances[0] = zoneDistances[0];    // Assign the first readings as the baselines
  zoneBaselineDistances[1] = zoneDistances[1];
  for (int i = 0; i < OCCUPANCY_CALIBRATION_LOOPS; i++) {    // Loop through a set number of times ... 
    if(TofSensor::measure() == SENSOR_TIMEOUT_ERROR){        // ... measuring again each time ...
      return false; 
    }  
    if((zoneDistances[0] >= zoneBaselineDistances[0] + FLOOR_INTERFERENCE_BUFFER          // ... if further measurements exceed the baseline + FLOOR_INTERFERENCE_BUFFER ...
         || zoneDistances[1] >= zoneBaselineDistances[1] + FLOOR_INTERFERENCE_BUFFER ) 
     ||(zoneDistances[0] <= zoneBaselineDistances[0] - FLOOR_INTERFERENCE_BUFFER          // ... OR further measurements are below baseline - FLOOR_INTERFERENCE_BUFFER
         || zoneDistances[1] <= zoneBaselineDistances[1] - FLOOR_INTERFERENCE_BUFFER )){
      Log.infoln("Occupancy zone not clear - try again");
      delay(CALIBRATION_RETRY_DELAY);
      return TofSensor::performOccupancyCalibration();                                    // ... retry calibration by returning a recursive call of this function, which resets the zoneBaselineDistances
    }
    if(zoneDistances[0] < zoneBaselineDistances[0]) zoneBaselineDistances[0] = zoneDistances[0];   // ... check if the readings are closer than the baseline stored for their zone ...
    if(zoneDistances[1] < zoneBaselineDistances[1]) zoneBaselineDistances[1] = zoneDistances[1];   // ... if they are, set them as the baseline.
  }

  if(zoneBaselineDistances[0] > 4000 || zoneBaselineDistances[1] > 4000) {    // if the baselines make no sense (happened when face down on the couch), retry. Maximum distance should be 4000mm 
    Log.infoln("Occupancy zone not clear - try again");
    delay(CALIBRATION_RETRY_DELAY);
    return TofSensor::performOccupancyCalibration();   // ... retry calibration by returning a recursive call of this function, which resets the zoneBaselineDistances
  } 

  zoneBaselineDistances[0] = zoneBaselineDistances[0] - FLOOR_INTERFERENCE_BUFFER;    // Adjust the baselines by subtracting the FLOOR_INTERFERENCE_BUFFER
  zoneBaselineDistances[1] = zoneBaselineDistances[1] - FLOOR_INTERFERENCE_BUFFER;



  Log.infoln("Target zone is clear with zone1 baseline at %imm and zone2 baseline at %imm",zoneBaselineDistances[0],zoneBaselineDistances[1]);
  return true;
}

int TofSensor::loop(){    // This function will update the current detection or occupancy for each zone. Switches modes based on interrupt triggers. Returns error codes also.
  int result = 0;
  occupancyState = 0;
  result = TofSensor::measure();
  occupancyState += (zoneDistances[0] < zoneBaselineDistances[0]) ? 1 : 0;
  occupancyState += (zoneDistances[1] < zoneBaselineDistances[1]) ? 2 : 0;
  return result;     // Relay the result of taking our samples.
}

int TofSensor::measure(){
  ready = 0;                                                                           
  for (int zone = 0; zone < 2; zone++){           // Take 2 samples, 1 for each zone.
    #if SHORT_DISTANCE_MODE
      int32_t timingBudget = 20000;                    //(in us)
      myTofSensor.setDistanceMode(VL53L1X::Short);
    #else
      int32_t timingBudget = 33000;                    //(in us)
      myTofSensor.setDistanceMode(VL53L1X::Long);          
    #endif
    
    myTofSensor.setROISize(OCCUPANCY_ZONE_SPAD_WIDTH, OCCUPANCY_ZONE_SPAD_DEPTH);
    myTofSensor.setROICenter(occupancyOpticalCenters[zone]);
    myTofSensor.setMeasurementTimingBudget(timingBudget);    // 20000us in short distance mode, 33000us in long distance mode

    // ** POLOLU DOCUMENTATION ** 
    // Starts a single-shot range measurement. If blocking is true (the default),
    // this function waits for the measurement to finish and returns the reading.
    // Otherwise, it returns 0 immediately.
    zoneDistances[zone] = myTofSensor.readRangeSingleMillimeters();
    if(zoneDistances[zone] == 65535){                           // If the reading suggests a data transfer or memory issue ...
      zone--;                                                                         // ... ignore the reading ...
      continue;                                                                          // ... and read that zone again.
    }
    #if PRINT_SENSOR_MEASUREMENTS                               // Logs both zones' distances for this loop.
      zone == 0 ? Log.infoln("{zone1 = %dmm}", zoneDistances[zone]) : Log.infoln("                                    (zone2 = %dmm)", zoneDistances[zone]);
    #endif
    // #if PRINT_ROI_DETAILS
    //   Log.infoln("SPAD array for zone %d: %d x %d = %d SPADs", zone, myTofSensor.getROIX(), myTofSensor.getROIY(), myTofSensor.getSpadNb());
    // #endif
  }
  return(++ready);
}

int TofSensor::getLastDistanceZone1() {
  return zoneDistances[0];
}

int TofSensor::getLastDistanceZone2() {
  return zoneDistances[1];
}

int TofSensor::getOccupancyState() {
  return occupancyState;
}

bool TofSensor::recalibrate() {
  if (TofSensor::performOccupancyCalibration()){Log.infoln("Recalibrated"); return true;}
  else {
    Log.infoln("Recalibration failed - waiting 10 seconds and resetting");
    delay(10000);
    sysStatus.alertCodeNode = 3;                    // Set a reset alert code and return false
    return false;
  }
}