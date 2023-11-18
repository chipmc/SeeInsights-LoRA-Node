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

// /** Detection **/
// int detectionBaselines[2] = {0, 0};                 // Upper and lower values that define a baseline "range" of signal strength that is considered as "nothing" (Max, Min)
// int lastDetectionSignal = 0;                        // The most recent Detection zone Signal
// int detectionState = 0;                             // The current detection state (detected someone or not)
// int detectionMode = 1;                              // Flag for putting the device in "detection" mode, a state saving power by waiting to trigger an interrupt

/** Occupancy **/
uint8_t occupancyOpticalCenters[2] = {FRONT_ZONE_CENTER, BACK_ZONE_CENTER}; // Array of optical centers for the Occupancy zones (zone 1 (ones) and zone 2 (twos))
int zoneSignalPerSpad[2][2] = {{0, 0}, {0, 0}};     // Stores the average Signal strength of all SPADs in bursts of 2 readings for each zone to reduce noise (zone 1 (ones) and zone 2 (twos))
int occupancyBaselines[2][2] = {{0, 0}, {0, 0}};    // Upper and lower values that define a baseline "range" of signal strength that is considered as "nothing" ((zone 1(Min, Max)) , (zone 2(Min, Max)))
int lastSignal[2] = {0, 0};                         // The most recent Signal (zone 1 (ones) and zone 2 (twos))
int lastAmbient[2] = {0, 0};                        // The most recent Ambient (zone 1 (ones) and zone 2 (twos))
int occupancyState = 0;                             // The current occupancy state (occupied or not, zone 1 (ones) and zone 2 (twos))
int ready = 0;                                      // "ready" flag

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
    Log.infoln("Sensor error reset in 10 seconds");
    delay(10000);
    sysStatus.alertCodeNode = 3;
  }
  else Log.infoln("Sensor init successfully");
  
  // Here is where we set the device properties
  myTofSensor.setSigmaThreshold(45);                // Default is 45 - this will make it harder to get a valid result - Range 1 - 16383
  myTofSensor.setSignalThreshold(1500);             // Default is 1500 raising value makes it harder to get a valid results- Range 1-16383
  myTofSensor.setTimingBudgetInMs(20);              // Was 20mSec

  while (TofSensor::measure() == SENSOR_BUFFRER_NOT_FULL) {delay(10);}; // Wait for the buffer to fill up
  Log.infoln("Calibrating TOF Sensor");

  if (TofSensor::performOccupancyCalibration()) Log.infoln("Calibration Complete");
  else {
    Log.infoln("Initial calibration failed - wait 10 secs and reset");
    delay(10000);
    sysStatus.alertCodeNode = 3;                    // Set a reset alert code and return false
    return false;
  }

  // myTofSensor.setDistanceModeShort();            // Once initialized, we are focused on the top half of the door
 
  return true;
}

bool TofSensor::performOccupancyCalibration() {
  int clear = 1;
  TofSensor::measure();
  occupancyBaselines[0][0] = lastSignal[0];         // Assign the first reading as max AND min for each zone
  occupancyBaselines[0][1] = lastSignal[0];
  occupancyBaselines[1][0] = lastSignal[1] + 5;     // little bit of leeway for the upper range to help with calibration
  occupancyBaselines[1][1] = lastSignal[1] + 5;
  for (int i=0; i<NUM_OCCUPANCY_CALIBRATION_LOOPS; i++) {                                                         // Loop through a set number of times ... 
    TofSensor::loop();
    if(zoneSignalPerSpad[0][0] < occupancyBaselines[0][0]) occupancyBaselines[0][0] = zoneSignalPerSpad[0][0];    // ... and update the respective min and max ranges each time
    if(zoneSignalPerSpad[1][0] < occupancyBaselines[1][0]) occupancyBaselines[1][0] = zoneSignalPerSpad[1][0];
    if(zoneSignalPerSpad[0][1] > occupancyBaselines[0][1]) occupancyBaselines[0][1] = zoneSignalPerSpad[0][1];
    if(zoneSignalPerSpad[1][1] > occupancyBaselines[1][1]) occupancyBaselines[1][1] = zoneSignalPerSpad[1][1];
  }

  if(occupancyBaselines[0][0] < 5) occupancyBaselines[0][0] = 5;   // Make sure we have a bit of room at the bottom of the range for black hair
  if(occupancyBaselines[1][0] < 5) occupancyBaselines[1][0] = 5;

  if(!clear){
      Log.infoln("Occupancy zone not clear - try again");
      delay(20);
      TofSensor::performOccupancyCalibration();
  } else {
    Log.infoln("Target zone is clear with zone1 range at {MIN: %ikcps/SPAD, MAX: %ikcps/SPAD} and zone2 range at {MIN: %ikcps/SPAD, MAX: %ikcps/SPAD}",occupancyBaselines[0][0],occupancyBaselines[0][1],occupancyBaselines[1][0],occupancyBaselines[1][1]);
  }
  return true;
}

// bool TofSensor::performDetectionCalibration() {
//   TofSensor::detect();
//   detectionBaselines[0] = lastDetectionSignal;        // Assign the first reading as max AND min of baseline
//   detectionBaselines[1] = lastDetectionSignal; 
//   for (int i=0; i<NUM_DETECTION_CALIBRATION_LOOPS; i++) {       // Loop through a set number of times ... 
//     TofSensor::detect();
//     if(lastDetectionSignal < detectionBaselines[0]) detectionBaselines[0] = lastDetectionSignal;    // ... and update the min and max readings each time.
//     if(lastDetectionSignal > detectionBaselines[1]) detectionBaselines[1] = lastDetectionSignal;
//   }
//   while (detectionState != 0){
//     Log.infoln("Detection zone not clear - try again");
//     delay(20);
//     TofSensor::loop();
//   }
//   detectionBaselines[0] -= 3;                               
//   detectionBaselines[1] += 3;
//   if(detectionBaselines[0] < 8) detectionBaselines[0] = 8;   // Make sure we have a bit of room at the bottom of the range for black hair. TODO:: MAYBE MAKE SPADS SMALLER OR USE DISTANCE

//   Log.infoln("Detection zone is clear with range {MIN: %ikcps/SPAD, MAX: %ikcps/SPAD} ",detectionBaselines[0],detectionBaselines[1]);
//   return TRUE;
// }

int TofSensor::loop(){                         // This function will update the current detection or occupancy for each zone. Switches modes based on interrupt triggers. Returns error codes also.
  int result = 0;
  // if(detectionMode){
  //   detectionState = 0;
  //   result = TofSensor::detect();
  //   detectionState += (lastDetectionSignal > detectionBaselines[1] || lastDetectionSignal < detectionBaselines[0]) ? 1 : 0;
  //   #if DEBUG_COUNTER
  //       Log.infoln("[DETECTION BASELINE RANGES] {Min=%ikbps/SPAD, Max=%ikbps/SPAD}", detectionBaselines[0], detectionBaselines[1]);
  //       Log.infoln("[DETECTION CHECK] Zone1:{1st=%ikbps/SPAD, 2nd=%ikbps/SPAD} Zone2:{1st=%ikbps/SPAD, 2nd=%ikbps/SPAD} - Occupancy State %i\n", zoneSignalPerSpad[0][0], zoneSignalPerSpad[0][1], zoneSignalPerSpad[1][0], zoneSignalPerSpad[1][1], occupancyState);
  //   #endif

  // } else {
    occupancyState = 0;
    result = TofSensor::measure();
    if (result == SENSOR_TIMEOUT_ERROR){
      sysStatus.alertCodeNode = 3;
      return result;
    }
    occupancyState += ((zoneSignalPerSpad[0][0] > (occupancyBaselines[0][1]) && zoneSignalPerSpad[0][1] > (occupancyBaselines[0][1]))
                      || (zoneSignalPerSpad[0][0] <= (occupancyBaselines[0][0]) && zoneSignalPerSpad[0][1] <= (occupancyBaselines[0][0]))) ? 1 : 0;
    
    occupancyState += ((zoneSignalPerSpad[1][0] > (occupancyBaselines[1][1]) && zoneSignalPerSpad[1][1] > (occupancyBaselines[1][1]))
                      || (zoneSignalPerSpad[1][0] <= (occupancyBaselines[1][0]) && zoneSignalPerSpad[1][1] <= (occupancyBaselines[1][0]))) ? 2 : 0;
    // #if DEBUG_COUNTER
    //   Log.infoln("[OCCUPANCY CHECK] Zone1:{1st=%ikbps/SPAD, 2nd=%ikbps/SPAD} Zone2:{1st=%ikbps/SPAD, 2nd=%ikbps/SPAD} - Occupancy State %i\n", zoneSignalPerSpad[0][0], zoneSignalPerSpad[0][1], zoneSignalPerSpad[1][0], zoneSignalPerSpad[1][1], occupancyState);
    //   delay(100);
    // #endif
  // }
  return result;     // Relay the result of taking our samples.
}

int TofSensor::measure(){
  ready = 0;                                                                           
  unsigned long startedRanging;
  for (byte samples = 0; samples < 4; samples++){                                        // Take 4 samples...
    int zone = samples % 2;                                                              // ... 2 for each zone.    
    myTofSensor.stopRanging();
    myTofSensor.clearInterrupt();
    myTofSensor.setROI(ROWS_OF_SPADS,COLUMNS_OF_SPADS,occupancyOpticalCenters[zone]);    // Set the Region of Interest for the two zones
    delay(1);
    myTofSensor.startRanging();
    startedRanging = millis();
    while(!myTofSensor.checkForDataReady()) {                                            // Time out if something is wrong with the sensor
      if (millis() - startedRanging > SENSOR_TIMEOUT) {
        Log.infoln("Sensor Timed out");
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

// int TofSensor::detect(){                                                              // Take a measurement in the detection zone (unused)
//   ready = 0;
//   unsigned long startedRanging;
//   myTofSensor.stopRanging();
//   myTofSensor.clearInterrupt();
//   myTofSensor.setROI(DETECTION_ROWS_OF_SPADS, DETECTION_COLUMNS_OF_SPADS, DETECTION_ZONE_CENTER);
//   delay(1);
//   myTofSensor.startRanging();
//   startedRanging = millis();
//   while(!myTofSensor.checkForDataReady()) {
//     if (millis() - startedRanging > SENSOR_TIMEOUT) {
//       Log.infoln("Sensor Timed out");
//       return SENSOR_TIMEOUT_ERROR;
//     }
//   }
//   lastDetectionSignal = myTofSensor.getSignalPerSpad();
//   return(++ready);
// }

// int TofSensor::getDetectionMode() {
//   return detectionMode;
// }

// void TofSensor::setDetectionMode(int mode) {
//   detectionMode = mode;
// }

// int TofSensor::getDetectionZone() {
//   return lastDetectionSignal;
// }

// int TofSensor::getDetectionState() {
//   return detectionState;
// }

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



