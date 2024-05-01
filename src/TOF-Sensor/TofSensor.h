// Time of Flight Sensor Class
// Author: Chip McClelland
// Date: May 2023
// License: GPL3
// This is the class for the ST Micro VL53L1X Time of Flight Sensor
// We are using the Sparkfun library which has some shortcomgings
// - It does not implement distance mode medium
// - It does not give access to the factory calibration of the optical center

#ifndef __TOFSENSOR_H
#define __TOFSENSOR_H

#include "VL53L1X.h" // Install the Pololu VL53L1X library through PlatformIO

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * TofSensor::instance().setup();
 * 
 * From global application loop you must call:
 * TofSensor::instance().loop();
 */
class TofSensor {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use TofSensor::instance() to instantiate the singleton.
     */
    static TofSensor &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use TofSensor::instance().setup();
     */
    bool setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * This function will test for any change in occupancy in zone1 or zone2 and return true if finished
     * without error. Returns various error codes if something goes wrong during the loop.
     * 
     * @details Calls TofSensor::instance().measure() and changes the occupancy state if 
     * 
     * You typically use TofSensor::instance().loop();
     */
    int loop();

    /**
     * @brief Takes 2 consecutive and alternating measurements of distance for both SPAD optical zones and
     * stores them in a 2D array.
     * 
     * You typically use TofSensor::instance().measure();
     */
    int measure();

     /**
     * @brief Takes 1 measurement of distance using the entire 16x16 array of SPADS, at a lower polling
     * rate, to see if a person is underneath and we need to begin measuring normally.
     * 
     * You typically use TofSensor::instance().detect();
     */
    int detect();

    /**
     * @brief These functions will return the last signal distance measurement in mm for each of the zones.
     * 
     * These functions do not trigger an update, they simply return the current value
    */
    int getLastDistanceZone1();

    /**
     * @brief These functions will return the last distance measurement in mm for each of the zones.
     * 
     * These functions do not trigger an update, they simply return the current value
    */
    int getLastDistanceZone2();

    /**
     * @brief Function to return the current occupancy state
     * 
     * @details Uses BCD to assign the value (1 occupied/ 0 not occupied) / (zone1 - ones, zone2 - twos)
     * Some examples value = 1 (zone1 occupied, zone2 not occupied), value = 3 - (both zones occupied)
    */
    int getOccupancyState();

    /**
     * @brief This function is called as part of the startup process to ensure the sensor does not see any obstructions
     * 
     * @details Stores the maximum and minimum distances, designated as the "baseline" range, for each spad.
     * Protected as this should only be called from the TofSensor setup process.
     * 
    */
    bool performOccupancyCalibration();

    /**
     * @brief Calls performDetectionCalibration and performOccupancyCalibration
     * 
    */
    bool recalibrate();

private:

    /**
     * @brief Configures the distanceMode and the minimum timing budget on the sensor, given sysStatus.distanceMode
     * 
     * @param distanceMode the distanceMode in the sysStatus struct
     * @param zoneDepth the SPAD depth (through the door)
     * @param zoneWidth the SPAD width (across the door)
     * @param zoneOpticalCenter the numbered SPAD to use as the center of the zone being configured (see Config.h)
    */
    void configureSensor(uint8_t distanceMode, uint8_t zoneDepth, uint8_t zoneWidth, uint8_t zoneOpticalCenter);

protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use TofSensor::instance() to instantiate the singleton.
     */
    TofSensor();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~TofSensor();

    /**
     * This class is a singleton and cannot be copied
     */
    TofSensor(const TofSensor&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    TofSensor& operator=(const TofSensor&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static TofSensor *_instance;

    VL53L1X myTofSensor;                 // Only called from this class

};
#endif  /* __TOFSENSOR_H */
