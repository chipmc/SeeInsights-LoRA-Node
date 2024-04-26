#ifndef __SERIAL_CONNECTION_H
#define __SERIAL_CONNECTION_H

#include "../AssetConnectionInterface.h"
#include <Arduino.h>

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * SerialConnection::instance().setup();
 * 
 * From global application loop you must call:
 * SerialConnection::instance().loop();
 */
class SerialConnection : public AssetConnectionInterface {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use SerialConnection::instance() to instantiate the singleton.
     */
    static SerialConnection &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use SerialConnection::instance().setup();
     */
    bool initialize() override;

    /**
     * @brief Send a message to the Serial1 device
    */
    bool sendMessage(const char *message) override;

    /**
     * @brief Get the response from the Serial1 device
    */
    bool receiveMessage(char *response, int responseSize) override;


protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use SerialConnection::instance() to instantiate the singleton.
     */
    SerialConnection();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~SerialConnection();

    /**
     * This class is a singleton and cannot be copied
     */
    SerialConnection(const SerialConnection&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    SerialConnection& operator=(const SerialConnection&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static SerialConnection *_instance;

};
#endif  /* __SERIAL_CONNECTION_H */
