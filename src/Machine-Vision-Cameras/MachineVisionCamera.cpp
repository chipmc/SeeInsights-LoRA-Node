#include "MachineVisionCamera.h"
#include "./OpenMV-H7-Plus/OpenMVH7Plus.h"

// Declare instance as null
MachineVisionCamera* MachineVisionCamera::_instance = nullptr;

// Constructor - initially, the _camera (private pointer to object implementing CameraInterface) points to nothing
MachineVisionCamera::MachineVisionCamera() : _camera(nullptr) {}

// Destructor - also deletes the _camera pointer
MachineVisionCamera::~MachineVisionCamera() {
    delete _camera; // Release memory
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
    // If _camera instance already exists, delete it before creating a new one
    if (_camera) {
        delete _camera;
        _camera = nullptr;
    }
    
    // Instantiate the appropriate _camera instance based on sensorType
    switch (sensorType) {
        case 0:
            _camera = &OpenMVH7Plus::instance(); // Get singleton instance
            break;
        // Add other _camera types here
        default:
            return false; // Unsupported sensorType
    }
    
    // Setup the _camera instance
    if (!_camera->setup()) {
        // Setup failed
        delete _camera;
        _camera = nullptr;
        Log.infoln("Failed to initialize Camera instance - MachineVisionCamera::setup()");
        return false;
    }
    
    Log.infoln("MachineVisionCamera successfully initialized - MachineVisionCamera::setup()");
    return true;
}

bool MachineVisionCamera::readCount() {
    if (!_camera) {
        Log.infoln("Failed to read count, _camera is null - MachineVisionCamera::readCount()");
        return false;
    }
    return _camera->readCount();
}
