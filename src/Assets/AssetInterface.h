#ifndef ASSET_INTERFACE_H
#define ASSET_INTERFACE_H

#include <ArduinoLog.h>     // https://github.com/thijse/Arduino-Log

/**
 * Defines default behavior that all asset types must implement
*/
class AssetInterface {
public:

        /** 
     * Assets are Singletons (cannot enforce here), so 
     * be sure to implement instance() functionality in 
     * new singletons that implement this interface.
     * 
     * See OpenMVH7Plus.h for an example
     */

    virtual ~AssetInterface() {}

    /**
     * @brief Perform asset setup operations
     */ 
    virtual bool setup() = 0;
    
    /**
     * @brief Reads data from the asset using an AssetConnection (like SerialConnection)
     */
    virtual bool readData() = 0;
};

#endif // ASSET_INTERFACE_H