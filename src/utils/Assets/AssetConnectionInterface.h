#ifndef ASSET_CONNECTION_INTERFACE_H
#define ASSET_CONNECTION_INTERFACE_H

#include <ArduinoLog.h>     // https://github.com/thijse/Arduino-Log

/**
 * Defines default behavior for all asset connections must implement
*/
class AssetConnectionInterface {
public:
    virtual ~AssetConnectionInterface() {}

    /**
     * AssetConnections are Singletons (cannot enforce here), so 
     * be sure to implement instance() functionality in new singletons
     * that implement this interface.
     */

    // Initialize the connection
    virtual bool initialize() = 0;

    // Send a message to the asset
    virtual bool sendMessage(const char* message) = 0;

    // Receive a message from the asset
    virtual bool receiveMessage(char* response, int responseSize) = 0;
};

#endif // ASSET_CONNECTION_INTERFACE_H