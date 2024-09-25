// Time of Flight Sensor Class
// Authors: Chip McClelland (See Insights, LLC)
//          Alex Bowen (Orbit Development, LLC)
// Date: May 2023
// License: GPL3
// This is the class for the ST Micro VL53L1X Time of Flight Sensor
// We are using the Pololu VL53L1X library // https://github.com/pololu/vl53l1x-arduino?tab=readme-ov-file

#include "ErrorCodes.h"
#include "MyData.h"
#include "Config.h"
#include "TofSensor.h"
#include <ArduinoLog.h>     // https://github.com/thijse/Arduino-Log
#include <Wire.h>

/** Measure **/
uint16_t measurementDistances[2] = {0, 0};                      // Stores the measured distances of the last measurement (measures both zone 1 and zone 2)
uint16_t measurementBaselineDistances[2] = {0, 0};              // Minimum measure distance captured during calibration PLUS a static value from Config.h to prevent floor interference.

/** Detect **/
uint16_t detectionDistance = 0;                                 // Stores the measured distance of the last **detection** attempt
uint16_t detectionBaselineDistance = 0;                         // Minimum detection distance captured during calibration PLUS a static value from Config.h to prevent floor interference.

int occupancyState = 0;                             // The current occupancy state (occupied or not, zone 1 (ones) and zone 2 (twos))
int detectionState = 0;                             // The current detection state (have detected a person in detection zone or not)

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
  myTofSensor.setTimeout(TOF_SENSOR_TIMEOUT);
  if(!myTofSensor.init()){
    if(numberOfRetries == 3){                      // if 3 retries, return false
      sysStatus.alertCodeNode = 3;  
      return false;
    } else {
      sysStatus.alertCodeNode = 3;
    }
    Log.infoln("Sensor did not initialize - retry in 10 seconds");
    delay(10000);
    numberOfRetries++;
  } else {
    Log.infoln("Sensor init successfully");
    Log.infoln("Calibrating TOF Sensor");

    while (TofSensor::instance().measure() == SENSOR_BUFFRER_NOT_FULL) {delay(10);}; // Wait for the buffer to fill up
    
    if (TofSensor::instance().performOccupancyCalibration()) Log.infoln("Calibration Complete");
    else {
      Log.infoln("Initial calibration failed - waiting 10 seconds and resetting");
      delay(10000);
      sysStatus.alertCodeNode = 3;                    // Set a reset alert code and return false
      return false;
    }

    return true;
  }
  return false;
}

bool TofSensor::performOccupancyCalibration() {
  if(TofSensor::instance().measure() == SENSOR_TIMEOUT_ERROR){   // update measurementDistances with initial measurement
    return false;
  }
  if(TofSensor::instance().detect() == SENSOR_TIMEOUT_ERROR){    // update detectionDistance with initial measurement
    return false;
  }
  measurementBaselineDistances[0] = measurementDistances[0];    // Assign the first readings as the baselines
  measurementBaselineDistances[1] = measurementDistances[1];
  detectionBaselineDistance = detectionDistance;
  for (int i = 0; i < sysStatus.occupancyCalibrationLoops; i++) {    // Loop through a set number of times ... 
    if(TofSensor::instance().measure() == SENSOR_TIMEOUT_ERROR){        // ... measuring again each time ...
      return false; 
    }  
    if(TofSensor::instance().detect() == SENSOR_TIMEOUT_ERROR){        // ... and detecting again each time ...
      return false; 
    } 
    if((measurementDistances[0] < measurementBaselineDistances[0] - sysStatus.interferenceBuffer          // If further measurements are closer than baseline - FLOOR_INTERFERENCE_BUFFER (value in mm)
         || measurementDistances[1] < measurementBaselineDistances[1] - sysStatus.interferenceBuffer )){
      Log.infoln("Occupancy zone not clear, measurements had too much variation - trying again (maybe increase interference buffer?)");
      delay(TOF_CALIBRATION_RETRY_DELAY);
      return TofSensor::instance().performOccupancyCalibration();   // ... retry calibration by returning a recursive call of this function, which resets the baseline distances
    }
    if(measurementDistances[0] < measurementBaselineDistances[0]) measurementBaselineDistances[0] = measurementDistances[0];   // ... check if the readings are closer than the baseline stored for their zone ...
    if(measurementDistances[1] < measurementBaselineDistances[1]) measurementBaselineDistances[1] = measurementDistances[1];   // ... if they are, set them as the baseline.
    if(detectionDistance < detectionBaselineDistance) detectionBaselineDistance = detectionDistance;  
  }

  measurementBaselineDistances[0] = measurementBaselineDistances[0] - sysStatus.interferenceBuffer;   // Adjust the baselines by subtracting the FLOOR_INTERFERENCE_BUFFER. 
  measurementBaselineDistances[1] = measurementBaselineDistances[1] - sysStatus.interferenceBuffer;   // We do this in order to 'raise' the baseline distance, which ignores variation in floor measurements
  detectionBaselineDistance = detectionDistance - sysStatus.interferenceBuffer;

  if(measurementBaselineDistances[0] > 4000 || measurementBaselineDistances[1] > 4000 || detectionBaselineDistance > 4000) {   // If we measured any baseline to be less than the FLOOR_INTERFERENCE_BUFFER, try again. 4000mm(4m) is the maximum measurement distance
    Log.infoln("Occupancy zone not clear (Something is too close to the sensor) - trying again");
    delay(TOF_CALIBRATION_RETRY_DELAY);
    return TofSensor::instance().performOccupancyCalibration();   // ... retry calibration by returning a recursive call of this function, which resets the measurementBaselineDistances
  } 

  Log.infoln("Target zone is clear with baselines: detection %imm / zone1 %imm / zone2 %imm", detectionBaselineDistance, measurementBaselineDistances[0],measurementBaselineDistances[1]);
  return true;
}

int TofSensor::loop(){    // This function will update the current detection state and occupancy state. Returns error codes.
  int result = 0;
  
  // OccupancyState is 0 if detectionState is 0. If detectionState is set to 1 in the below conditional, the first measure()
  // operation will **also** occur before this function returns in order to catch people fastest.
  if (occupancyState == 0){
    delay(1000 / sysStatus.tofDetectionsPerSecond);   // Enforce our configured intermeasurement period by calling delay()
    result = TofSensor::instance().detect();          // Perform detect() operation instead of measure() when occupancyState == 0
    detectionState = (detectionDistance < detectionBaselineDistance) ? 1 : 0;
  }       

  if(detectionState == 1) {   // If we detect someone when occupancyState == 0, immediately begin measuring at max polling rate. detectionState is persistent through loops.
    occupancyState = 0;             // occupancyState is **fully** recalculated every time. 
    result = TofSensor::instance().measure();
    occupancyState += (measurementDistances[0] < measurementBaselineDistances[0]) ? 1 : 0;
    occupancyState += (measurementDistances[1] < measurementBaselineDistances[1]) ? 2 : 0;
    // If occupancyState == 0 after measuring, the next loop will detect and recalibrate detectionState.
  }

  return result;     // Relay the result of taking our samples.
}

int TofSensor::detect(){
  ready = 0;

  uint8_t zoneWidth = 16;                      // width of SPADs (across the door)
  uint8_t zoneDepth = 16;                      // depth of SPADs (through the door)
  uint8_t zoneOpticalCenter = 199;             // denotes dead-center of SPAD array
  
  configureSensor(sysStatus.distanceMode, zoneDepth, zoneWidth, zoneOpticalCenter);

  // ** POLOLU DOCUMENTATION ** 
  // Starts a single-shot range measurement. If blocking is true (the default),
  // this function waits for the measurement to finish and returns the reading.
  // Otherwise, it returns 0 immediately.
  detectionDistance = myTofSensor.readRangeSingleMillimeters();

  if(detectionDistance == 65535){                           // If the reading suggests a data transfer or memory issue ...
    return TofSensor::instance().detect();                                                             // ... try to detect again.
  }

  #if TOF_PRINT_SENSOR_MEASUREMENTS                             // Logs the detection distance.
    Log.infoln("[DETECTING]                   {detection zone = %dmm}                  ", detectionDistance);
  #endif
  
  #if TOF_PRINT_ROI_DETAILS
    uint8_t ROIx;
    uint8_t ROIy;
    myTofSensor.getROISize(&ROIx, &ROIy);
    uint8_t ROICenter = myTofSensor.getROICenter();
    Log.infoln("SPAD array for detection zone: %d x %d SPADs with center %d", ROIx, ROIy, ROICenter);
  #endif

  return(++ready);
}

int TofSensor::measure(){
  ready = 0;
  uint8_t zoneWidth = 0;                      // width of SPADs (across the door)
  uint8_t zoneDepth = 0;                      // depth of SPADs (through the door)
  uint8_t zoneOpticalCenters[2] = {0,0};          // Array of optical centers for the Occupancy zones (zone 1 and zone 2)
  
  switch(sysStatus.zoneMode){             // Set the spad depth, spad width and opticalCenters as defined by the zoneMode. See Config.h for zone mode definitions.
    case 0:                 // default
      zoneWidth = 16;
      zoneDepth = 8;
      zoneOpticalCenters[0] = 167;
      zoneOpticalCenters[1] = 231;
    break;
    case 1:                 // separated
      zoneWidth = 16;
      zoneDepth = 6;
      zoneOpticalCenters[0] = 159;
      zoneOpticalCenters[1] = 239;
    break;
    case 2:                 // verySeparated
      zoneWidth = 16;
      zoneDepth = 4;
      zoneOpticalCenters[0] = 151;
      zoneOpticalCenters[1] = 247;
    break;
    case 3:                 // frontFocused
      zoneWidth = 16;
      zoneDepth = 4;
      zoneOpticalCenters[0] = 151;
      zoneOpticalCenters[1] = 183;
    break;
    case 4:                 // backFocused
      zoneWidth = 16;
      zoneDepth = 4;
      zoneOpticalCenters[0] = 215;
      zoneOpticalCenters[1] = 247;
    break;
    case 5:                 // thin
      zoneWidth = 8;
      zoneDepth = 8;
      zoneOpticalCenters[0] = 167;
      zoneOpticalCenters[1] = 231;
    break;
    case 6:                 // veryThin
      zoneWidth = 4;
      zoneDepth = 8;
      zoneOpticalCenters[0] = 167;
      zoneOpticalCenters[1] = 231;
    break;
  }        

  for (int zone = 0; zone < 2; zone++){           // Take 2 samples, 1 for each zone.

    configureSensor(sysStatus.distanceMode, zoneDepth, zoneWidth, zoneOpticalCenters[zone]);

    // ** POLOLU DOCUMENTATION ** 
    // Starts a single-shot range measurement. If blocking is true (the default),
    // this function waits for the measurement to finish and returns the reading.
    // Otherwise, it returns 0 immediately.
    measurementDistances[zone] = myTofSensor.readRangeSingleMillimeters();

    if(measurementDistances[zone] == 65535){                           // If the reading suggests a data transfer or memory issue ...
      zone--;                                                                         // ... ignore the reading ...
      continue;                                                                          // ... and read that zone again.
    }

    #if TOF_PRINT_SENSOR_MEASUREMENTS                               // Logs both zones' distances for this loop.
      zone == 0 ? Log.infoln("[MEASURING]  {zone1 = %dmm}", measurementDistances[zone]) : Log.infoln("[MEASURING]                                             {zone2 = %dmm}", measurementDistances[zone]);
    #endif

    #if TOF_PRINT_ROI_DETAILS
      uint8_t ROIx;
      uint8_t ROIy;
      myTofSensor.getROISize(&ROIx, &ROIy);
      uint8_t ROICenter = myTofSensor.getROICenter();
      Log.infoln("SPAD array for zone %d: %d x %d SPADs with center %d", zone, ROIx, ROIy, ROICenter);
    #endif
  }
  return(++ready);
}

int TofSensor::getLastDistanceZone1() {
  return measurementDistances[0];
}

int TofSensor::getLastDistanceZone2() {
  return measurementDistances[1];
}

int TofSensor::getOccupancyState() {
  return occupancyState;
}

bool TofSensor::recalibrate() {
  if (TofSensor::instance().performOccupancyCalibration()){Log.infoln("Recalibrated"); return true;}
  else {
    Log.infoln("Recalibration failed - waiting 10 seconds and resetting");
    delay(10000);
    sysStatus.alertCodeNode = 3;                    // Set a reset alert code and return false
    return false;
  }
}

void TofSensor::configureSensor(uint8_t distanceMode, uint8_t zoneDepth, uint8_t zoneWidth, uint8_t zoneOpticalCenter){
  uint32_t timingBudget;
  switch (sysStatus.distanceMode) {  // Set the timing budget to the minimum value allowable for the distanceMode, according to the datasheet https://www.pololu.com/file/0J1506/vl53l1x.pdf
    case 0:
      timingBudget = 22000;                           // minimum ranging duration for distanceMode short (20000us)
      myTofSensor.setDistanceMode(VL53L1X::Short);
    break;
    case 1:
      timingBudget = 33000;                           // minimum ranging duration for distanceMode medium (33000us)
      myTofSensor.setDistanceMode(VL53L1X::Medium);
    break;
    case 2:
      timingBudget = 33000;                           // minimum ranging duration for distanceMode long (33000us)
      myTofSensor.setDistanceMode(VL53L1X::Long);          
    break;
    default: // default to long if something is up
      timingBudget = 33000;                           // minimum ranging duration for distanceMode long (33000us)
      myTofSensor.setDistanceMode(VL53L1X::Long);
  }
  
  myTofSensor.setROISize(zoneDepth, zoneWidth);
  myTofSensor.setROICenter(zoneOpticalCenter);
  myTofSensor.setMeasurementTimingBudget(timingBudget);    // 20000us minimum in short distance mode, 33000us minimum in medium/long distance mode
}