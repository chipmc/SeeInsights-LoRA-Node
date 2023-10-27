/**
 * @file take_measurements.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This is the top-level class for collecting data when the device awakes  
 * The libraries and functions needed will depend the spectifics of the device and sensors
 * Sensors that are on the carrier board will be taken care of in this class
 * Sensors that are attached will 
 * 
 * @version 0.1
 * @date 2023-1015
 * 
 */

// Particle functions
#ifndef TAKE_MEASUREMENTS_H
#define TAKE_MEASUREMENTS_H

//Include standard header files from libraries:
#include <arduino.h>
#include <ArduinoLog.h>
#include "Adafruit_MAX1704X.h"
#include "Adafruit_SHT31.h"                             // Class instance for SHT31-D temperature and humidity sensor
#include "MyData.h"
#include "stsLED.h"
#include "timing.h"
#include "ErrorCodes.h"
#include "TofSensor.h"
#include "PeopleCounter.h"

#define measure take_measurements::instance()

class take_measurements {
public:

    char internalTempStr[16];                       // External as this can be called as a Particle variable
    char signalStr[64];

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use pinout::instance() to instantiate the singleton.
     */
    static take_measurements &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use pinout::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use pinout::instance().loop();
     */
    bool loop();

    /**
     * @brief This code collects basic data from the default sensors - temperature, humidity, battery and charge level
     * 
     * @details These are the sensors that are on the carrier board.
     * 
     * @returns Returns true if succesful and puts the data into the current object
     * 
     */
    bool takeMeasurements();                               // Function that calls the needed functions in turn

    /**
     * @brief Measures the temperature and humidity
     * 
     * @details generic code that converts the analog value of the TMP-36 sensors to degrees c - Assume 3.3V Power
     * 
     * @returns Returns true unless there is an issue with the sensor
     * 
     */
    bool getTemperatureHumidity();                        // Temperature from the tmp36 - inside the enclosure


    /**
     * @brief In this function, we will measure the battery state of charge and the current functional state
     * 
     * @details One factor that is an issue today is the accurace of the state of charge if the device is waking
     * from sleep.  In order to help with this, there is a test for enable sleep and an additional delay.
     * 
     * @return true  - If the battery has a charge over 60%
     * @return false - Less than 60% indicates a low battery condition
     */
    bool batteryState();                                   // Data on state of charge and battery status. Returns true if SOC over 60%

    /**
     * @brief Checks to see if the temperature is in the range to support charging
     * 
     * @details Will enable or disable charging based on the current temperature
     * 
     * @link https://batteryuniversity.com/learn/article/charging_at_high_and_low_temperatures @endlink
     * 
     */
    bool isItSafeToCharge();                               // See if it is safe to charge based on the temperature

    /**
     * @brief This function is called once a hardware interrupt is triggered by the device's sensor
     * 
     * @details The sensor may change based on the settings in sysSettings but the overall concept of operations
     * is the same regardless.  The sensor will trigger an interrupt, which will set a flag. In the main loop
     * that flag will call this function which will determine if this event should "count" as a visitor.
     * 
     * @returns true so we can reset the interrupt flag
     * 
     */
    bool recordCount();

    protected:

       /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use pinout::instance() to instantiate the singleton.
     */
    take_measurements();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~take_measurements();

    /**
     * This class is a singleton and cannot be copied
     */
    take_measurements(const take_measurements&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    take_measurements& operator=(const take_measurements&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static take_measurements *_instance;
};
#endif