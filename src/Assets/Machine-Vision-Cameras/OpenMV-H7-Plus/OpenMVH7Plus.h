#ifndef __OPENMV_H7_PLUS_H
#define __OPENMV_H7_PLUS_H

#include "Assets/AssetInterface.h"
#include "Utils/Assets/Serial/SerialConnection.h"

/**
 * OpenMV H7 Plus camera - Singleton that implements CameraInterface
 */
class OpenMVH7Plus : public AssetInterface {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use OpenMVH7Plus::instance() to instantiate the singleton.
     */
    static OpenMVH7Plus& instance();

    /**
     * @brief Performs setup operations for the OpenMV H7 Plus camera.
     */
    bool setup() override;

    /**
     * @brief Reads data from an OpenMV H7 Plus, using a SerialConnection
     */
    bool readData() override;

protected:
    OpenMVH7Plus();
    
    virtual ~OpenMVH7Plus();
    
    OpenMVH7Plus(const OpenMVH7Plus&) = delete;

    OpenMVH7Plus& operator=(const OpenMVH7Plus&) = delete;

    static OpenMVH7Plus *_instance;
};

#endif  /* __OPENMV_H7_PLUS_H */