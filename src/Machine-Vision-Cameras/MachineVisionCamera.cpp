#include "MachineVisionCamera.h"
#include "./OpenMV-H7-Plus/OpenMVH7Plus.h"

// [static]
MachineVisionCamera* MachineVisionCamera::_instance = nullptr;

// Constructor
MachineVisionCamera::MachineVisionCamera() : cameraInstance(nullptr) {}

// Destructor
MachineVisionCamera::~MachineVisionCamera() {
    delete cameraInstance;
}

// Singleton instance getter
MachineVisionCamera& MachineVisionCamera::instance() {
    if (!_instance) {
        _instance = new MachineVisionCamera();
    }
    return *_instance;
}

// Setup function
bool MachineVisionCamera::setup(int sensorType) {
    // If camera instance already exists, delete it before creating a new one
    if (cameraInstance) {
        delete cameraInstance;
        cameraInstance = nullptr;
    }
    
    // Instantiate the appropriate camera instance based on sensorType
    switch (sensorType) {
        case 0:
            cameraInstance = &OpenMVH7Plus::instance(); // Get singleton instance
            break;
        // Add other camera types here
        default:
            return false; // Unsupported sensorType
    }
    
    // Setup the camera instance
    if (!cameraInstance->setup()) {
        // Setup failed
        delete cameraInstance;
        cameraInstance = nullptr;
        return false;
    }
    
    return true;
}

bool MachineVisionCamera::capture() {
    if (!cameraInstance) {
        return false;
    }
    return cameraInstance->capture();
}
