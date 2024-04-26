#include "MachineVisionCamera.h"
#include "./OpenMV-H7-Plus/OpenMVH7Plus.h"

// Declare instance as null
MachineVisionCamera* MachineVisionCamera::_instance = nullptr;

// Constructor - initially, the camera (private pointer to CameraInstance object) points to nothing
MachineVisionCamera::MachineVisionCamera() : camera(nullptr) {}

// Destructor - also deletes the camera pointer
MachineVisionCamera::~MachineVisionCamera() {
    delete camera; // Release memory
}

// Singleton pattern [static]
MachineVisionCamera& MachineVisionCamera::instance() {
    if (!_instance) {
        _instance = new MachineVisionCamera();
    }
    return *_instance;
}

// Setup function
bool MachineVisionCamera::setup(int sensorType) {
    // If camera instance already exists, delete it before creating a new one
    if (camera) {
        delete camera;
        camera = nullptr;
    }
    
    // Instantiate the appropriate camera instance based on sensorType
    switch (sensorType) {
        case 0:
            camera = &OpenMVH7Plus::instance(); // Get singleton instance
            break;
        // Add other camera types here
        default:
            return false; // Unsupported sensorType
    }
    
    // Setup the camera instance
    if (!camera->setup()) {
        // Setup failed
        delete camera;
        camera = nullptr;
        return false;
    }
    
    return true;
}

bool MachineVisionCamera::readCount() {
    if (!camera) {
        return false;
    }
    return camera->readCount();
}
