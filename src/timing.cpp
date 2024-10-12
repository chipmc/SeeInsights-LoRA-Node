#include "timing.h"

AB1805 ab1805(Wire); // Class instance for the the AB1805 RTC

timing *timing::_instance;

// [static]
timing &timing::instance() {
    if (!_instance) {
        _instance = new timing();
    }
    return *_instance;
}

timing::timing() {
}

timing::~timing() {
}

void timing::setup() {
  // ab1805.resetConfig();
  ab1805.withFOUT(gpio.WAKE).setup();
  ab1805.setWDT(ab1805.WATCHDOG_MAX_SECONDS);
  // ab1805.setRtcFromTime(1694699857+60);  // Set the time - This will come from the LoRA Gateway
  ab1805.getRtcAsTime(time_cv,hundrths_cv);
  Log.infoln("AB1805 Initialized with Unix time of: %l", time_cv);
}

void timing::loop() {
    // static uint32_t lastTime = 0;

    // Put your code to run during the application thread loop here
    ab1805.loop(); 

/*
    if (millis() - lastTime > 10000) {
        ab1805.getRtcAsTime(time_cv,hundrths_cv);
        lastTime = millis();
        Log.infoln("Time: %u.%u", (uint32_t)time_cv, hundrths_cv);
    }   
    */
    
}

/*******************************************************************************
 * Method Name: setTime()
 *******************************************************************************/
bool timing::setTime(time_t UnixTime, uint8_t hundredths){
  ab1805.setRtcFromTime(UnixTime,hundredths);
  if (ab1805.isRTCSet()) {
    Log.infoln("AB1805 is set to %l", UnixTime);
    return true;
  }
  else {
    Log.infoln("AB1805 is not set");
    return false;
  }
}

time_t timing::getTime() {

  time_t time_seconds;
  uint8_t hundredths;
  ab1805.getRtcAsTime(time_seconds, hundredths);

  return time_seconds;
}



/*******************************************************************************
 * Method Name: update()
 *******************************************************************************/
bool timing::update(){
 // Need to fill this in once I have the clock working

    return true;
}

/*******************************************************************************
 * Method Name: InterruptAtEvent()
 *******************************************************************************/
void timing::interruptAtEvent(uint8_t eventType){
  
 // Need to fill this in once I have the clock working

}

void timing::interruptAtTime(time_t UnixTime, uint8_t hundredths){
  ab1805.interruptAtTime(UnixTime,hundredths);
}

void timing::clearRepeatingInterrupt(){
  ab1805.clearRepeatingInterrupt();
}

void timing::stopWDT(){
  ab1805.stopWDT();
}

void timing::resumeWDT(){
  ab1805.resumeWDT();
}

void timing::setWDT(int seconds){
  ab1805.setWDT(seconds);
}

bool timing::isRTCSet(){
  return ab1805.isRTCSet();
}

bool timing::deepPowerDown(uint16_t seconds){

  // Instead of using the deepPowerDown method from the AB1805 library that limits it to 255 seconds, we will use the deepPowerDownTime method to put the device to sleep until a speciifc time in the future. 
  // Care must be taken to ensure the time is set correctly on the AB1805 before calling this method and that the time is always in the future. Otherwise, you risk going to sleep and never waking up. 
  if (!ab1805.isRTCSet()) {
    ab1805.setRtcFromTime((time_t)1, 0);
  }
  ab1805.getRtcAsTime(time_cv,hundrths_cv);
  return ab1805.deepPowerDownTime((uint32_t)time_cv + seconds, hundrths_cv);

  // This is the old method which is limited to 255 seconds
  // return ab1805.deepPowerDown(seconds);
}