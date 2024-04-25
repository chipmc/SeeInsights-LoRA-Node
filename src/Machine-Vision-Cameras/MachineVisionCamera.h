// Machine Vision Camera Class
// Author: Alex Bowen
// Date: May 2023
// License: GPL3
// This is an instance of a machine vision camera, where the type of camera is defined 

#ifndef __MACHINEVISIONCAMERA_H
#define __MACHINEVISIONCAMERA_H

// Base Class for Camera Instances
class CameraInstance {
public:
    virtual bool setup() = 0;
    virtual bool capture() = 0;
    virtual ~CameraInstance() {}
};

class MachineVisionCamera {
public:
    static MachineVisionCamera &instance();

    bool setup(int sensorType);

    bool capture();

protected:
    MachineVisionCamera();

    virtual ~MachineVisionCamera();

    MachineVisionCamera(const MachineVisionCamera&) = delete;

    MachineVisionCamera& operator=(const MachineVisionCamera&) = delete;

    static MachineVisionCamera *_instance;

private:
    CameraInstance *cameraInstance;
};

#endif  /* __MACHINEVISIONCAMERA_H */
