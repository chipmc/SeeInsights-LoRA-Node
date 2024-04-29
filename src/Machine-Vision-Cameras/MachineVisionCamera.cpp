#include "MachineVisionCamera.h"

// Declare instance as null
MachineVisionCamera* MachineVisionCamera::_instance = nullptr;

// Constructor - initially, the _camera (reference pointer to CameraInterface implementor) points to nothing
MachineVisionCamera::MachineVisionCamera() : _camera(nullptr) {}

// Destructor - also deletes the CameraInterface implementor reference
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

bool MachineVisionCamera::setup(int sensorType) {
    // If reference to a CameraInterface instance already exists, delete it before creating a new one
    if (_camera) {
        delete _camera;
        _camera = nullptr;
    }
    
    // Instantiate the appropriate CameraInterface instance based on sensorType
    switch (sensorType) {
        case 0: // TODO:: Create sensorTypes for machine vision cameras
            _camera = &OpenMVH7Plus::instance(); // Get reference to singleton instance
            break;
        // Create other CameraInterface implementors here
        default:
            return false; // Unsupported sensorType
    }
    
    // Perform setup operations on the _camera instance referenced in this class
    if (!_camera->setup()) {
        // Setup failed
        delete _camera;
        _camera = nullptr;
        Log.infoln("Failed to initialize CameraInterface instance - MachineVisionCamera::setup()");
        return false;
    }
    
    Log.infoln("MachineVisionCamera successfully initialized - MachineVisionCamera::setup()");
    return true;
}

bool MachineVisionCamera::readData() {
    if (!_camera) {
        Log.infoln("Failed to read data, _camera is null - MachineVisionCamera::readCount()");
        return false;
    }
    return _camera->readData();
}
