#ifndef __ACCELEROMETER_H
#define __ACCELEROMETER_H

#include "Assets/AssetInterface.h"
#include <ArduinoLog.h>
#include "MyData.h"
#include "Config.h"
#include "stsLED.h"
#include "ModMMA8452Q.h"
#include "timing.h"

/**
 * OpenMV H7 Plus camera - Singleton that implements CameraInterface
 */
class Accelerometer : public AssetInterface {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use Accelerometer::instance() to instantiate the singleton.
     */
    static Accelerometer& instance();

    /**
     * @brief Performs setup operations for the OpenMV H7 Plus camera.
     */
    bool setup() override;
 
    /**
     * @brief Reads data from an OpenMV H7 Plus, using a SerialConnection
     */
    bool readData() override;

protected:
    Accelerometer();
    
    virtual ~Accelerometer();
    
    Accelerometer(const Accelerometer&) = delete;

    Accelerometer& operator=(const Accelerometer&) = delete;

    static Accelerometer *_instance;
};

#endif  /* __ACCELEROMETER_H */