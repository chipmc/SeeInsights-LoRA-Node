// People Counter Class
// Author: Chip McClelland
// Date: May 2023
// License: GPL3
// In this class, we look at the occpancy values and determine what the occupancy count should be 
// Note, this code is limited to a single sensor with two zones
// Note, this code assumes that Zone 1 is the inner (relative to room we are measureing occupancy for) and Zone 2 is outer

#include "PeopleCounter.h"

static int occupancyLimit = DEFAULT_PEOPLE_LIMIT;

PeopleCounter *PeopleCounter::_instance;

// [static]
PeopleCounter &PeopleCounter::instance() {
    if (!_instance) {
        _instance = new PeopleCounter();
    }
    return *_instance;
}

PeopleCounter::PeopleCounter() {
}

PeopleCounter::~PeopleCounter() {
}

void PeopleCounter::setup() {
}

bool PeopleCounter::loop(){                // This function is only called if there is a change in occupancy state
    static int oldOccupancyState = 0;       // Need to remember these for past state path
    static bool atTheThreshold = false;
    int oldOccupancyCount = current.occupancyNet;

    current.occupancyState = TofSensor::instance().getOccupancyState();

    switch (current.occupancyState) {
      case 0:                               // No occupancy detected
        oldOccupancyState = 0;
        atTheThreshold = false;
      break;

      case 1:
        if (atTheThreshold) {
          atTheThreshold = false;
          if (oldOccupancyState == 2) {
            current.occupancyGross++;
            current.occupancyNet++;
          }
        }
        oldOccupancyState = 1;
      break;

      case 2:
        if (atTheThreshold) {
          atTheThreshold = false;
          if (oldOccupancyState == 1) {
            if (current.occupancyNet > 0) {
              current.occupancyNet--;
              current.occupancyGross--;
            }
          }
        }
        oldOccupancyState = 2;
      break;

      case 3:
        atTheThreshold = true;
      break;

      default:
        #if PEOPLECOUNTER_DEBUG
        Log.infoln("Error in occupancy state");
        #endif
      break;
    }

   #if TENFOOTDISPLAY
    if (oldOccupancyCount != current.occupancyNet) {
      currentData.currentDataChanged = true;
      printBigNumbers(current.occupancyNet);
      return true;
    }
   #else
    if (oldOccupancyCount != occupancyCount) {
      currentData.currentDataChanged = true;
      Log.infoln("Occupancy %s %i",(occupancyCount > oldOccupancyCount) ? "increased to" : "decreased to", occupancyCount);
      return true;
    }
   #endif
   return false;
}

void PeopleCounter::setCount(int value){
  current.occupancyNet = value;
}

int PeopleCounter::getLimit(){
  return occupancyLimit;
}

void PeopleCounter::setLimit(int value){
  occupancyLimit = value;
}


void PeopleCounter::printBigNumbers(int number) {
  Log.infoln("  ");

  switch (abs(number)) {
    case 0:
      Log.infoln("%s  0000  ", (number < 0) ? "      " : "");
      Log.infoln("%s 0    0 ", (number < 0) ? "      " : "");
      Log.infoln("%s0      0", (number < 0) ? "      " : "");
      Log.infoln("%s0      0", (number < 0) ? "------" : "");  
      Log.infoln("%s0      0", (number < 0) ? "      " : "");     
      Log.infoln("%s 0    0 ", (number < 0) ? "      " : "");
      Log.infoln("%s  0000  ", (number < 0) ? "      " : "");
    break;

    case 1:
      Log.infoln("%s    11  ", (number < 0) ? "      " : "");
      Log.infoln("%s   1 1  ", (number < 0) ? "      " : "");
      Log.infoln("%s     1  ", (number < 0) ? "      " : "");
      Log.infoln("%s     1  ", (number < 0) ? "------" : "");
      Log.infoln("%s     1  ", (number < 0) ? "      " : "");
      Log.infoln("%s     1  ", (number < 0) ? "      " : "");
      Log.infoln("%s   11111", (number < 0) ? "      " : "");
    break;

    case 2:
      Log.infoln("%s  2222  ", (number < 0) ? "      " : "");
      Log.infoln("%s 2    22", (number < 0) ? "      " : "");
      Log.infoln("%s     2  ", (number < 0) ? "      " : "");
      Log.infoln("%s   2    ", (number < 0) ? "------" : "");
      Log.infoln("%s  2     ", (number < 0) ? "      " : "");
      Log.infoln("%s22     2", (number < 0) ? "      " : "");
      Log.infoln("%s2222222 ", (number < 0) ? "      " : "");
      break;

      case 3:
      Log.infoln("%s  3333  ", (number < 0) ? "      " : "");
      Log.infoln("%s 3    3 ", (number < 0) ? "      " : "");
      Log.infoln("%s       3", (number < 0) ? "      " : "");
      Log.infoln("%s   333  ", (number < 0) ? "------" : "");
      Log.infoln("%s       3", (number < 0) ? "      " : "");
      Log.infoln("%s 3    3 ", (number < 0) ? "      " : "");
      Log.infoln("%s  3333  ", (number < 0) ? "      " : "");
      break;

      case 4:
      Log.infoln("%s4      4", (number < 0) ? "      " : "");
      Log.infoln("%s4      4", (number < 0) ? "      " : "");
      Log.infoln("%s4      4", (number < 0) ? "      " : "");
      Log.infoln("%s  4444  ", (number < 0) ? "------" : "");
      Log.infoln("%s       4", (number < 0) ? "      " : "");
      Log.infoln("%s       4", (number < 0) ? "      " : "");
      Log.infoln("%s       4", (number < 0) ? "      " : "");
      break;

      case 5:
      Log.infoln("%s  555555", (number < 0) ? "      " : "");
      Log.infoln("%s 5      ", (number < 0) ? "      " : "");
      Log.infoln("%s 555555 ", (number < 0) ? "      " : "");
      Log.infoln("%s      5 ", (number < 0) ? "------" : "");
      Log.infoln("%s       5", (number < 0) ? "      " : "");
      Log.infoln("%s      5 ", (number < 0) ? "      " : "");
      Log.infoln("%s 555555 ", (number < 0) ? "      " : "");
      break;

      case 6:
      Log.infoln("%s  666666", (number < 0) ? "      " : "");
      Log.infoln("%s 6      ", (number < 0) ? "      " : "");
      Log.infoln("%s  66666 ", (number < 0) ? "      " : "");
      Log.infoln("%s6      6", (number < 0) ? "------" : "");
      Log.infoln("%s6      6", (number < 0) ? "      " : "");
      Log.infoln("%s 6    6 ", (number < 0) ? "      " : "");
      Log.infoln("%s  6666  ", (number < 0) ? "      " : "");
      break;

      case 7:
      Log.infoln("%s  777777", (number < 0) ? "      " : "");
      Log.infoln("%s 7     7", (number < 0) ? "      " : "");
      Log.infoln("%s      7 ", (number < 0) ? "      " : "");
      Log.infoln("%s     7  ", (number < 0) ? "------" : "");
      Log.infoln("%s    7   ", (number < 0) ? "      " : "");
      Log.infoln("%s   7    ", (number < 0) ? "      " : "");
      Log.infoln("%s  7     ", (number < 0) ? "      " : "");
      break;

      case 8:
      Log.infoln("%s  8888  ", (number < 0) ? "      " : "");
      Log.infoln("%s 8    8 ", (number < 0) ? "      " : "");
      Log.infoln("%s8      8", (number < 0) ? "      " : "");
      Log.infoln("%s  8888  ", (number < 0) ? "------" : "");
      Log.infoln("%s8      8", (number < 0) ? "      " : "");
      Log.infoln("%s 8    8 ", (number < 0) ? "      " : "");
      Log.infoln("%s  8888  ", (number < 0) ? "      " : "");
      break;

      case 9:
      Log.infoln("%s 99999  ", (number < 0) ? "      " : "");
      Log.infoln("%s9     9 ", (number < 0) ? "      " : "");
      Log.infoln("%s9      9", (number < 0) ? "      " : "");
      Log.infoln("%s 99999  ", (number < 0) ? "------" : "");
      Log.infoln("%s       9", (number < 0) ? "      " : "");
      Log.infoln("%s      9 ", (number < 0) ? "      " : "");
      Log.infoln("%s 999999 ", (number < 0) ? "      " : "");
      break;

      default:
      Log.infoln("********");
      Log.infoln("********");
      Log.infoln("********");
      Log.infoln("********");
      Log.infoln("********");
      Log.infoln("********");
      Log.infoln("********");
      break;
  }
  Log.infoln("  ");
}
