/*
 * Project See Insights LoRA node
 * Description: Code adapted from the Maple IOT Solutions LoRa Node. 
 * Author: Chip McClelland modifying Jeff Skarda's original code
 * Date:9/13/2023
 * 
 * This device connects to a private gateway using the Lora radio communicaiton method. 
 * This code is intended for the Adafruit M0 RFM95 Featherwing although could be adapted to other hardware. 
 */

/*************************************************************************************/
//                        Includes
/*************************************************************************************/

//Include Libraries:
#include <arduino.h>
#include <ArduinoLowPower.h>
#include <ArduinoLog.h>     // https://github.com/thijse/Arduino-Log
#include <Wire.h>

//Include program structure
#include "pinout.h"
#include "timing.h"
#include "stsLED.h"
#include "take_measurements.h"
#include "MyData.h"
#include "LoRA_Functions.h"

//#define IRQ_Invalid 0
//#define IRQ_AB1805 1

const uint8_t firmwareRelease = 0;

void wakeUp_RFM95_IRQ();
void wakeUp_IRQ();


/*************************************************************************************/
//                        Define the finite state machine:
/*************************************************************************************/



/*************************************************************************************/
//                        Define all other Global Variables here:
/*************************************************************************************/



/*************************************************************************************/
//                        Define Interrupt Functions:
/*************************************************************************************/


/*************************************************************************************/
//                        SETUP
/*************************************************************************************/
void setup() 
{
  Wire.begin(); //Establish Wire.begin for I2C communication
  //Establish Serial connection if connected for debugging
  Serial.begin(9600);
  delay(5000);

  // Log.begin(LOG_LEVEL_SILENT, &Serial);
  Log.begin(LOG_LEVEL_TRACE, &Serial);
  Log.infoln("PROGRAM: See Insights LoRa Node!" CR);
  Log.infoln("Starting up...!");

  gpio.setup();
  LED.setup(gpio.STATUS);
  LED.on();

  sysData.setup();
  currentData.setup();
  sysStatus.firmwareRelease = firmwareRelease;
  tm.setup();

  // In this section we test for issues and set alert codes as needed
	if (! LoRA.setup(false)) 	{						// Start the LoRA radio - Node
		sysStatus.alertCodeNode = 3;										// Initialization failure
		sysStatus.alertTimestampNode = tm.getTime();
		Log.infoln("LoRA Initialization failure alert code %d - power cycle in 30", sysStatus.alertCodeNode);
	}
	else if (sysStatus.nodeNumber > 10 || !tm.isRTCSet()) {			// If the node number indicates this node is uninitialized or the clock needs to be set, initiate a join request
		sysStatus.alertCodeNode = 1; 									// Will initiate a join request
		Log.infoln("Node number indicated unconfigured node of %d setting alert code to %d", sysStatus.nodeNumber, sysStatus.alertCodeNode);
	}
  
  LowPower.attachInterruptWakeup(gpio.RFM95_INT, wakeUp_RFM95_IRQ, RISING);
  LowPower.attachInterruptWakeup(gpio.INT, wakeUp_IRQ, FALLING);
  // DIO0 is an extra interrupt output from the radio. Currently not used. Could be used for LoRaWAN and/or CAD sleep in the future. 
  // LowPower.attachInterruptWakeup(gpio.RFM95_DIO0, wakeUp_RFM95_DIO0, RISING);

  //Initialize each class used in this program
  take_measurements::instance().setup();
  LED.off();

  Log.infoln("Testing LoRA Radio");
  LoRA.clearBuffer();
  Log.infoln("Sending join request");
  LoRA.composeJoinRequesttNode();


  LED.off();
  Log.infoln("Finished setup");

}

/*************************************************************************************/
//                        LOOP
/*************************************************************************************/
void loop()
{ 

  tm.loop();          // Pet the hardware watchdog
  LED.loop();         // Update the Status LED
  take_measurements::instance().loop();
  sysData.loop();
  currentData.loop();
  LoRA.loop();

}

void wakeUp_RFM95_IRQ() {

}

void wakeUp_IRQ() {

}


