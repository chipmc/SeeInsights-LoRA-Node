#ifndef CONFIG_H
#define CONFIG_H

/******************************************************************************************************/
/**                                                                                                  **/
/**                                       LORA NODE CORE MODULE                                      **/
/**                                                                                                  **/
/******************************************************************************************************/

/*******************************************/
/**      Static Variable Definitions      **/
/*******************************************/

/**  Interrupt Request Values   **/
#define IRQ_Invalid 0								            // Here is where we will keep track of what woke us ...
#define IRQ_AB1805 1                            // ... value set when woken up by AB1805
#define IRQ_RF95_DIO0 2                         // ... value set when woken up by RF95_DIO0
#define IRQ_RF95_IRQ 3                          // ... value set when woken up by RF95_IRQ
#define IRQ_UserSwitch 4                        // ... value set when woken up by User Switch
#define IRQ_Sensor  5                           // ... value set when woken up by sensor asset

/*******************************************/
/**           Node Configuration          **/
/*******************************************/

/**  Node Settings  **/
#define NODENUMBEROFFSET 10000UL					      // By how much do we off set each node by node number

/**  Timing Settings  **/
#define DETECTION_WAIT_LENGTH 1000UL				    // How long do we wait for a detection from the TOF sensor when the PIR sensor is active
#define OCCUPANCY_LATENCY 500UL                 // Amount of time to keep the interrupt active upon a detection by the PIR sensor
#define TRANSMIT_LATENCY 60UL						        // How long do we wait after the last time we sent a message to send another=


/******************************************************************************************************/
/**                                                                                                  **/
/**                              TIME OF FLIGHT OCCUPANCY SENSOR MODULE                              **/
/**                                                                                                  **/
/******************************************************************************************************/

/*******************************************/
/**          Mounting Parameters          **/
/*******************************************/

/**  Firmware Thresholds  **/
#define DEFAULT_PEOPLE_LIMIT 5                  // Values above this will generate an alert

/*******************************************/
/**            Debugging Tools            **/
/*******************************************/

/**  Debugging Flags  **/
#define PRINT_SENSOR_MEASUREMENTS 0             // Prints the 2-dimensional array of readings from each loop and visualizes the front and back zones. Works well with PropleCounterConfig's OCCUPANCYSTATE_DEBUG.
#define PRINT_STACK_VISUALIZATION 1             // Actively prints a visual representation of the occupancyState stack and the line number (Ex. SEQUENCE [SIZE = 2]: [0, 2] <--- 2)
#define PRINT_HOURLY_COUNT_TENFOOTDISPLAY 1     // Prints currentData.hourlyCount using ASCII characters to produce a large number that can be read at a distance.
#define PRINT_OCCUPANCY_STATE_TENFOOTDISPLAY 0  // Prints the occupancy state using ASCII characters to produce a large number that can be read at a distance.

/*******************************************/
/**        TofSensor Configuration        **/
/*******************************************/
/*
  *  The TofSensor has a receiver array consisting of 16X16 Single Photon Diodes. 
  *  By default, all of them are used but you can reduce the receiver to a region of interest (ROI).
  *  It has to be at least 4X4 in size, but can be bigger and have uneven sidelenghts.
  *  It is defined by the sidelengths in x and y direction and its center.
  *  To set the center, set the pad that is to the right and above the exact center of the region you'd like to measure as your opticalCenter
  *  Here is the function call: setROI(uint8_t x, uint8_t y, uint8_t opticalCenter); 
*/

/**  Sensor Settings  **/
#define SENSOR_TIMEOUT 500                      // Forces TofSensor::measure() to stop after waiting SENSOR_TIMEOUT ms for the SFEVL53L1X checkForDataReady() function to return a nonzero value.  

/**  Calibration Settings  **/
#define OCCUPANCY_CALIBRATION_LOOPS 25          // How many sets of samples to take during occupancy calibration (4 samples each, 2 for each zone to reduce noise).
#define DETECTION_CALIBRATION_LOOPS 50          // How many samples to take during detection calibration.
#define CALIBRATION_SIGNAL_RETRY_THRESHOLD 35   // Value of the signal that will halt calibration and retry - Ex. a person was detected during calibration

/**  Table of Optical Centers   ***
  * 
  * [Pin 1]
  * 128,136,144,152,160,168,176,184,  192,200,208,216,224,232,240,248
  * 129,137,145,153,161,169,177,185,  193,201,209,217,225,233,241,249
  * 130,138,146,154,162,170,178,186,  194,202,210,218,226,234,242,250
  * 131,139,147,155,163,171,179,187,  195,203,211,219,227,235,243,251
  * 132,140,148,156,164,172,180,188,  196,204,212,220,228,236,244,252
  * 133,141,149,157,165,173,181,189,  197,205,213,221,229,237,245,253
  * 134,142,150,158,166,174,182,190,  198,206,214,222,230,238,246,254
  * 135,143,151,159,|167|,175,183,191,  199,207,215,223,|231|,239,247,255

  * 127,119,111,103, 95, 87, 79, 71,  63, 55, 47, 39, 31, 23, 15, 7
  * 126,118,110,102, 94, 86, 78, 70,  62, 54, 46, 38, 30, 22, 14, 6
  * 125,117,109,101, 93, 85, 77, 69,  61, 53, 45, 37, 29, 21, 13, 5
  * 124,116,108,100, 92, 84, 76, 68,  60, 52, 44, 36, 28, 20, 12, 4
  * 123,115,107, 99, 91, 83, 75, 67,  59, 51, 43, 35, 27, 19, 11, 3
  * 122,114,106, 98, 90, 82, 74, 66,  58, 50, 42, 34, 26, 18, 10, 2
  * 121,113,105, 97, 89, 81, 73, 65,  57, 49, 41, 33, 25, 17, 9, 1
  * 120,112,104, 96, 88, 80, 72, 64,  56, 48, 40, 32, 24, 16, 8, 0
*/

/**  Occupancy Zone Configurations  **/
#define OCCUPANCY_ROWS_OF_SPADS         6       // This is the depth (Through the door - when sensor mounted on the inside doorframe)
#define OCCUPANCY_COLUMNS_OF_SPADS      8       // This is the width (accross the door with the sensor long axis perpendicular to the threshold) of the active SPADS
#define OCCUPANCY_FRONT_ZONE_CENTER     167     // Denotes the SPAD number (see chart below) that is the to the right and above the exact center of occupancy zone 1's ROI
#define OCCUPANCY_BACK_ZONE_CENTER      231     // Denotes the SPAD number (see chart below) that is the to the right and above the exact center of occupancy zone 2's ROI

/**  Detection Zone Configuration   **/
#define DETECTION_ROWS_OF_SPADS         16      // This is the depth (Through the door - when sensor mounted on the inside doorframe) (all rows)
#define DETECTION_COLUMNS_OF_SPADS      16      // This is the width (accross the door with the sensor long axis perpendicular to the threshold) of the active SPADs (all columns)
#define DETECTION_ZONE_CENTER           199     // Denotes the SPAD number (see chart below) that is the to the right and above the exact center of the detection zone's ROI (absolute middle)

#endif