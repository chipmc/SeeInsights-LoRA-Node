/**
 * @file    sensors.h
 * @author  Chip McClelland version of original by Jeff Skarda (mapleiotsolutions@gmail.com)
 * @brief   All functions that are used to init and take readings from sensors
 * @details 
 * @version 0.1
 * @date    2023-10-17
 * 
 * Version History:
 * 0.1 - Initial realease 
 * For starters, we will use the Adafruit MAX17048 for battery monitoring and the SHT31-D for temperature and humidity
 */

//Include a standard header guard
#ifndef __SENSORS_H
#define __SENSORS_H

//Include standard header files from libraries:
#include <arduino.h>
#include <ArduinoLog.h>
#include "Adafruit_MAX1704X.h"
#include "Adafruit_SHT31.h"                 // Class instance for SHT31-D temperature and humidity sensor

//Include application specific header files
#include "pinout.h"

//Macros(#define) to swap out during pre-processing (use sparingly)
#define sns sensors::instance()

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * sensors::instance().setup();
 * 
 * From global application loop you must call:
 * sensors::instance().loop();
 */
class sensors {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use sensors::instance() to instantiate the singleton.
     */
    static sensors &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use sensors::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use sensors::instance().loop();
     */
    void loop();

    /**
     * @readBatteryVoltage() returns the battery voltage in Voles
    */
    int16_t readBattery();



protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use sensors::instance() to instantiate the singleton.
     */
    sensors();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~sensors();

    /**
     * This class is a singleton and cannot be copied
     */
    sensors(const sensors&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    sensors& operator=(const sensors&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static sensors *_instance;
        
    /**
     * @brief Call this instead of delay. This way we can still process LoRa messages by calling
     * 
     * @param t ms to delay while still processing LoRa Messages
     * 
     */
    inline void softDelay(uint32_t t);

    /**
     * @brief select which I2C sensor we want to read from before we read it
     * 
     * @param i the selector of which one to read from (0-7)
     * 
     */
    void muxSel(uint8_t i);

protected:
    const uint8_t   ABPAddress      = 0x38;
    const float     OUTPUT_MIN      = 1638.4;       // 1638 counts (10% of 2^14 counts or 0x0666)
    const float     OUTPUT_MAX      = 14745.6;      // 14745 counts (90% of 2^14 counts or 0x3999)
    const float     ABPPRESSURE_MIN = 0;       // min is 0 psi
    const float     ABPPRESSURE_MAX = 15.0;    // max is 15 psi

};
#endif  /* __SENSORS_H */