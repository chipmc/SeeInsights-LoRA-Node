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

// Defines 
#define NODENUMBEROFFSET 10000UL					// By how much do we off set each node by node number
#define IRQ_Invalid 0								// Here is where we will keep track of what woke us
#define IRQ_AB1805 1
#define IRQ_RF95_DIO0 2
#define IRQ_RF95_IRQ 3
#define IRQ_UserSwitch 4
#define IRQ_Sensor  5

// Global timing settings
#define OCCUPANCY_LATENCY 1000UL					// How long do we keep pinging after the last time we saw someone in the door
#define TRANSMIT_LATENCY 60UL						// How long do we wait after the last time we sent a message to send another

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

const uint8_t firmwareRelease = 1;

// Instandaitate the classes

// State Machine Variables
enum State { INITIALIZATION_STATE, ERROR_STATE, IDLE_STATE, SLEEPING_STATE, ACTIVE_PING, LoRA_TRANSMISSION_STATE, LoRA_LISTENING_STATE, LoRA_RETRY_WAIT_STATE};
char stateNames[8][16] = {"Initialize", "Error", "Idle", "Sleeping", "Active Ping","LoRA Transmit", "LoRA Listening", "LoRA Retry Wait"};
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
volatile bool userSwitchDectected = false;		
volatile bool sensorDetect = false;
volatile uint8_t IRQ_Reason = 0; // 0 - Invalid, 1 - AB1805, 2 - RFM95 DIO0, 3 - RFM95 IRQ, 4 - User Switch, 5 - Sensor

// Device Setup
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

  //Initialize each class used in this program
  gpio.setup();
  LED.setup(gpio.STATUS);
  LED.on();
  timeFunctions.setup();
  sysData.setup();
  currentData.setup();
  sysStatus.firmwareRelease = firmwareRelease;
  measure.setup();

  // Need to set up the User Button pressed action here

  LowPower.attachInterruptWakeup(gpio.RFM95_INT, wakeUp_RFM95_IRQ, RISING);
  // LowPower.attachInterruptWakeup(gpio.INT, sensorISR, RISING);
  LowPower.attachInterruptWakeup(gpio.USER_SW, userSwitchISR, FALLING);
  LowPower.attachInterruptWakeup(gpio.WAKE, wakeUp_Timer, FALLING); 
  // DIO0 is an extra interrupt output from the radio. Currently not used. Could be used for LoRaWAN and/or CAD sleep in the future. 
  // LowPower.attachInterruptWakeup(gpio.RFM95_DIO0, wakeUp_RFM95_DIO0, RISING);

  // In this section we test for issues and set alert codes as needed
	if (! LoRA.setup(false)) 	{						// Start the LoRA radio - Node
		sysStatus.alertCodeNode = 3;										// Initialization failure
		sysStatus.alertTimestampNode = timeFunctions.getTime();
		Log.infoln("LoRA Initialization failure alert code %d - power cycle in 30", sysStatus.alertCodeNode);
	}
	else if (sysStatus.nodeNumber > 10 || !timeFunctions.isRTCSet()) {			// If the node number indicates this node is uninitialized or the clock needs to be set, initiate a join request
		sysStatus.alertCodeNode = 1; 									// Will initiate a join request
		Log.infoln("Node number indicated unconfigured node of %d setting alert code to %d", sysStatus.nodeNumber, sysStatus.alertCodeNode);
	}

	//if (state == INITIALIZATION_STATE) state = SLEEPING_STATE;               	// Sleep unless otherwise from above code
  if (state == INITIALIZATION_STATE) state = IDLE_STATE;
  // Log.infoln("Startup complete for the Node with alert code %d and last connect %s", sysStatus.alertCodeNode, Time.format(sysStatus.lastConnection), "%T").c_str());
  Log.infoln("Startup complete for the Node with alert code %d", sysStatus.alertCodeNode);

  LED.off();
}

// Main Loop
void loop()
{ 
	switch (state) {
		case IDLE_STATE: {													// Unlike most sketches - nodes spend most time in sleep and only transit IDLE once or twice each period
			if (state != oldState) publishStateTransition();              	// We will apply the back-offs before sending to ERROR state - so if we are here we will take action
			if (currentData.currentDataChanged && timeFunctions.getTime() - sysStatus.lastConnection > TRANSMIT_LATENCY) {	// If the current data has changed and we have not connected in the last minute
				state = LoRA_TRANSMISSION_STATE;							// Go to transmit state
				Log.infoln("Current data changed - going to transmit");
			}
			else if (sysStatus.alertCodeNode != 0) state = ERROR_STATE;		// If there is an alert code, we need to resolve it
			else if (sensorDetect) state = ACTIVE_PING;						// Someone is in the door stoart pinging
			// else state = SLEEPING_STATE;									// If nothing else, go back to sleep - disabled until we get the PIR sensor
		} break;

		case SLEEPING_STATE: {
			unsigned long wakeInSeconds, wakeBoundary;
			time_t time;
			IRQ_Reason = IRQ_Invalid;

			publishStateTransition();              							// Publish state transition
			// How long to sleep
			if (timeFunctions.isRTCSet()) {
				wakeBoundary = (sysStatus.frequencyMinutes * 60UL);
				wakeInSeconds = constrain(wakeBoundary - timeFunctions.getTime() % wakeBoundary, 0UL, wakeBoundary);  // If Time is valid, we can compute time to the start of the next report window	
				// This is not right - need to compute the time to the next event
				time = timeFunctions.getTime() + wakeInSeconds;
				// Log.infoln("Sleep for %lu seconds until next event at %s with sensor %s", wakeInSeconds, Time.format(time, "%T").c_str(), (sysStatus.get_openHours()) ? "on" : "off");
				Log.infoln("Sleep for %l seconds until next event with sensor %s", wakeInSeconds, (sysStatus.openHours) ? "on" : "off");
			}
			else {
				wakeInSeconds = 60UL;
				time = timeFunctions.getTime() + wakeInSeconds;
				Log.infoln("Time not valid, sleeping for 60 seconds");
			}
			// Turn things off to save power
			// if (!sysStatus.openHours) if (sysStatus.openHours) sensorControl(sysStatus.get_sensorType(),false);
			// Configure Sleep

			timeFunctions.stopWDT();  												      // No watchdogs interrupting our slumber
			timeFunctions.interruptAtTime(time, 0);                 					  // Set the interrupt for the next event
			LowPower.deepSleep(timeFunctions.WDT_MaxSleepDuration);
				timeFunctions.resumeWDT();                                             // Wakey Wakey - WDT can resume
			if (IRQ_Reason == IRQ_AB1805) {
				Log.infoln("Time to wake up and report");
				state = IDLE_STATE;
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
				Log.infoln("Woke up for Sensor"); 	// Stay in Sleep state
			}
			else if (IRQ_Reason == IRQ_UserSwitch) {
				Log.infoln("Woke up for User Switch");
				state = IDLE_STATE;
			}
			else {
				Log.infoln("Woke up without an interrupt - going to IDLE");
				state = IDLE_STATE;
			}

			// sensorControl(sysStatus.get_sensorType(),true);									// Enable the sensor

		} break;

		case ACTIVE_PING: {																// Defined as a state so we could get max sampling rate
			static unsigned long lastOccupancy = 0;

			if (state != oldState) {
				lastOccupancy = millis();
				publishStateTransition();
				Log.infoln("Active Ping with count of %d and OccupancyState of %d", current.hourlyCount, current.occupancyState);
			}

			if (millis() - lastOccupancy > OCCUPANCY_LATENCY) {							// It has been too long since we know there was someone in the door
				if (current.occupancyState) lastOccupancy = millis();					// If the door is occupied - set the flag for another period
				else {																	// If not, then we need to leave the active state
					sensorDetect = false;												// Clear the sensor flag						
					state = IDLE_STATE;													// If not, we will go back to IDLE_STATE
				}
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
				sysData.sysDataChanged = true;													// We have received a message - need to update the system data
				state = IDLE_STATE;
			}
			else if (millis() - listeningStarted > 5000L) {
				Log.infoln("Listened for 5 seconds - going back to sleep");
				// state = SLEEPING_STATE;
				state = IDLE_STATE;		// Need to do this until we get the interrupt working.
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
				sysData.sysDataChanged = true;
			}
			else if (retryCount >= 3) {
				Log.infoln("Too many retries - giving up for this period");
				retryCount = 0;
				if ((timeFunctions.getTime() - sysStatus.lastConnection > 2 * sysStatus.frequencyMinutes * 60UL)) { 	// Device has not connected for two reporting periods
					Log.infoln("Nothing for two reporting periods - power cycle after current cycle");
					sysStatus.alertCodeNode = 3;							// This will trigger a power cycle reset
					sysStatus.alertTimestampNode = timeFunctions.getTime();		
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

		case LoRA_RETRY_WAIT_STATE: {										// In this state we introduce a random delay and then retransmit
			static unsigned long variableDelay = 0;
			static unsigned long startDelay = 0;

			if (state != oldState) {
				publishStateTransition();                   				// Publish state transition
				variableDelay = random(20000);								// a random delay up to 20 seconds
				startDelay = millis();
				Log.infoln("Going to retry in %lu seconds", variableDelay/1000UL);
			}

			if (millis() >= startDelay + variableDelay) state = LoRA_TRANSMISSION_STATE;

		} break;

		case ERROR_STATE: {													// Where we go if things are not quite right
			if (state != oldState) publishStateTransition();                // We will apply the back-offs before sending to ERROR state - so if we are here we will take action

			switch (sysStatus.alertCodeNode) {
			case 1:															// Case 1 is an unconfigured node - needs to send join request
				sysStatus.nodeNumber = 11;
				Log.infoln("LoRA Radio initialized as an unconfigured node %i and a deviceID of %i", sysStatus.nodeNumber, sysStatus.deviceID);
				state = LoRA_LISTENING_STATE;							// Sends the alert and clears alert code
			break;
			case 2:															// Case 2 is for Time not synced
				Log.infoln("Alert 2- Time is not valid going to join again");
				state = LoRA_LISTENING_STATE;							// Sends the alert and clears alert code
			break;
			case 3: {														// Case 3 is generic - power cycle device to recover from errors
				static unsigned long enteredState = millis();
				if (millis() - enteredState > 30000L) {
					Log.infoln("Alert 3 - Resetting device");
					sysStatus.alertCodeNode = 0;							// Need to clear so we don't get in a retry cycle
					sysStatus.alertTimestampNode = timeFunctions.getTime();
			sysData.storeSysData();         // All this is required as we are done trainsiting loop
					delay(2000);
					timeFunctions.deepPowerDown();
				}
			} break;
			case 4: 														// In this state, we have retried sending - time to reinitilize the modem
				Log.infoln("Initialize LoRA radio");
				if(LoRA_Functions::instance().initializeRadio()) {
					Log.infoln("Initialization successful");	
					sysStatus.alertCodeNode = 0;							// Modem reinitialized successfully, going back to retransmit
					state = LoRA_LISTENING_STATE;							// Sends the alert and clears alert code
				}
				else {
					Log.infoln(("Initialization not successful - power cycle"));
					sysStatus.alertCodeNode = 3;							// Next time through - will transition to power cycle
					sysStatus.alertTimestampNode = timeFunctions.getTime();
					state = IDLE_STATE;
				}
			break;
			case 5:															              // In this case, we will reset all data
				sysData.initialize();										        // Resets the sysStatus values to factory default
				currentData.resetEverything();									// Resets the node counts
				sysStatus.alertCodeNode = 1;								    // Resetting system values requires we re-join the network
				sysStatus.alertTimestampNode = timeFunctions.getTime();			
				Log.infoln("Full Reset and Re-Join Network");
				state = LoRA_LISTENING_STATE;							// Sends the alert and clears alert code

			break;
			case 6: 														// In this state system data is retained but current data is reset
				currentData.resetEverything();
				sysStatus.alertCodeNode = 0;
				state = LoRA_LISTENING_STATE;								// Once we clear the counts we can go back to listening
			break;
			default:
				Log.infoln("Undefined Error State");
				sysStatus.alertCodeNode = 0;
				state = IDLE_STATE;
			break;
			}
		}
		case INITIALIZATION_STATE: 
			// Should not get here
			state = IDLE_STATE;
			break;
	}

	// Housekeeping
	if (userSwitchDectected) {
		delay(100);									// Debounce the button press
		userSwitchDectected = false;				// Clear the interrupt flag
		// if (!listeningDurationTimer.isEnabled()) listeningDurationTimer.enableIfNot();				// Don't reset timer if it is already running
		Log.infoln("Detected button press");
		state = LoRA_TRANSMISSION_STATE;
	}

	if (current.occupancyState) {
		delay(1);
		if (current.occupancyState) sensorDetect = true;		// Reduce noise
	} 

	// Update the vairous classes
	timeFunctions.loop();          	// Pet the hardware watchdog
	LED.loop();         			// Update the Status LED
	measure.loop();					// Check to see if the sensor value has changed
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
	if (state == IDLE_STATE) {
		if (!timeFunctions.isRTCSet()) snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s with invalid time", stateNames[oldState],stateNames[state]);
		else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);
	}
	else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);
	oldState = state;
	Log.infoln(stateTransitionString);
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

// To be eliminated
void transmitDelayTimerISR() {
	state = LoRA_TRANSMISSION_STATE;										// Time for our node to transmit
}

// To be eliminated
void listeningDurationTimerISR() {
	LoRA_Functions::instance().sleepLoRaRadio();							// Done with the radio - shut it off
	state = SLEEPING_STATE;													// Go back to sleep
}

void userSwitchISR() {
	userSwitchDectected = true;
  IRQ_Reason = IRQ_UserSwitch;
}

void sensorISR()
{
  IRQ_Reason = IRQ_Sensor;
  sensorDetect = true;
}
