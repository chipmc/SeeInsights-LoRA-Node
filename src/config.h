/**
 * @file    config.h
 * @author  Jeff Skarda (mapleiotsolutions@gmail.com)
 * @brief   All functions that are used to handle configuration
 * @details The library will read from EEPROM to set and update various configuration parameters
 * @version 0.1
 * @date    2022-09-01
 * 
 * Version History:
 * 0.1 - Initial realease 
 * 
 */

//Include a standard header guard
#ifndef __CONFIG_H
#define __CONFIG_H

//Include standard header files from libraries:
#include <arduino.h>
#include <ArduinoLog.h>
#include <JC_EEPROM.h>

//Include application specific header files
#include "pinout.h"


//Macros(#define) to swap out during pre-processing (use sparingly)
#define cfg config::instance()

#define tempFVac1 100
#define tempFVac1Vac2 101
#define tempFVac1Level 102
#define tempFLevel 103
#define tempFVac1Vac2Vac3 104
#define tempFVac1Vac2Vac3Vac4 105

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * config::instance().setup();
 * 
 * From global application loop you must call:
 * config::instance().loop();
 */
class config {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use config::instance() to instantiate the singleton.
     */
    static config &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use config::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use config::instance().loop();
     */
    void loop();

    uint16_t            devID;                                  // Define the default device ID. This is unique for every device (1 to 65,535). It can be updated via a join request. 
    uint8_t             devFeatures;                            // Integer value representing the faetures installed on this device
    char                devSerial[10];                          // Define the device serial number. This is stored in EEPROM
    uint8_t             pcbVersion;                             // Read the PCB Version from EEPROM
    uint8_t             pcbLiPoFuel;                             // Read the PCB Version from EEPROM
    uint8_t             pcbI2CMUX;                             // Read the PCB Version from EEPROM
    int16_t             PPM_Adj;                                // The adjusted PPM value to sync with the GPS 1 PPS signal. Read from EEPROM.
    unsigned char       encryptkey[16];                         // Value of the Encryption key. This is typically read from EEPROM
    const uint8_t       firmVersion          = 6;               // The current version of firmware (0-255)
    uint8_t             hub_address          = 0;               // The static hub address
    uint8_t             meshNode;                               // Variable to hold the status of being a mesh node
    uint8_t             freqSelect           = 0;               // What is the selected frequency for LoRa Transmissions based on DIP switch 2, 3, 4 (8 channels available)

protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use config::instance() to instantiate the singleton.
     */
    config();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~config();

    /**
     * This class is a singleton and cannot be copied
     */
    config(const config&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    config& operator=(const config&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static config *_instance;

};
#endif  /* __CONFIG_H */