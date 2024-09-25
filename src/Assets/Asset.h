// Machine Vision Camera Class - Bridge Pattern
// Author: Alex Bowen
// Date: May 2023
// License: GPL3
// This is an instance of any machine vision asset, where interactions with the asset
// are 

#ifndef __ASSET_H
#define __ASSET_H

#include "AssetInterface.h"
#include "./Machine-Vision-Cameras/OpenMV-H7-Plus/OpenMVH7Plus.h"
// Add other asset types here if needed


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
 * Asset::instance().setup(1)    // setup with sensorType 1 (let's pretend this is a type of asset)
 * Asset::instance().readData()  // reads data from asset type 1 (in accordance with its CameraInterface implementation)
 * 
 * Asset::instance().setup(2)    // setup with sensorType 2 (let's pretend this is a type of asset)
 * Asset::instance().readData()  // reads data from asset type 2 (in accordance with its CameraInterface implementation)
 * 
*/

#define asset Asset::instance()

class Asset {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use Asset::instance() to instantiate the singleton.
     */
    static Asset &instance();

    /** 
     * @brief Instantiates the appropriate CameraInterface implementor based on sysStatus.sensorType,
     * then performs the setup operations on that asset.
     *
     * Initializes asset instances as a 'CameraFactory' class normally would, for simplicity.
     * 
     * Asset::instance().setup(int sensorType)
     */ 
    bool setup(int sensorType);

    /** 
     * @brief Performs the readData operation on the member CameraInterface implementor reference.
     *
     * Asset::instance().readData()
     */ 
    bool readData();

protected:
    Asset();

    virtual ~Asset();

    Asset(const Asset&) = delete;

    Asset& operator=(const Asset&) = delete;

    static Asset *_instance;

private:
    /** 
     * @brief References an Asset instance - for decoupled interactions
     */
    AssetInterface *_asset;
};

#endif  /* __MACHINE_VISION_CAMERA_H */
