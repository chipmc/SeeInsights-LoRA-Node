// Machine Vision Camera Class
// Author: Alex Bowen
// Date: May 2023
// License: GPL3
// This is an instance of a machine vision camera, where the type of camera is defined 

#ifndef __MACHINE_VISION_CAMERA_H
#define __MACHINE_VISION_CAMERA_H

#include "CameraInterface.h"

class MachineVisionCamera {
public:
    static MachineVisionCamera &instance();

    bool setup(int sensorType);

    bool readCount();

protected:
    MachineVisionCamera();

    virtual ~MachineVisionCamera();

    MachineVisionCamera(const MachineVisionCamera&) = delete;

    MachineVisionCamera& operator=(const MachineVisionCamera&) = delete;

    static MachineVisionCamera *_instance;

private:
    CameraInterface *camera;  // Contains a Camera instance to interact with
};

#endif  /* __MACHINE_VISION_CAMERA_H */
