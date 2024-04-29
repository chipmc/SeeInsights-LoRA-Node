// Machine Vision Camera Class - Bridge Pattern
// Author: Alex Bowen
// Date: May 2023
// License: GPL3
// This is an instance of any machine vision camera, where interactions with the camera
// are 

#ifndef __MACHINE_VISION_CAMERA_H
#define __MACHINE_VISION_CAMERA_H

#include "CameraInterface.h"
#include "./OpenMV-H7-Plus/OpenMVH7Plus.h"
// Add other camera types here if needed


/** 
 * Bridge Pattern - decouples this abstraction from CameraInterface implementations.
 * 
 * Single class that initializes and interacts with any type of CameraInterface implementation. 
 * 
 * Initializes the CameraInterface implementor represented by sysStatus.sensorType and
 * delegates all work to that implementor, serving as an abstraction that enforces uniform 
 * usage of all CameraInterface implementations within the main loop.
 * 
 * 
 * Example Usage:
 * 
 * MachineVisionCamera::instance().setup(1)    // setup with sensorType 1 (let's pretend this is a type of camera)
 * MachineVisionCamera::instance().readData()  // reads data from camera type 1 (in accordance with its CameraInterface implementation)
 * 
 * MachineVisionCamera::instance().setup(2)    // setup with sensorType 2 (let's pretend this is a type of camera)
 * MachineVisionCamera::instance().readData()  // reads data from camera type 2 (in accordance with its CameraInterface implementation)
 * 
*/
class MachineVisionCamera {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use MachineVisionCamera::instance() to instantiate the singleton.
     */
    static MachineVisionCamera &instance();

    /** 
     * @brief Instantiates the appropriate CameraInterface implementor based on sysStatus.sensorType,
     * then performs the setup operations on that camera.
     *
     * Initializes camera instances as a 'CameraFactory' class normally would, for simplicity.
     * 
     * MachineVisionCamera::instance().setup(int sensorType)
     */ 
    bool setup(int sensorType);

    /** 
     * @brief Performs the readData operation on the member CameraInterface implementor reference.
     *
     * MachineVisionCamera::instance().readData()
     */ 
    bool readData();

protected:
    MachineVisionCamera();

    virtual ~MachineVisionCamera();

    MachineVisionCamera(const MachineVisionCamera&) = delete;

    MachineVisionCamera& operator=(const MachineVisionCamera&) = delete;

    static MachineVisionCamera *_instance;

private:
    /** 
     * @brief References a Camera instance - for decoupled interactions
     */
    CameraInterface *_camera;
};

#endif  /* __MACHINE_VISION_CAMERA_H */
