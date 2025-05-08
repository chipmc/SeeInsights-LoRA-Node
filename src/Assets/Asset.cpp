#include "Asset.h"

// Declare instance as null
Asset* Asset::_instance = nullptr;

// Constructor - initially, the _asset (reference pointer to AssetInterface implementor) points to nothing
Asset::Asset() : _asset(nullptr) {}

// Destructor - also deletes the AssetInterface implementor reference
Asset::~Asset() {
    delete _asset; // Release memory
}

// Singleton pattern [static]
Asset& Asset::instance() {
    if (!_instance) {
        _instance = new Asset();
    }
    return *_instance;
}

bool Asset::setup(int sensorType) {
    // If reference to a AssetInterface instance already exists, delete it before creating a new one
    if (_asset) {
        delete _asset;
        _asset = nullptr;
    }
    
    // Instantiate the appropriate AssetInterface instance based on sensorType
    switch (sensorType) {
        case 12:    // OpenMV Machine Vision Asset
            _asset = &OpenMVH7Plus::instance(); // Get reference to singleton instance
            Log.infoln("Initializing asset as OpenMVH7Plus - Asset::setup()");
            break;
        case 13:    // Accelerometer Asset
            _asset = &Accelerometer::instance(); // Get reference to singleton instance
            Log.infoln("Initializing asset as Accelerometer - Asset::setup()");
            break;
        // Create other AssetInterface implementors here
        default:
            return false; // Unsupported sensorType
    }
    
    // Perform setup operations on the _asset instance referenced in this class
    const int maxRetries = 3;  // Maximum number of setup attempts
    int retryCount = 0;

    while (retryCount < maxRetries) {
        if (_asset->setup()) {
            // Setup successful
            return true;
        }

        // Setup failed
        Log.errorln("Asset setup failed (attempt %d/%d). Retrying...", retryCount + 1, maxRetries);
        
        retryCount++;
        delay(2000);  // Wait before retrying
    }

    // If setup still fails after maxRetries, return failure
    Log.errorln("Failed to initialize AssetInterface instance after %d attempts - Asset::setup()", maxRetries);
    return false;
    
    Log.infoln("Asset successfully initialized - Asset::setup()");
    return true;
}

bool Asset::readData() {
    if (!_asset) {
        Log.infoln("Failed to read data, _asset is null - Asset::readCount()");
        return false;
    }
    return _asset->readData();
}
