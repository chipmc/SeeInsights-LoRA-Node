/*
 * Project See Insights LoRA node
 * Description: Code adapted from the Maple IOT Solutions LoRa Node. 
 * Author: Chip McClelland modifying Jeff Skarda's original code
 * Date:9/13/2023
 * 
 * This device connects to a private gateway using the Lora radio communicaiton method. 
 * This code is intended for the Adafruit M0 RFM95 Featherwing although could be adapted to other hardware. 
 */

// Releases and descriptions
// v0 - Initial release based on Jeff Skarda's node code and my Lora Particle Node v12
// v1 - Changed from early prototype to room occupancy counting - Works but can't sleep until we implement the PIR sensor = Changed LoRA settings to short range.
// v2 - Added PIR sensor and sleep mode - works but needs to be tuned for power consumption
// v3 - Moved to the new occupancy module.
// v4 - Breaking change, new Libraries and new data structures.  Requires gateway code v13 or higher
// v5 - Merged improvements to the TOF sensor and interrupt wakeups via the PIR sensor. Consolidated file configurations and organized file structure.
// v6 - Lots of bug fixes around data storage and node initialization.
// v7 - Started to add the logic to support spaces, placement and single entry/exit (requires Gateway v14 or above)
// v7.1 - Minor updates to support messaging to the gateway and Ubidots.
// v7.2 - Fixes to reporting (Gross, net and less than zero if a single entrance) and sleep time calculation
// v7.3 - Reduced the verbosity of the sleeping messaging.  
// v7.4 - Changed name of single entrance to multi - want default to be zero
// v8.0 - Breaking change - amended data payload values for data and join requests - Requires gateway v15 or above
// v9.0 - Breaking change - amended data payload values for data requests - setting mounting variables based on Join data payload - Requires gateway v16 or above
// v10 - Now using the Pololu VL53L1X library (installed through platformIO). Simplified TOF measurements and calibration. Vast improvements to SPAD configuration using this library
// v11 - Breaking Change - Requires gateway v17 or above
//		 ... Now able to fully control all configuration for the TOF sensor. Enhanced Alerts by adding alertContext (1 byte of data) to the alerts.
//		 ... added zoneModes, a set of predefined SPAD configurations. Zone mode can be changed by the gateway 
// v11.1 - Fixed data type issues with occupancyNet, fixed misuse of strcmp found in PeopleCounter where counts were in backwards direction
// v11.2 - Implemented LoRA Radio sleep to reduce power consumption and changed sleep mode to deep sleep
// v11.3 - Added Alert Code 12 - Gateway can manually set the occupancyNet value
// v11.4 - Breaking change - Gateway v17.5 or later - changed size of alertContext in data payload to uint16_t, expanded range of interferenceBuffer and occupancyCalibrationLoops
// v11.5 - fixed bug with space, where we were checking the wrong index in buffer in Lora_Functions.cpp
// v11.6 - Added a feature to put the device to sleep when the battery charge gets below 10%
// v11.7 - Fixed issue with Battery Interrupt - need to use an internal pull-up
// v11.8 - Now using the !SHUTDOWN pin on the MAX17048 for sleep mode (note no shutdown pin for the PIR sensor)
// v11.9 - Potnential fix for device locking up when going to sleep - added a delay after turning off the I2C bus
// v11.6 - Added a feature to put the device to sleep when the battery charge gets below 10%
// v11.7 - Fixed issue with Battery Interrupt - need to use an internal pull-up
// v11.8 - Now using the !SHUTDOWN pin on the MAX17048 for sleep mode (note no shutdown pin for the PIR sensor)
// v12 - TOF Sensor now has a 'detection' mode, where it will detect people using a 16x16 array of SPADS with a configurable intermeasurement period before measuring at the max rate
// 		... Gateway can now configure TOF detections per second through particle command (see README of gateway)
//		... Requires Gateway v21.5 or later for particle function to be available
// v13 - Node now reports at a frequency set by the gateway - Requires Gateway v22 or later
// v13.1 - Node now reports TRNASMIT_LATENCY seconds after the last count change, instead of immediately with a rate limit

/*
Wish List:
1) Better error handling on startup - memory
5) Perhaps we need to count and report the number of wakeup events
*/

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
#include "Config.h"

const uint8_t firmwareRelease = 13;

// Instandaitate the classes

// State Machine Variables
enum State { INITIALIZATION_STATE, ERROR_STATE, IDLE_STATE, SLEEPING_STATE, LOW_BATTERY, ACTIVE_PING, LoRA_TRANSMISSION_STATE, LoRA_LISTENING_STATE, LoRA_RETRY_WAIT_STATE};
char stateNames[9][16] = {"Initialize", "Error", "Idle", "Sleeping", "Low Battery", "Active Ping","LoRA Transmit", "LoRA Listening", "LoRA Retry Wait"};
volatile State state = INITIALIZATION_STATE;
State oldState = INITIALIZATION_STATE;

// Initialize Functions
void transmitDelayTimerISR();
void listeningDurationTimerISR();
void wakeUp_RFM95_IRQ();
void userSwitchISR();
void sensorISR();
void publishStateTransition(void);
void wakeUp_RFM95_IRQ();
void wakeUp_Timer();

// Program Variables
volatile bool userSwitchDetected = false;		
volatile bool sensorDetect = false;
volatile uint8_t IRQ_Reason = 0; 						// 0 - Invalid, 1 - AB1805, 2 - RFM95 DIO0, 3 - RFM95 IRQ, 4 - User Switch, 5 - Sensor

// Device Setup
void setup() 
{
	Wire.begin(); 										// Establish Wire.begin for I2C communication
	Serial.begin(115200);								//Establish Serial connection if connected for debugging
	delay(2000);

	// Log.begin(LOG_LEVEL_SILENT, &Serial);
	Log.begin(LOG_LEVEL_TRACE, &Serial);
	Log.infoln("PROGRAM: See Insights LoRa Node!" CR);

	//Initialize each class used in this program
	pinout::instance().setup();							// Pins and their modes
	gpio.setup();										// GPIO pins
	LED.setup(gpio.STATUS);								// Led used for status
	LED.on();
	sysData.setup();									// System state persistent store
	delay(100);											// Reduce initialization errors - to be tested
	timeFunctions.setup();
	currentData.setup();
	sysStatus.firmwareRelease = firmwareRelease;
	measure.setup();
	current.batteryState = 1;							// The prevents us from being in a deep sleep loop - need to measure on each reset

	// Need to set up the User Button pressed action here
	LowPower.attachInterruptWakeup(gpio.RFM95_INT, wakeUp_RFM95_IRQ, RISING);
	LowPower.attachInterruptWakeup(gpio.I2C_INT, sensorISR, RISING);
	LowPower.attachInterruptWakeup(gpio.USER_SW, userSwitchISR, FALLING);
	LowPower.attachInterruptWakeup(gpio.WAKE, wakeUp_Timer, FALLING); 
	// LowPower.attachInterruptWakeup(gpio.RFM95_DIO0, wakeUp_RFM95_DIO0, RISING);	// DIO0 is an extra interrupt output from the radio. Could be used for LoRaWAN and/or CAD sleep in the future. 

	// In this section we test for issues and set alert codes as needed
	if (! LoRA.setup(false)) 	{						// Start the LoRA radio - Node
		sysStatus.alertCodeNode = 3;					// Initialization failure
		Log.infoln("LoRA Initialization failure alert code %d - power cycle in 30", sysStatus.alertCodeNode);
	}
	else if (sysStatus.nodeNumber == 255 || !timeFunctions.isRTCSet()) {			// If the node number indicates this node is uninitialized or the clock needs to be set, initiate a join request
		Log.infoln("We got here becasue the node number is %d and the RTC is %s", sysStatus.nodeNumber, (timeFunctions.isRTCSet()) ? "set" : "not set");
		sysStatus.alertCodeNode = 1; 					// Will initiate a join request
		Log.infoln("Node number indicated unconfigured node of %d setting alert code to %d", sysStatus.nodeNumber, sysStatus.alertCodeNode);
	}

	// Next, we will make sure that the device is set up to sleep for a reasonable amount of time
	if (sysStatus.alertCodeNode == 0) {
		if (sysStatus.nextConnection < timeFunctions.getTime()) {
			sysStatus.nextConnection = timeFunctions.getTime() + 60UL;		// If the next connection is in the past, set it to 1 minute from now
			Log.infoln("Next connection was in the past - setting to 1 minute from now");
		}
		else if (sysStatus.nextConnection - timeFunctions.getTime() > 3600UL) {
			sysStatus.nextConnection = timeFunctions.getTime() + 3600UL;		// If the next connection is more than an hour from now, set it to 1 hour from now
			Log.infoln("Next connection was more than an hour from now - setting to 1 hour from now");
		}
	}

	if (state == INITIALIZATION_STATE) state = IDLE_STATE;
	// Log.infoln("Startup complete for the Node with alert code %d and last connect %s", sysStatus.alertCodeNode, Time.format(sysStatus.lastConnection), "%T").c_str());
	Log.infoln("Startup complete for the Node with alert code %d", sysStatus.alertCodeNode);

	sysStatusData::instance().sysDataChanged = true;
	currentStatusData::instance().currentDataChanged = true;

	LED.off();
}

// Main Loop
void loop()
{ 
	switch (state) {

		case IDLE_STATE: {														// Unlike most sketches - nodes spend most time in sleep and only transit IDLE once or twice each period
			static unsigned long keepAwake = 0;
			if (state != oldState) {
				keepAwake = millis();
				publishStateTransition();              							// We will apply the back-offs before sending to ERROR state - so if we are here we will take action
			}

			if (current.batteryState == 0) state = LOW_BATTERY;					// Battery level is very low - going to sleep until we get some charge
			else if (sysStatus.alertCodeNode != 0) state = ERROR_STATE;			// If there is an alert code, we need to resolve it
			else if (sensorDetect) state = ACTIVE_PING;							// If someone is detected by PIR ...
			else if (millis() - keepAwake > 1000) state = SLEEPING_STATE;		// If nothing else, go back to sleep - keep awake for 1 second 
		} break;

		case SLEEPING_STATE: {
			IRQ_Reason = IRQ_Invalid;

			if (digitalRead(gpio.INT)) {Log.infoln("Sensor pin(line1) still high - delaying sleep"); break;}									// If the sensor is still high, we need to stay awake
			if (digitalRead(gpio.I2C_INT)){Log.infoln("Sensor pin(line2) still high - delaying sleep"); break;}

			publishStateTransition();              								// Publish state transition

			LoRA.sleepLoRaRadio();												// Put the LoRA radio to sleep
			
			time_t currentTime = timeFunctions.getTime();						// How long to sleep

			if ((currentStatusData::instance().currentDataChanged == true)) {	// If the current data has changed, set the next wake/report to TRANSMIT_LATENCY seconds from now
				Log.infoln("Current data has changed - going to transmit");
				sysStatus.nextConnection = currentTime + TRANSMIT_LATENCY;	// Set nextConnection to TRANSMIT_LATENCY from now
				currentStatusData::instance().currentDataChanged = false; // Set currentDataChanged to false so that, if another count comes in within TRANSMIT_LATENCY seconds, we refresh the wait time
			}

			if (sysStatus.nextConnection - currentTime <= 0){ // if a report is overdue
				Log.infoln("Report is overdue - going to transmit");
				state = LoRA_TRANSMISSION_STATE;	// transmit now
				break;
			} else {		// otherwise, go to sleep 
				unsigned long sleepTime = sysStatus.nextConnection - currentTime;	
				Log.infoln("Going to sleep for %u seconds", sleepTime);

				timeFunctions.stopWDT();  											// No watchdogs interrupting our slumber
				timeFunctions.interruptAtTime(currentTime + sleepTime, 0);          // Set the interrupt for the next event
				digitalWrite(gpio.I2C_EN, LOW);										// Turn off the I2C bus (pre-production module)
				delay(50);
				LowPower.deepSleep((sleepTime + 1) * 1000UL);						// Go to sleep
				timeFunctions.resumeWDT();                                          // Wakey Wakey - WDT can resume
				digitalWrite(gpio.I2C_EN, HIGH);										// Turn on the I2C bus (pre-production module)
			}
			
			if (IRQ_Reason == IRQ_AB1805) {
				Log.infoln("Time to wake up and report");
				state = LoRA_TRANSMISSION_STATE;								// A full period has passed - time to report
			}
			else if (IRQ_Reason == IRQ_RF95_DIO0) {
				Log.infoln("Woke up for DIO0");
				state = LoRA_LISTENING_STATE;
			}
			else if (IRQ_Reason == IRQ_RF95_IRQ) {
				Log.infoln("Woke up for RF95 IRQ");
				state = LoRA_LISTENING_STATE;
			}
			else if (IRQ_Reason == IRQ_Sensor) {
				Log.infoln("Woke up for Sensor"); 								// Interrupt from the PIR Sensor
				state = IDLE_STATE;
			}
			else if (IRQ_Reason == IRQ_UserSwitch) {
				Log.infoln("Woke up for User Switch");
				state = IDLE_STATE;
			}
			else {
				Log.infoln("Woke up without an interrupt - going to IDLE");
				state = IDLE_STATE;
			}

			// sensorControl(sysStatus.get_sensorType(),true);					// Enable the sensor

		} break;

		case LOW_BATTERY: {														// This is our low power state - ignoring all else

			if (state != oldState) {
				publishStateTransition();													
				Log.infoln("Transition to Low Battery operations");
				LoRA.sleepLoRaRadio();											// Make sure the radio is off
				LED.off();														// Turn off the led in case it was on
			}

			// How long to sleep
			time_t time = timeFunctions.getTime() + 3600UL;						// We will sleep for one hour and then check to see if the battery had recovered
			unsigned long time_millis = time * 1000UL;

			timeFunctions.stopWDT();  											// No watchdogs interrupting our slumber
			digitalWrite(gpio.I2C_EN, LOW);									// Turn off the I2C bus (pre-production module)
			Log.infoln("Going to sleep for one hour with sensor off");

			timeFunctions.interruptAtTime(time + 1, 0);                 		// Set the interrupt for the next event - this is the backup alarm - like a snooze button
			LoRA.sleepLoRaRadio();												// Put the LoRA radio to sleep
			LowPower.deepSleep(time_millis);									// Go to sleep
			timeFunctions.resumeWDT();                                          // Wakey Wakey - WDT can resume
			digitalWrite(gpio.I2C_EN, HIGH);									// Turn off the I2C bus (pre-production module)
			Log.infoln("Waking up the sensor");

			measure.takeMeasurements();											// Check to see if the battery is charged
			state = IDLE_STATE;

		} break;

		case ACTIVE_PING: {														// Defined as a state so we could get max sampling rate
			sensorDetect = false;

			if (state != oldState) {
				publishStateTransition();													
				Log.infoln("Active Ping with occupancyNet of %d. occupancyGross of %d and occupancyState of %d", current.occupancyNet, current.occupancyGross, current.occupancyState);
			}

			measure.loop();	

			if (!digitalRead(gpio.I2C_INT) && current.occupancyState != 3) {				// If the pin is LOW, and the occupancyState is not 3 send back to IDLE
				state = IDLE_STATE;																// ... and go back to IDLE_STATE
			}
		}  break;

		case LoRA_LISTENING_STATE: {															// Timers will take us to transmit and back to sleep
			static unsigned long listeningStarted = 0;

			if (state != oldState) {
				listeningStarted = millis();
				randomSeed(sysStatus.lastConnection * sysStatus.nodeNumber);					// Done so we can genrate rando numbers later
				publishStateTransition();                   									// Publish state transition
			}

			if (LoRA_Functions::instance().listenForLoRAMessageNode()) {						// Listen for LoRA signals - could be an acknowledgement or a message to relay to another node
				if (sysStatus.alertCodeNode != 0) {
					Log.infoln("Alert code %d - going to error state", sysStatus.alertCodeNode);
					state = ERROR_STATE;														// Need to resolve alert before listening for others
				}
				Log.infoln("Received a message in %lmSec", millis() - listeningStarted);
				sysStatusData::instance().sysDataChanged = true;													// We have received a message - need to update the system data
				state = IDLE_STATE;
			}
			else if (millis() - listeningStarted > 5000L) {
				Log.infoln("Listened for 5 seconds - going back to sleep");
				state = IDLE_STATE;																// Go back to IDLE state - no response
			}

		} break;

		case LoRA_TRANSMISSION_STATE: {
			bool result = false;
			static int retryCount = 0;

			publishStateTransition();                   						// Let everyone know we are changing state
			sysStatus.lastConnection = timeFunctions.getTime();					// Prevents cyclical Transmits
			measure.takeMeasurements();											// Taking measurements now should allow for accurate battery measurements
			LoRA_Functions::instance().clearBuffer();
			// Based on Alert code, determine what message to send
			if (sysStatus.alertCodeNode == 0) result = LoRA_Functions::instance().composeDataReportNode();
			else if (sysStatus.alertCodeNode == 1 || sysStatus.alertCodeNode == 2) result = LoRA_Functions::instance().composeJoinRequesttNode();
			else {
				Log.infoln("Alert code = %d",sysStatus.alertCodeNode);
				state = ERROR_STATE;
				break;															// Resolve the alert code in ERROR_STATE
			}		

			if (result) {
				retryCount = 0;													// Successful transmission - go listen for response
				state = LoRA_LISTENING_STATE;
				sysStatusData::instance().sysDataChanged = true;
			}
			else if (retryCount >= 3) {
				Log.infoln("Too many retries - giving up for this period");
				retryCount = 0;
				if ((timeFunctions.getTime() - sysStatus.lastConnection > 3600UL) && timeFunctions.getTime() > sysStatus.nextConnection) { 	// Device has not connected and it is past due for a connection
					Log.infoln("Not connecting - power cycle after current cycle");
					sysStatus.alertCodeNode = 3;							// This will trigger a power cycle reset	
					state = ERROR_STATE;									// Likely radio is locked up - reset the device and radio
					break;
				}
				state = LoRA_LISTENING_STATE;
			}
			else {
				Log.infoln("Transmission failed - retry number %d",retryCount++);
				state = LoRA_RETRY_WAIT_STATE;
			}
		} break;

		case LoRA_RETRY_WAIT_STATE: {											// In this state we introduce a random delay and then retransmit
			static unsigned long variableDelay = 0;
			static unsigned long startDelay = 0;

			if (state != oldState) {
				publishStateTransition();                   					// Publish state transition
				variableDelay = random(20000);									// a random delay up to 20 seconds
				startDelay = millis();
				Log.infoln("Going to retry in %u seconds", variableDelay/1000UL);
			}

			if (millis() >= startDelay + variableDelay) state = LoRA_TRANSMISSION_STATE;

		} break;

		case ERROR_STATE: {														// Where we go if things are not quite right
			if (state != oldState) {
				publishStateTransition();                						// We will apply the back-offs before sending to ERROR state - so if we are here we will take action
			}

			switch (sysStatus.alertCodeNode) {
			case 1:															// Case 1 is an unconfigured node - needs to send join request
				sysStatus.nodeNumber = 255;
				Log.infoln("LoRA Radio initialized as an unconfigured node %i and a uniqueID of %u", sysStatus.nodeNumber, sysStatus.uniqueID);
				state = LoRA_TRANSMISSION_STATE;							// Sends the alert and clears alert code
			break;
			case 2:																// Case 2 is for Time not synced
				Log.infoln("Alert 2- Time is not valid going to join again");
				state = LoRA_TRANSMISSION_STATE;								// Sends the alert and clears alert code
			break;
			case 3: {															// Case 3 is generic - power cycle device to recover from errors
				static unsigned long enteredState = millis();
				if (millis() - enteredState > 30000L) {
					Log.infoln("Alert 3 - Resetting device");
					sysStatus.alertCodeNode = 0;							// Need to clear so we don't get in a retry cycle
					sysData.storeSysData();         // All this is required as we are done trainsiting loop
					delay(2000);
					timeFunctions.deepPowerDown();
				}
			} break;
			case 4: 															// In this state, we have retried sending - time to reinitilize the modem
				Log.infoln("Initialize LoRA radio");
				if(LoRA_Functions::instance().initializeRadio()) {
					Log.infoln("Initialization successful");	
					sysStatus.alertCodeNode = 0;								// Modem reinitialized successfully, going back to retransmit
					state = LoRA_LISTENING_STATE;								// Sends the alert and clears alert code
				}
				else {
					Log.infoln(("Initialization not successful - power cycle"));
					sysStatus.alertCodeNode = 3;							// Next time through - will transition to power cycle
					state = IDLE_STATE;
				}
			break;
			case 5:															// In this case, we will reset all data
				sysData.initialize();										// Resets the sysStatus values to factory default
				currentData.resetEverything();								// Resets the node counts
				sysStatus.alertCodeNode = 1;								// Resetting system values requires we re-join the network		
				Log.infoln("Full Reset and Re-Join Network");
				state = LoRA_LISTENING_STATE;									// Sends the alert and clears alert code
			break;
			case 6: 															// In this state system data is retained but current data is reset
				currentData.resetEverything();
				sysStatus.alertCodeNode = 0;
				state = IDLE_STATE;											// Once we clear the counts we can go back to Idle / sleep - the park is closed
			break;
			case 7: 															// In this state an update to the zoneMode is to be made using the alertContext
				sysStatus.zoneMode = sysStatus.alertContextNode;
				Log.infoln("Alert code 7 - Zone mode now set to %d", sysStatus.zoneMode);
				sysStatus.alertCodeNode = 0;
				state = LoRA_TRANSMISSION_STATE;								// Sends the alert and clears alert code
			break;
			case 8: 															// In this state an update to the zoneMode is to be made using the alertContext
				sysStatus.distanceMode = sysStatus.alertContextNode;
				Log.infoln("Alert code 8 - Distance mode now set to %d", sysStatus.distanceMode);
				sysStatus.alertCodeNode = 0;
				state = LoRA_TRANSMISSION_STATE;								// Sends the alert and clears alert code
			break;
			case 9: 															// In this state an update to the interferenceBuffer is to be made using the alertContext
				sysStatus.interferenceBuffer = sysStatus.alertContextNode;
				Log.infoln("Alert code 9 - Interference Buffer now set to %dmm", sysStatus.interferenceBuffer);
				sysStatus.alertCodeNode = 0;
				state = LoRA_TRANSMISSION_STATE;								// Sends the alert and clears alert code
			break;
			case 10: 															// In this state an update to the occupancyCalibrationLoops is to be made using the alertContext
				sysStatus.occupancyCalibrationLoops = sysStatus.alertContextNode;
				Log.infoln("Alert code 10 - Occupancy Calibration Loops now set to %d", sysStatus.occupancyCalibrationLoops);
				sysStatus.alertCodeNode = 0;
				state = LoRA_TRANSMISSION_STATE;								// Sends the alert and clears alert code
			break;
			case 11: 															// In this state a recalibration is being initiated by the alert code
				measure.recalibrate();
				Log.infoln("Alert code 11 - Device recalibrating");
				sysStatus.alertCodeNode = 0;
				state = LoRA_TRANSMISSION_STATE;								// Sends the alert and clears alert code
			break;
			case 12: 															// This state sets the value of the current net count to the value sent in the alert context on the gateway data acknowledgement
				current.occupancyNet = sysStatus.alertContextNode;
				Log.infoln("Alert code 12 - Gateway has overwritten occupancyNet. New occupancyNet: %d", current.occupancyNet);
				sysStatus.alertCodeNode = 0;
				state = LoRA_TRANSMISSION_STATE;								// Sends the alert and clears alert code
			break;
			case 13: 															// This state sets the value of the current net count to the value sent in the alert context on the gateway data acknowledgement
				sysStatus.tofDetectionsPerSecond = sysStatus.tofDetectionsPerSecond;
				Log.infoln("Alert code 13 - TOF Sensor Polling Rate now set to %dms", sysStatus.tofDetectionsPerSecond);
				sysStatus.alertCodeNode = 0;
				state = LoRA_TRANSMISSION_STATE;								// Sends the alert and clears alert code
			break;
			default:
				Log.infoln("Undefined Error State");
				sysStatus.alertCodeNode = 0;
				state = IDLE_STATE;
			break;
			}
		} break;
		case INITIALIZATION_STATE: 
			// Should not get here
			state = IDLE_STATE;
		break;
	}

	// Housekeeping
	if (userSwitchDetected) {
		delay(100);																// Debounce the button press
		userSwitchDetected = false;											// Clear the interrupt flag
		// if (!listeningDurationTimer.isEnabled()) listeningDurationTimer.enableIfNot();		// Don't reset timer if it is already running
		Log.infoln("Detected button press");
		state = LoRA_TRANSMISSION_STATE;
	}

	// Update the vairous classes
	timeFunctions.loop();          											    // Pet the hardware watchdog
	LED.loop();         														// Update the Status LED
	sysData.loop();
	currentData.loop();
	LoRA.loop();
}

/**
 * @brief Publishes a state transition to the Log Handler and to the Particle monitoring system.
 *
 * @details A good debugging tool.
 */
void publishStateTransition(void)
{
	char stateTransitionString[256];
	bool publish = true;

	if (state == IDLE_STATE) {
		if (!timeFunctions.isRTCSet()) snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s with invalid time", stateNames[oldState],stateNames[state]);
		else if (oldState == ACTIVE_PING) publish = false;			// Don't log this transition - too many times
		else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);
	}
	else if (state == ACTIVE_PING && oldState == IDLE_STATE) publish = false;		// Don't log this transition - too many times
	else if (sysStatus.alertCodeNode != 0) snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s with alert code %d", stateNames[oldState],stateNames[state], sysStatus.alertCodeNode);
	else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);
	oldState = state;
	if (publish) Log.infoln(stateTransitionString);
}

void wakeUp_RFM95_IRQ() {
    IRQ_Reason = IRQ_RF95_IRQ;
    // Something more here?
}

void wakeUp_DIO0_IRQ() {
    IRQ_Reason = IRQ_RF95_DIO0;
    // Something more here?
}

void wakeUp_Timer() {
    IRQ_Reason = IRQ_AB1805;
    // Something more here?
}

void userSwitchISR() {
	userSwitchDetected = true;
  	IRQ_Reason = IRQ_UserSwitch;
}

void sensorISR() {	
	sensorDetect = true;	      // flag that the sensor has detected something
	IRQ_Reason = IRQ_Sensor;      // and write to IRQ_Reason in order to wake the device up
}
