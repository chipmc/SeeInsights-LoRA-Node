// People Counter Class
// Author: Chip McClelland
// Date: May 2023
// License: GPL3
// In this class, we look at the occpancy values and determine what the occupancy count should be 
// Note, this code is limited to a single sensor with two zones
// Note, this code assumes that Zone 1 is the inner (relative to room we are measureing occupancy for) and Zone 2 is outer

#include "PeopleCounterConfig.h"
#include "PeopleCounter.h"
#include <utils\StackArray.h>

// StackArray data structure:
// [ bottom, ... , ... , top ]
StackArray <int> stateStack;
StackArray <int> tempStack;

static int occupancyLimit = DEFAULT_PEOPLE_LIMIT;

int impossibleStateTransition[4] = {3, 2, 1, 0};                  // Define impossible state transitions (Ex. newOccupancyState cannot equal impossibilityMap[lastOccupancyState])

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
  MOUNTED_INSIDE ? PeopleCounter::instance().setCount(1) : PeopleCounter::instance().setCount(0);
}

bool PeopleCounter::loop(){
    int oldOccupancyCount = current.hourlyCount;
  int newOccupancyState = TofSensor::instance().getOccupancyState();
  
  if(newOccupancyState != stateStack.peek()){
    switch(stateStack.count()){
      case 0:                         
        stateStack.push(0);                                       // First value MUST be a 0
        #if PEOPLECOUNTER_DEBUG
          Log.infoln("[Line 55]: SEQUENCE [SIZE = %i]: [%i] <--- %i", stateStack.count(), stateStack.peekIndex(0), newOccupancyState);
        #endif                                           
        break;
      case 1:
        stateStack.push(newOccupancyState);                  // Push to the stack without checking for impossibilities
        #if PEOPLECOUNTER_DEBUG
          Log.infoln("[Line 61]: SEQUENCE [SIZE = %i]: [%i, %i] <--- %i", stateStack.count(), stateStack.peekIndex(0), stateStack.peekIndex(1), newOccupancyState);
        #endif
        break;                                                           
      case 2:                                                     // When the stateStack has 2 or 3 items, we must identify impossible patterns and fix them - or backtrack.
      case 3:                                                               // [0, 1] <-- 2 becomes [0, 1, 3, 2],  [0, 2] <-- 1 becomes [0, 2, 3, 1],
        applyImpossibleStateTransitionCorrections(newOccupancyState);  // [0, 3] <-- 2 becomes [0, 1, 3, 2],  [0, 3] <-- 1 becomes [0, 2, 3, 1], 
        #if PEOPLECOUNTER_DEBUG                                             // [0, x, 3] <-- x becomes [0, x],     [0, 3, x] <-- 3 becomes [0, 3],     [0, x, 3] <-- 0 becomes [0].  
          switch (stateStack.count()){                                      // Anything not in need of corrections is added to the stack as normal.
            case 1:
              Log.infoln("[Line 70]: SEQUENCE [SIZE = %i]: [%i] <--- %i", stateStack.count(), stateStack.peekIndex(0), newOccupancyState);
              break;
            case 2:
              Log.infoln("[Line 73]: SEQUENCE [SIZE = %i]: [%i, %i] <--- %i", stateStack.count(), stateStack.peekIndex(0), stateStack.peekIndex(1), newOccupancyState);
              break;
            case 3:
              Log.infoln("[Line 76]: SEQUENCE [SIZE = %i]: [%i, %i, %i] <--- %i", stateStack.count(), stateStack.peekIndex(0), stateStack.peekIndex(1), stateStack.peekIndex(2), newOccupancyState);
              break;
            case 4:
              Log.infoln("[Line 79]: SEQUENCE [SIZE = %i]: [%i, %i, %i, %i] <--- %i", stateStack.count(), stateStack.peekIndex(0), stateStack.peekIndex(1), stateStack.peekIndex(2), stateStack.peekIndex(3), newOccupancyState);
              break;
          }
        #endif
        break;
      case 4:
        if(newOccupancyState != 0 && newOccupancyState != impossibleStateTransition[stateStack.peek()]){  // If the final occupancy state is NOT 0 AND is not impossible, backtrack ...
          while(stateStack.peek() != newOccupancyState){                                                      // ... until the top of the stack is equal to the new occupancy state ...
              stateStack.pop();                                                                                     // ... we remove the top of the stack.
          }
          #if PEOPLECOUNTER_DEBUG
            switch (stateStack.count()){
              case 1:
                Log.infoln("[Line 92]: SEQUENCE [SIZE = %i]: [%i] <--- %i", stateStack.count(), stateStack.peekIndex(0), newOccupancyState);
                break;
              case 2:
                Log.infoln("[Line 95]: SEQUENCE [SIZE = %i]: [%i, %i] <--- %i", stateStack.count(), stateStack.peekIndex(0), stateStack.peekIndex(1), newOccupancyState);
                break;
              case 3:
                Log.infoln("[Line 98]: SEQUENCE [SIZE = %i]: [%i, %i, %i] <--- %i", stateStack.count(), stateStack.peekIndex(0), stateStack.peekIndex(1), stateStack.peekIndex(2), newOccupancyState);
                break;
              case 4:
                Log.infoln("[Line 101]: SEQUENCE [SIZE = %i]: [%i, %i, %i, %i] <--- %i", stateStack.count(), stateStack.peekIndex(0), stateStack.peekIndex(1), stateStack.peekIndex(2), stateStack.peekIndex(3), newOccupancyState);
                break;
            }
          #endif        
        } else {                                                  // If the new occupancy state is 0 ...
          (newOccupancyState == impossibleStateTransition[stateStack.peek()]) ? stateStack.push(0) : stateStack.push(newOccupancyState);  // ... push the final state.
        }
        break;
    }
  }
  
  if(stateStack.count() == 5){                                    // If the stack is finished ...
    char states[56];                                              // ... turn it into a string by popping all values off the stack...
      #if PEOPLECOUNTER_DEBUG
        Log.infoln("[Line 115]: SEQUENCE [SIZE = %i]: [%i, %i, %i, %i, %i] <--- %i", stateStack.count(), stateStack.peekIndex(0), stateStack.peekIndex(1), stateStack.peekIndex(2), stateStack.peekIndex(3), stateStack.peekIndex(4), newOccupancyState);
      #endif 
      snprintf(states, sizeof(states), "%i%i%i%i%i", stateStack.pop(), stateStack.pop(), stateStack.pop(), stateStack.pop(), stateStack.pop());   
      if(strcmp(states, "01320")){
        if(!MOUNTED_INSIDE){                                      // ... and if the sequence (backwards) matches the increment sequence then increment the count.
          current.dailyCount++;
          current.hourlyCount++;
          currentData.currentDataChanged = true;
        } else {
          if(current.hourlyCount > 0 || !SINGLE_ENTRANCE){
            current.dailyCount--;
            current.hourlyCount--;
            currentData.currentDataChanged = true;
          }
        }
        #if TENFOOTDISPLAY
            printBigNumbers(current.hourlyCount);
        #endif       
        return true;                    
      } else if(strcmp(states, "02310")) {
        if(!MOUNTED_INSIDE){                                      // ... and if the sequence (backwards) matches the decrement sequence then decrement the count.
          if(current.hourlyCount > 0 || !SINGLE_ENTRANCE){
            current.dailyCount--;
            current.hourlyCount--;
            currentData.currentDataChanged = true;
          }
        } else {
          current.dailyCount++;
          current.hourlyCount++;
          currentData.currentDataChanged = true;
        }
        #if TENFOOTDISPLAY
            printBigNumbers(current.hourlyCount);
        #endif
        return true;
      } else {
        Log.infoln("ERROR: Algorithm somehow produced states: %s", states);     // ... if the sequence does not match the decrement or increment sequence, do nothing.
      }
  }
  #if OCCUPANCYSTATE_DEBUG
      if (current.occupancyState != stateStack.peek() && current.occupancyState != 255) printBigNumbers(current.occupancyState);
  #endif
  current.occupancyState = stateStack.peek();                                   // set the current occupancyState to the topmost value on the stack. (post correction)
  return false;
}


int PeopleCounter::getCount(){
  Log.infoln("Occupancy count is %d",current.hourlyCount);
  return current.hourlyCount;

}

void PeopleCounter::setCount(int value){
  current.hourlyCount = value;
}

int PeopleCounter::getLimit(){
  return occupancyLimit;
}

void PeopleCounter::setLimit(int value){
  occupancyLimit = value;
}

void PeopleCounter::applyImpossibleStateTransitionCorrections(int newOccupancyState){
  int needsCleanup = 0;
  tempStack.push(newOccupancyState);                              // Push the new occupancyState to the tempStack
  while(stateStack.count() > 1){                                  // Go through the stack containing prior states
    int currentState = stateStack.pop();                               
    int stateAfter = tempStack.peek();
    int stateBefore = stateStack.peek();
    if(stateBefore == stateAfter){                                // If the state before current is equal to the state after current ...
      stateStack.push(currentState);                              // ... put current back
      needsCleanup = 1;
      break;
    }
    if(impossibleStateTransition[stateBefore] == currentState){          // If the transition from stateBefore --> the new state is impossible, we must have failed to detect the person at some point ...
      tempStack.push(currentState);                                            // ... so push current ...
      int missedState = impossibleStateTransition[stateAfter];                       // ... consult the impossible/magical transition map to determine what state was missed ...
      tempStack.push(missedState);                                                          // ... and push that missing state into the stack to correct the sequence.
    } else if(impossibleStateTransition[currentState] == stateAfter) {   // If the transition from the new state --> after is impossible, we must have failed to detect the person at some point ...
      int missedState = impossibleStateTransition[stateBefore];                // ... so consult the impossible/magical transition state map to determine what state was missed ...
      tempStack.push(missedState);                                                   // ... push that missing state into the stack to correct the sequence. ...
      tempStack.push(currentState);                                                         // ... then push the new state.
    } else {                                                             // If the transition from stateBefore --> the new state AND from the new state --> stateAfter are both possible ...
      tempStack.push(currentState);                                            // ... push the new state to the stack as normal.
    }
  }
  while(!tempStack.isEmpty()){                                    // Move everything in the tempStack back to the permanent stack
    stateStack.push(tempStack.pop());
  } 
  if(needsCleanup) { 
    do {                                                        
      stateStack.pop();                                           // Remove the top of the stack.
    } while (stateStack.peek() != newOccupancyState);             // ... until the top of the stack is equal to the new occupancy state
  } 
}

void PeopleCounter::printBigNumbers(int number) {
 StackArray <int> bigNumberStack;                                 // For printing big numbers
  Log.infoln("  ");
  int currentNumber = number;
  while(currentNumber % 10 != currentNumber) {
    bigNumberStack.push(abs(currentNumber % 10));
    currentNumber = currentNumber / 10;
  }
  bigNumberStack.push(currentNumber);
  int numPrinted = 0;
  String bigStrings1;
  String bigStrings2;
  String bigStrings3;
  String bigStrings4;
  String bigStrings5;
  String bigStrings6;
  String bigStrings7;
  if(number < 0){
    bigStrings1 += "      ";
    bigStrings2 += "      ";
    bigStrings3 += "      ";
    bigStrings4 += "------";  
    bigStrings5 += "      ";     
    bigStrings6 += "      ";
    bigStrings7 += "      ";
  }
  while(!bigNumberStack.isEmpty()){
    currentNumber = bigNumberStack.pop();
    switch (abs(currentNumber)) {
      case 0:
        bigStrings1 += "  0000  ";
        bigStrings2 += " 0    0 ";
        bigStrings3 += "0      0";
        bigStrings4 += "0      0";
        bigStrings5 += "0      0";
        bigStrings6 += " 0    0 ";
        bigStrings7 += "  0000  ";
      break;

      case 1:
        bigStrings1 += "    11  ";
        bigStrings2 += "   1 1  ";
        bigStrings3 += "     1  ";
        bigStrings4 += "     1  ";
        bigStrings5 += "     1  ";
        bigStrings6 += "     1  ";
        bigStrings7 += "   11111";
      break;

      case 2:
        bigStrings1 += "  2222  ";
        bigStrings2 += " 2    22";
        bigStrings3 += "     2  ";
        bigStrings4 += "   2    ";
        bigStrings5 += "  2     ";
        bigStrings6 += "22     2";
        bigStrings7 += "2222222 ";
        break;

        case 3:
        bigStrings1 += "  3333  ";
        bigStrings2 += " 3    3 ";
        bigStrings3 += "       3";
        bigStrings4 += "   333  ";
        bigStrings5 += "       3";
        bigStrings6 += " 3    3 ";
        bigStrings7 += "  3333  ";
        break;

        case 4:
        bigStrings1 += "4      4";
        bigStrings2 += "4      4";
        bigStrings3 += "4      4";
        bigStrings4 += "  4444  ";
        bigStrings5 += "       4";
        bigStrings6 += "       4";
        bigStrings7 += "       4";
        break;

        case 5:
        bigStrings1 += "  555555";
        bigStrings2 += " 5      ";
        bigStrings3 += " 555555 ";
        bigStrings4 += "      5 ";
        bigStrings5 += "       5";
        bigStrings6 += "      5 ";
        bigStrings7 += " 555555 ";
        break;

        case 6:
        bigStrings1 += "  666666";
        bigStrings2 += " 6      ";
        bigStrings3 += "6 66666 ";
        bigStrings4 += "6      6";
        bigStrings5 += "6      6";
        bigStrings6 += " 6    6 ";
        bigStrings7 += "  6666  ";
        break;

        case 7:
        bigStrings1 += "  777777";
        bigStrings2 += " 7     7";
        bigStrings3 += "      7 ";
        bigStrings4 += "     7  ";
        bigStrings5 += "    7   ";
        bigStrings6 += "   7    ";
        bigStrings7 += "  7     ";
        break;

        case 8:
        bigStrings1 += "  8888  ";
        bigStrings2 += " 8    8 ";
        bigStrings3 += "8      8";
        bigStrings4 += "  8888  ";
        bigStrings5 += "8      8";
        bigStrings6 += " 8    8 ";
        bigStrings7 += "  8888  ";
        break;

        case 9:
        bigStrings1 += " 99999  ";
        bigStrings2 += "9     9 ";
        bigStrings3 += "9      9";
        bigStrings4 += " 99999 9";
        bigStrings5 += "       9";
        bigStrings6 += "      9 ";
        bigStrings7 += " 999999 ";
        break;
    }
    numPrinted++;
  }
  Log.infoln(bigStrings1.c_str());
  Log.infoln(bigStrings2.c_str());
  Log.infoln(bigStrings3.c_str());
  Log.infoln(bigStrings4.c_str());
  Log.infoln(bigStrings5.c_str());
  Log.infoln(bigStrings6.c_str());
  Log.infoln(bigStrings7.c_str());
  Log.infoln("  ");
}
