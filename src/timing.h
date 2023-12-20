/**
 * @file    timing.h
 * @author  Jeff Skarda (mapleiotsolutions@gmail.com)
 * @brief   All functions that are used to estalish and update time and overall event sequencing as it pertains to time
 * @details 
 * @version 0.1
 * @date    2022-09-01
 * 
 * Version History:
 * 0.1 - Initial realease 
 * 
 */

//Include a standard header guard
#ifndef __TIMING_H
#define __TIMING_H

//Include standard header files from libraries:
#include <arduino.h>
#include <AB1805_RK.h>          // https://github.com/mapleiotsolutions/AB1805_RK_Arduino

//Include application specific header files
#include "pinout.h"

//Macros(#define) to swap out during pre-processing (use sparingly)
#define timeFunctions timing::instance()
#define eventFlag_curDvcRpt 0
#define eventFlag_rptEnd 1
#define eventFlag_nxtRptStrt 2
#define eventFlag_nxtDvcRpt 3


/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * timing::instance().setup();
 * 
 * From global application loop you must call:
 * timing::instance().loop();
 */
class timing {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use timing::instance() to instantiate the singleton.
     */
    static timing &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use timing::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use timing::instance().loop();
     */
    void loop();

    /*
    * @brief Call this to update variables required for timing 
    * 
    */
    bool update();

    /**
     * @brief set the time based on the value we recieved from the LoRa Gateway
     */
    bool setTime(time_t UnixTime, uint8_t hundredths);

    /**
     * @brief - Get the time in UNITX Time format - GMT
    */
   time_t getTime();

    /**
     * @brief set an interrupt for a future time based on an event type
     * 
     * @details This is used to set a specific interrupt type at an event in the future
     * This is will first calculate the duration of time to the event of interest and then set an interrupt on the AB1805
     * 
     * @param eventType 
     * Available Event Types are:
     *      eventFlag_nxtRptStrt - Set an interrupt when the next reporting window starts
     *      eventFlag_nxtDvcRpt - Set an interrupt when this device should report it's data
     */
    void interruptAtEvent(uint8_t eventType);

    /**
     * @brief Clear any repeating interrupt
     * 
     * @details This is used to clear any repeating interrupts that are present.
     */
    void clearRepeatingInterrupt();

    /**
     * @brief Set an interrupt at a specific time
    */
    void interruptAtTime(time_t UnixTime, uint8_t hundredths);

    /**
     * @brief set the time based on the value we recieved from the LoRa Gateway
     */
    void stopWDT();

    /**
     * @brief set the time based on the value we recieved from the LoRa Gateway
     */
    void resumeWDT();

    /**
     * @brief Pet the watchdog 
     */
    void setWDT(int seconds = -1);

    /**
     * @brief returns true if the RTC is set, otherwise returns false
     */
    bool isRTCSet();

    /**
     * @brief deep Sleep the processor using the AB1805 for ultra low power
     */
    void deepPowerDown(uint16_t seconds=30);



    uint16_t    nxtRptStrt_sec      = 60;   // Number of seconds to the next reporting start period
    uint16_t    prevRptStrt_sec     = 0;    // Number of seconds since the start of the last reporting start 
    uint16_t    RxTimeComp_ms;              // Total time compensation due to transmission delays (retransmissions, hops, transmit time, timeouts, etc.)
    uint8_t     Rx_Hundrths_Comp;           // Hundreths value after compensation
    uint32_t    Rx_Time_Comp;               // UNIX time value after compensation 
    uint32_t    dvcRpt_Millis;              // Millis when the device should report it's own readings
    uint32_t    rptEnd_Millis;              //  Millis when reporting finished for all nodes
    uint32_t    nxtRptStrt_time; 
    uint8_t     nxtRptStrt_hund;
    uint32_t    nxtDvcRpt_time;
    uint8_t     nxtDvcRpt_hund;
    uint32_t    rptEnd_time;
    uint8_t     rptEnd_hund;
    uint32_t    curDvcRpt_time;
    uint8_t     curDvcRpt_hund;
    uint32_t    nxtRptStart_Millis = nxtRptStrt_sec * 1000;
    time_t      time_cv;
    uint8_t     hundrths_cv;
    uint32_t    WDT_MaxSleepDuration = 113000;  // This is the maximum sleep duration before the watchdog must be pet. 


protected:
    const uint16_t  RTC_Deadband_ms = 20; // The deadband correction 

    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use timing::instance() to instantiate the singleton.
     */
    timing();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~timing();

    /**
     * This class is a singleton and cannot be copied
     */
    timing(const timing&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    timing& operator=(const timing&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static timing *_instance;

};
#endif  /* __TIMING_H */