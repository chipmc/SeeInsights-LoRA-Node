#include "timing.h"
#include "config.h"

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

  Log.traceln("");
  Log.traceln("************************* TIMERS **************************");
  // ab1805.resetConfig();
  ab1805.withFOUT(gpio.IRQ).setup();
  ab1805.setPPMAdj(cfg.PPM_Adj); 
  ab1805.setWDT(ab1805.WATCHDOG_MAX_SECONDS);
  // ab1805.setRtcFromTime(1694699857+60);  // Set the time - This will come from the LoRA Gateway
  ab1805.getRtcAsTime(time_cv,hundrths_cv);

  Log.infoln("AB1805 Initialized with Unix time of: %l", time_cv);

  Log.traceln("");
  Log.traceln("************************* TIMERS END**************************");
}

void timing::loop() {
    static uint32_t lastTime = 0;

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
bool timing::setTime(){
 
 // Need to fill this in once I have the clock working

  return true;
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

void timing::deepPowerDown(uint16_t seconds){
  ab1805.deepPowerDown(seconds);
}