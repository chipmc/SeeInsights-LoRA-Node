// Presence Detection
// Author: Chip McClelland
// Date: May 2024
// License: GPL3
// In this class, we look at the accelerometer tap detection interrupt and see if we sense presence

#ifndef __PRESENCE_H
#define __PRESENCE_H

#include <arduino.h>
#include <ArduinoLog.h>
#include "MyData.h"
#include "Config.h"
#include "ModMMA8452Q.h"
#include "stsLED.h"

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * peopleCounter::instance().setup();
 * 
 * From global application loop you must call:
 * peopleCounter::instance().loop();
 */
class Presence {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use peopleCounter::instance() to instantiate the singleton.
     */
    static Presence &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use peopleCounter::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use peopleCounter::instance().loop();
     */
    bool loop();

protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use peopleCounter::instance() to instantiate the singleton.
     */
    Presence();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~Presence();

    /**
     * This class is a singleton and cannot be copied
     */
    Presence(const Presence&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    Presence& operator=(const Presence&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static Presence *_instance;

};
#endif  /* __PRESENCE_H */
