#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H

#include <ArduinoLog.h>     // https://github.com/thijse/Arduino-Log

/**
 * Defines default behavior that all camera types must implement
*/
class CameraInterface {
public:

    /** 
     * Cameras are Singletons (cannot enforce here), so 
     * be sure to implement instance() functionality in 
     * new singletons that implement this interface.
     */

    virtual ~CameraInterface() {}

    /**
     * @brief Perform camera setup operations
     */ 
    virtual bool setup() = 0;
    
    /**
     * @brief Reads data from the camera using an AssetConnection (like SerialConnection)
     */
    virtual bool readData() = 0;
};

#endif // CAMERA_INTERFACE_H