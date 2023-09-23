/**
 * @file    Pinout.h
 * @author  Jeff Skarda (mapleiotsolutions@gmail.com)
 * @brief   All functions that are used to establish pin mapping and set pinout functionality 
 * @details 
 * @version 0.1
 * @date    2022-09-01
 * 
 * Version History:
 * 0.1 - Initial realease 
 * 
 */

//Include a standard header guard
#ifndef __PINOUT_H
#define __PINOUT_H

//Include standard header files from libraries:
#include <arduino.h>
#include <ArduinoLog.h>

//Include application specific header files
// ---NONE---

//Macros(#define) to swap out during pre-processing (use sparingly)
#define gpio pinout::instance()

/**********************************************************************************************************************
 * ******************************            Pinout Details                  ******************************************
 * 
 *
 * Replace ME - Add pinout documentation details here
 * 
***********************************************************************************************************************/

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * pinout::instance().setup();
 * 
 * From global application loop you must call:
 * pinout::instance().loop();
 */
class pinout {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use pinout::instance() to instantiate the singleton.
     */
    static pinout &instance();

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
    void loop();


protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use pinout::instance() to instantiate the singleton.
     */
    pinout();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~pinout();

    /**
     * This class is a singleton and cannot be copied
     */
    pinout(const pinout&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    pinout& operator=(const pinout&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static pinout *_instance;

public:
    //Define pins for the RFM9x:
    static const uint8_t RFM95_CS       = 4;
    static const uint8_t RFM95_RST      = 3;
    static const uint8_t RFM95_INT      = 2;
    static const uint8_t RFM95_DIO0     = A5;

    // //Define pins for the Sensors:
    static const uint8_t TankPower      = A1;
    static const uint8_t TankPin        = A2;
    static const uint8_t VacPwr         = A3;
    static const uint8_t VacSel         = A4;
    static const uint8_t deepRst        = A5;
    static const uint8_t VBATPIN        = A7;
    static const uint8_t Relay1         = 6;
    static const uint8_t Relay2         = 7;
    static const uint8_t DIP1           = 12;
    static const uint8_t DIP2           = 10;
    static const uint8_t DIP3           = 14;
    static const uint8_t DIP4           = 11;
    static const uint8_t DIP5           = 0;
    static const uint8_t DIP6           = 1;
    static const uint8_t CE             = 26;
    static const uint8_t IRQ            = 8;
    static const uint8_t PB_LT          = D05;
    static const uint8_t ONE_WIRE_BUS   = 9;

};
#endif  /* __PINOUT_H */