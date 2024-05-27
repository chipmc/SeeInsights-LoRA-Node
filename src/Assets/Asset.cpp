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
            Log.infoln("Asset initialized as OpenMVH7Plus - Asset::setup()");
            break;
        // Create other AssetInterface implementors here
        default:
            return false; // Unsupported sensorType
    }
    
    // Perform setup operations on the _asset instance referenced in this class
    if (!_asset->setup()) {
        // Setup failed
        delete _asset;
        _asset = nullptr;
        Log.infoln("Failed to initialize AssetInterface instance - Asset::setup()");
        return false;
    }
    
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
