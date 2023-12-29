/**
 * @file    MyData.h
 * @author  Chip McClelland (chip@seeinsights.com)
 * @brief   We will define the structure of our data 
 * @details 
 * @version 0.1
 * @date    2023-10-15
 * 
 * Version History:
 * 0.1 - Initial realease 
 * 
 */

// System Data and some Current Data is stored in EEPROM - memory map is as follows (8 bit page)
/* System Data Memory Map

Overhead
    Address         Data Type      Variable             Description        
	00              uint8_t        structureVersion     Varialble that changes when the structure is updated
    01-04           uint32_t       uniqueID             Unique identifier for this device - generated by the gateway on first connection
    05-09           Reserved
System Data
	10      	    uint8_t        firmwareRelease      Version of the device firmware (integer - aligned to particle product firmware)
    11              uint8_t        nodeNumber           Assigned by the gateway on joining the network
    12              uint16_t       magicNumber          Number that validates nodes on a network (all share this number)
    14              uint16_t       token                Token to validate the node on the network
    16              uint32_t       uniqueID             uniqueID - unique to each device
	20	            uint8_t        resetCount           Reset count of device (0-256)
	21              time_t         lastConnection       last time we successfully connected to the gateway
    25              time_t         nextConnection       next time we will attempt to connect to the gateway
	29              uint8_t        alertCodeNode        Alert code from node
	30      	    uint8_t        sensorType           PIR sensor, car counter, others - this value is changed by the Gateway
    31              uint8_t        placement            0 for outside, 1 for inside - determnines whether we count up or down
    32-49           Reserved
Current Data
    90              int8_t         internalTempC;       Enclosure temperature in degrees C
    94              int8_t         internalHumidity     Enclosure humidity in percent
	98      	    int8_t         stateOfCharge        Battery charge level
	102          	uint8_t        batteryState         Stores the current battery state (charging, discharging, etc)
	103             int16_t        RSSI                 Latest signal strength value (updated adter ack and sent to gateway on next data report)
	105	            int16_t        SNR				    Latest Signal to Noise Ratio (updated after ack and send to gatewat on next dara report)
	110             uint16_t       occupancyGross      Change in occupancy since last report
	113             uint16_t       occupancyNet         Current occupancy count
    115             uint8_t        occupancyState       Allows us to monitor occupancy state across functions
*/

#ifndef __MYDATA_H
#define __MYDATA_H

//Include standard header files from libraries:
#include <arduino.h>
#include <ArduinoLog.h>
#include "SparkFun_External_EEPROM.h" // Click here to get the library: http://librarymanager/All#SparkFun_External_EEPROM

#define STRUCTURES_VERSION 4                           // Version of the data structures (system and data)

//Macros(#define) to swap out during pre-processing (use sparingly). This is typically used outside of this .H and .CPP file within the main .CPP file or other .CPP files that reference this header file. 
// This way you can do "data.setup()" instead of "MyPersistentData::instance().setup()" as an example
#define currentData currentStatusData::instance()
#define sysData sysStatusData::instance()
#define sysStatus sysStatusData::instance().sysStatusStruct
#define current currentStatusData::instance().currentStruct

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * MyPersistentData::instance().setup();
 * 
 * From global application loop you must call:
 * MyPersistentData::instance().loop();
 */

// *******************  SysStatus Storage Object **********************
//
// ********************************************************************

class sysStatusData {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    static sysStatusData &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use MyPersistentData::instance().setup();
     */
    bool setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use MyPersistentData::instance().loop();
     */
    void loop();

	/**
	 * @brief Validates values and, if valid, checks that data is in the correct range.
	 * 
	 */
	bool validate(size_t dataSize);

	/**
	 * @brief Will reinitialize data if it is found not to be valid
	 * 
	 * Be careful doing this, because when MyData is extended to add new fields,
	 * the initialize method is not called! This is only called when first
	 * initialized.
	 * 
	 */
	void initialize();

    /**
     * @brief Stores the system data to EEPROM to facilitate recover after a power cycle or reset
     * 
    */
    void storeSysData();

    /**
     * @brief Prints system data in a readable format for Serial Montitor
    */
    void printSysData();

    /**
     * @brief We have a new uniqueID that needs to be stored in a special place in EEPROM
     * 
     * This is used to read and write to the EEPROM
     */
    void updateUniqueID();


	struct SystemDataStructure
	{
        uint8_t structuresVersion;                        // Version of the data structures (system and data)
        uint8_t firmwareRelease;                          // Version of the device firmware (integer - aligned to particle product firmware)
        uint16_t magicNumber;							  // A way to identify nodes and gateways so they can trust each other
        uint8_t nodeNumber;                               // Assigned by the gateway on joining the network
        uint16_t token;                                   // Token to validate the node on the network
        uint32_t uniqueID;                                // uniqueID - unique to each device
		uint8_t resetCount;                               // reset count of device (0-256)
		time_t lastConnection;                     		  // Last time we successfully connected to Particle
		time_t nextConnection;                            // When we are reporing at minute increments - what are they - for Gateways
		uint8_t alertCodeNode;                            // Alert code from node
		uint8_t sensorType;                               // PIR sensor, car counter, others - this value is changed by the Gateway
        uint8_t space;                                    // Assciates the node with a space - this value is changed by the Gateway
        uint8_t placement;                                // 0 for outside, 1 for inside - determnines whether we count up or down
        uint8_t multi;                                    // 1 if this device is taking occupancy of a room with more than one - prevents negative counts
	};
	SystemDataStructure sysStatusStruct;

public:
    bool sysDataChanged = false;

	//Members here are internal only and therefore protected
protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    sysStatusData();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~sysStatusData();

    /**
     * This class is a singleton and cannot be copied
     */
    sysStatusData(const sysStatusData&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    sysStatusData& operator=(const sysStatusData&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static sysStatusData *_instance;
    
};




// *****************  Current Status Storage Object *******************
//
// ********************************************************************

class currentStatusData {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    static currentStatusData &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use MyPersistentData::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use MyPersistentData::instance().loop();
     */
    void loop();

	/**
	 * @brief Load the appropriate system defaults - good ot initialize a system to "factory settings"
	 * 
	 */
	void loadCurrentDefaults();                          // Initilize the object values for new deployments

	/**
	 * @brief Resets the current and hourly counts
	 * 
	 */
	void resetEverything();  

	/**
	 * @brief Validates values and, if valid, checks that data is in the correct range.
	 * 
	 */
	bool validate(size_t dataSize);

	/**
	 * @brief Will reinitialize data if it is found not to be valid
	 * 
	 * Be careful doing this, because when MyData is extended to add new fields,
	 * the initialize method is not called! This is only called when first
	 * initialized.
	 * 
	 */
	void initialize();  

    /**
     * @brief Stores relevant current data (not all the current struct - only the stuff that needs to surve a reset)
     * 
     * We will need to trigger this with a flag when any of the relevant values change.
    */
    void storeCurrentData();

    /**
     * @brief Prints current data in a readable format for Serial Montitor
    */  
    void printCurrentData();

	struct CurrentDataStructure
	{
        int8_t internalTempC;                             // Enclosure temperature in degrees C
        int8_t internalHumidity;                          // Enclosure humidity in percent
		int8_t stateOfCharge;                             // Battery charge level
		uint8_t batteryState;                             // Stores the current battery state (charging, discharging, etc)
		int16_t RSSI;                                     // Latest signal strength value (updated adter ack and sent to gateway on next data report)
		int16_t SNR;									  // Latest Signal to Noise Ratio (updated after ack and send to gatewat on next dara report)
		uint16_t occupancyGross;                          // Sum of occupancy changes for the day
        uint16_t occupancyNet;                            // Current occupancy count
        uint8_t occupancyState;                           // Allows us to monitor occupancy state across functions
		// OK to add more fields here 
	};
	CurrentDataStructure currentStruct;

public:
    bool currentDataChanged = false;

	//Members here are internal only and therefore protected
protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    currentStatusData();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~currentStatusData();

    /**
     * This class is a singleton and cannot be copied
     */
    currentStatusData(const currentStatusData&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    currentStatusData& operator=(const currentStatusData&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static currentStatusData *_instance;
};
#endif  /* __MYDATA_H */
