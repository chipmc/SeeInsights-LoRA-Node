/**
 * @file LoRA-Functions.h - Singleton approach
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This file allows me to move all the LoRA functionality out of the main program - application implementation / not general
 * @version 0.1
 * @date 2022-09-14
 * 
 */

// Data exchange formats
// Format of a data report - From the Node to the Gateway so includes a token
/*
*** Header Section - Common to all Nodes
buf[0 - 1] magicNumber                      // Magic number that identifies the Gateway's network
buf[2] nodeNumber                           // nodeNumber - unique to each node on the gateway's network
buf[3 - 4] Token                            // Token given to the node and good for 24 hours
buf[5] sensorType                           // What sensor type is it
buf[6 - 9] uniqueID          // This is a 4-byte identifier that is unique to each node and is only set once
*** Payload Section - 12 Bytes but interpretion is different for each sensor type
buf[10 - 21] payload                        // Payload - 12 bytes sensor type determines interpretation
*** Status Data - Common to all Nodes
buf[22] temp;                               // Enclosure temp (single byte signed integer -127 to 127 degrees C)
buf[23] battChg;                            // State of charge (single byte signed integer -1 to 100%)
buf[24] battState;                          // Battery State (single byte signed integer 0 to 6)
buf[25] resets                              // Reset count
buf[26-27] RSSI                             // From the Node's perspective
buf[28-29] SNR                              // From the Node's perspective
*** Re-Transmission Data - Common to all Nodes
buf[30] Re-Tries                            // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
buf[31] Re-Transmission Delay               // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

// Format of a data acknowledgement - From the Gateway to the Node
/*    
    buf[0 - 1] magicNumber                  // Magic Number
    buf[2] nodeNumber                       // Node number (unique for the network)
    buf[3 - 4] Token                        // Parrot the token back to the node
    buf[5 - 8] Time.now()                   // Set the time 
    buf[9 - 10] Seconds to next Report      // The gateway tells the node how many seconds until next transmission window - up to 18 hours
    buf[11] alertCode                       // This lets the Gateway trigger an alert on the node - typically a join request
    buf[12] sensorType                      // Let's the Gateway reset the sensor if needed 
    buf[13] Re-Tries                        // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
    buf[14] Re-Transmission Delay           // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

// Format of a join request - From the Node to the Gateway
// SensorType - 0/pressure, 1/PIR, 2/Magnetometer, 4/Soil

/*
buf[0-1] magicNumber;                       // Magic Number
buf[2] nodeNumber;                          // nodeNumber - typically 255 for a join request
buf[3 - 4] token                            // token for validation - may not be valid - if it is valid - response is to set the clock only
buf[5] sensorType				            // Identifies sensor type to Gateway
buf[6 - 9] Unique Node Identifier           // This is a 4-byte identifier that is unique to each node and is only set once
buf[10-13] Payload                          // Payload - 4 bytes sensor type determines interpretation
buf[14]  Re-Tries                           // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
buf[15]  Re-Transmission Delay              // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

// Format for a join acknowledgement -  From the Gateway to the Node
/*
    buf[0 - 1]  magicNumber                 // Magic Number
    buf[2] nodeNumber;                      // nodeNumber - This is the old node number - if the node is new - it will be 255 - will get updated in Join ACK
    buf[3 - 4] token                        // token for validation - good for the day
    buf[5 - 8] Time.now()                   // Set the time 
    buf[9 - 10] Seconds till next report    // For the Gateway minutes on the hour  
    buf[11] alertCodeNode                   // Gateway can set an alert code here
    buf[12]  sensorType				        // Gateway confirms sensor type
    buf[13 - 16] uniqueID                   // This is a 4-byte identifier that is unique to each node and is only set once by the gateway on 1st joining
    buf[17] nodeNumber                      // This is the new node number - if the node is new - it will be 255 - will get updated in Join ACK
    buf[18-21] Payload                      // Payload - 4 bytes sensor type determines interpretation                         
    buf[22]  Re-Tries                       // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
    buf[23] Re-Transmission Delay           // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

#ifndef __LORA_FUNCTIONS_H
#define __LORA_FUNCTIONS_H

//Include standard header files from libraries:
#include <arduino.h>
#include <ArduinoLog.h>

// Additional libraries
#include <RH_RF95.h>						        // https://github.com/mapleiotsolutions/RF9X-RK
#include <RHMesh.h>
#include <RHEncryptedDriver.h>
#include <Speck.h>
#include "pinout.h"
#include "MyData.h"
#include "timing.h"
#include "stsLED.h"

#define LoRA LoRA_Functions::instance()

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * LoRA_Functions::instance().setup(gateway - true or false);
 * 
 * From global application loop you must call:
 * LoRA_Functions::instance().loop();
 */
class LoRA_Functions {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use LoRA_Functions::instance() to instantiate the singleton.
     */
    static LoRA_Functions &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use LoRA_Functions::instance().setup();
     */
    bool setup(bool gatewayID);

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use LoRA_Functions::instance().loop();
     */
    void loop();


    // Common Functions
    /**
     * @brief Clear whatever message is in the buffer - good for waking
     * 
     * @details calls the driver and iterates through the message queue
     * 
    */
    void clearBuffer();

    /**
     * @brief Class to put the LoRA Radio to sleep when we exit the LoRa state
     * 
     * @details May help prevent the radio locking up based on local LoRA traffic interference
     * 
    */
    void sleepLoRaRadio();

    /**
     * @brief Initialize the LoRA radio
     * 
     */
   bool initializeRadio();

 
    // Node Functions
    /**
     * @brief Listens for the gateway to respond to the node - takes note of message flag
     * 
     * @return true 
     * @return false 
     */
    bool listenForLoRAMessageNode();                // Node - sent a message - awiting reply
    /**
     * @brief Composes a Data Report and sends to the Gateway
     * 
     * @return true 
     * @return false 
     */
    bool composeDataReportNode();                  // Node - Composes data report
    /**
     * @brief Acknowledges the response from the Gateway that acknowledges receipt of a data report
     * 
     * @return true 
     * @return false 
     */
    bool receiveAcknowledmentDataReportNode();     // Node - receives acknolwedgement
    /**
     * @brief Composes a Join Request and sends to the Gateway
     * 
     * @return true 
     * @return false 
     */
    bool composeJoinRequesttNode();                // Node - Composes Join request
    /**
     * @brief Acknowledges the response from the Gateway that acknowledges receipt of a Join Request
     * 
     * @return true 
     * @return false 
     */
    bool receiveAcknowledmentJoinRequestNode();    // Node - received join request asknowledgement


protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use LoRA_Functions::instance() to instantiate the singleton.
     */
    LoRA_Functions();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~LoRA_Functions();

    /**
     * This class is a singleton and cannot be copied
     */
    LoRA_Functions(const LoRA_Functions&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    LoRA_Functions& operator=(const LoRA_Functions&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static LoRA_Functions *_instance;

public:
    // In this implementation - we have one gateway numde number 0 and up to 10 nodes with node numbers 1-10
    // Node numbers greater than 10 initiate a join request
    const uint8_t GATEWAY_ADDRESS = 0;
    // const double RF95_FREQ = 915.0;				 	// Frequency - ISM
    const int RF95_FREQ = 92684;				        // Center frequency for the omni-directional antenna I am using - x100 for Jeff's Fork of RFM9X

};
#endif  /* __LORA_FUNCTIONS_H */
