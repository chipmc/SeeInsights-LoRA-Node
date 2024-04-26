#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H

#include <ArduinoLog.h>     // https://github.com/thijse/Arduino-Log

class CameraInterface {
public:
    virtual ~CameraInterface() {}

    virtual bool setup() = 0;
    
    virtual bool readCount() = 0;
};

#endif // CAMERA_INTERFACE_H