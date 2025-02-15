#include "LoRA_Functions.h"
#include "Assets/Asset.h"

RH_RF95 rf95(gpio.RFM95_CS, gpio.RFM95_INT);  	// Class instance for the RFM95 radio driver
Speck myCipher;                             	// Class instance for Speck block ciphering
RHEncryptedDriver driver(rf95, myCipher);   	// Class instance for Encrypted RFM95 driver
RHMesh manager(driver, LoRA.GATEWAY_ADDRESS);        // Class instance to manage message delivery and receipt, using the driver declared above

LoRA_Functions *LoRA_Functions::_instance;

// [static]
LoRA_Functions &LoRA_Functions::instance() {
    if (!_instance) {
        _instance = new LoRA_Functions();
    }
    return *_instance;
}

LoRA_Functions::LoRA_Functions() {
}

LoRA_Functions::~LoRA_Functions() {
}


// ************************************************************************
// *****                      LoRA Setup                              *****
// ************************************************************************


// Define the message flags
typedef enum { NULL_STATE, JOIN_REQ, JOIN_ACK, DATA_RPT, DATA_ACK, ALERT_RPT, ALERT_ACK} LoRA_State;
char loraStateNames[7][16] = {"Null", "Join Req", "Join Ack", "Data Report", "Data Ack", "Alert Rpt", "Alert Ack"};
static LoRA_State lora_state = NULL_STATE;

// Mesh has much greater memory requirements, and you may need to limit the
// max message length to prevent wierd crashes
#ifndef RH_MAX_MESSAGE_LEN
#define RH_MAX_MESSAGE_LEN 255
#endif

// Mesh has much greater memory requirements, and you may need to limit the
// max message length to prevent wierd crashes
// #define RH_MESH_MAX_MESSAGE_LEN 50
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];               // Related to max message size - RadioHead example note: dont put this on the stack:

bool LoRA_Functions::setup(bool gatewayID) {
    // Set up the Radio Module
	LoRA_Functions::initializeRadio();

	manager.setTimeout(120); // Set to 120
	manager.setRetries(2); // Set to 2	

	Log.infoln("In LoRA setup - node number %d and uniqueID of %u",sysStatus.nodeNumber, sysStatus.uniqueID);

	if (gatewayID == true) {
		sysStatus.nodeNumber = GATEWAY_ADDRESS;							// Gateway - Manager is initialized by default with GATEWAY_ADDRESS - make sure it is stored in FRAM
		Log.infoln("LoRA Radio initialized as a gateway with a uniqueID of %u", sysStatus.uniqueID);
	}
	else if (sysStatus.nodeNumber > 0 && sysStatus.nodeNumber < 255) {
		manager.setThisAddress(sysStatus.nodeNumber);// Node - use the Node address in valid range from memory
		Log.infoln("LoRA Radio initialized as node %i and a uniqueID of %u", manager.thisAddress(), sysStatus.uniqueID);
	}
	else {																						// Else, we will set as an unconfigured node
		manager.setThisAddress(255);
		sysStatus.alertCodeNode = 1;															// Join request required
		Log.infoln("LoRA Radio initialized as an unconfigured node %i and a uniqueID of %u and alert code %d", manager.thisAddress(), sysStatus.uniqueID, sysStatus.alertCodeNode);
	}

	return true;
}

void LoRA_Functions::loop() {
    // Put your code to run during the application thread loop here
}


// ************************************************************************
// *****					Common LoRA Functions					*******
// ************************************************************************


void LoRA_Functions::clearBuffer() {
	uint8_t bufT[RH_RF95_MAX_MESSAGE_LEN];
	uint8_t lenT;

	while(driver.recv(bufT, &lenT)) {};
}

void LoRA_Functions::sleepLoRaRadio() {
	driver.sleep();                             	// Here is where we will power down the LoRA radio module
}

bool  LoRA_Functions::initializeRadio() {  			// Set up the Radio Module
	digitalWrite(gpio.RFM95_RST,LOW);				// Reset the radio module before setup
	delay(10);
	digitalWrite(gpio.RFM95_RST,HIGH);
	delay(10);

	if (!manager.init()) {
		Log.infoln("LoRA Radio Initialization failed");					// Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
		return false;
	}
	rf95.setFrequency(RF95_FREQ);					// Frequency is typically 868.0 or 915.0 in the Americas, or 433.0 in the EU - Are there more settings possible here?
	rf95.setTxPower(23, false);                   // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then you can set transmitter powers from 5 to 23 dBm (13dBm default).  PA_BOOST?

	rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);	 // Optimized for fast transmission and short range - MAFC
	// driver.setModemConfig(RH_RF95::Bw125Cr45Sf2048); // This is the value used in the park 
	//driver.setModemConfig(RH_RF95::Bw125Cr48Sf4096);	// This optimized the radio for long range - https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html
	rf95.setLowDatarate();						// https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html#a8e2df6a6d2cb192b13bd572a7005da67
	manager.setTimeout(1000);						// 200mSec is the default - may need to extend once we play with other settings on the modem - https://www.airspayce.com/mikem/arduino/RadioHead/classRHReliableDatagram.html
return true;
}


// ************************************************************************
// *****                         Node Functions                       *****
// ************************************************************************
bool LoRA_Functions::listenForLoRAMessageNode() {
	uint8_t len = sizeof(buf);
	uint8_t from;  
	uint8_t dest;
	uint8_t id;
	uint8_t messageFlag;
	uint8_t hops;
	if (manager.recvfromAck(buf, &len, &from, &dest, &id, &messageFlag, &hops))	{					// We have received a message
		buf[len] = 0;
		if ((buf[0] << 8 | buf[1]) != sysStatus.magicNumber) {
			Log.infoln("Magic Number mismatch - ignoring message");
			return false;
		} 
		lora_state = (LoRA_State)messageFlag;
		Log.infoln("Received from node %d with RSSI / SNR of %d / %d - a %s message with %d hops", from, driver.lastRssi(), rf95.lastSNR(), loraStateNames[lora_state], hops);

		sysStatus.token = (buf[3] << 8 | buf[4]);						// Set the token for validation - good for the day
		sysStatus.lastConnection = ((buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | buf[8]);	// Correct time from gateway
		timeFunctions.setTime(sysStatus.lastConnection,0);  // Set time based on response from gateway
		uint16_t secondsTillNextReport = (buf[9] << 8 | buf[10]);			// Frequency of reporting set by Gateway
		if (secondsTillNextReport < 60) secondsTillNextReport = 60;		// Minimum of 60 seconds
		sysStatus.nextConnection = timeFunctions.getTime() + secondsTillNextReport;

		Log.infoln("Next report is in %u seconds", secondsTillNextReport);

		// Process Alert Codes
		sysStatus.alertCodeNode = buf[11];			// The gateway may set an alert code for the node
		sysStatus.alertContextNode = (buf[12] << 8 | buf[13]);		// The gateway may send alert context with an alert code

		if (sysStatus.alertCodeNode) {
			Log.infoln("The gateway set an alert %d with context %d", sysStatus.alertCodeNode, sysStatus.alertContextNode);
		}

		if (lora_state == DATA_ACK) { if(LoRA_Functions::instance().receiveAcknowledmentDataReportNode()) return true;}
		else if (lora_state == JOIN_ACK) { if(LoRA_Functions::instance().receiveAcknowledmentJoinRequestNode()) return true;}
		else {Log.infoln("Invalid LoRA message flag"); return false;}

	}
	else LoRA_Functions::clearBuffer();
	return false;
}


bool LoRA_Functions::composeDataReportNode() {

	LED.on();

	buf[0] = highByte(sysStatus.magicNumber);
	buf[1] = lowByte(sysStatus.magicNumber);
	buf[2] = sysStatus.nodeNumber;
	buf[3] = highByte(sysStatus.token);
	buf[4] = lowByte(sysStatus.token);
	buf[5] = sysStatus.sensorType;
	buf[6] = sysStatus.uniqueID >> 24;
	buf[7] = sysStatus.uniqueID >> 16;
	buf[8] = sysStatus.uniqueID >> 8;
	buf[9] = sysStatus.uniqueID;	
	buf[10] = highByte(current.occupancyGross);
	buf[11] = lowByte(current.occupancyGross);
	buf[12] = highByte(current.occupancyNet);
	buf[13] = lowByte(current.occupancyNet);
	buf[14] = sysStatus.space;								// The data payload size is constant - not all sensor types will use all 8 bytes
	buf[15] = sysStatus.placement;
	buf[16] = sysStatus.multi;
	buf[17] = sysStatus.zoneMode;
	buf[18] = current.internalTempC;
	buf[19] = current.stateOfCharge;
	buf[20] = current.batteryState;	
	buf[21] = sysStatus.resetCount;
	buf[22] = highByte(current.RSSI);
	buf[23] = lowByte(current.RSSI);
	buf[24] = highByte(current.SNR);
	buf[25] = lowByte(current.SNR);
	buf[26] = 0;		// These last two bytes are used by the radiohead library to track re-transmissions and re-transmission delays
	buf[27] = 0;

	// Send a message to manager_server
  	// A route to the destination will be automatically discovered.
	unsigned char result = manager.sendtoWait(buf, 28, GATEWAY_ADDRESS, DATA_RPT);
	
	if ( result == RH_ROUTER_ERROR_NONE) {
		// It has been reliably delivered to the next node.
		// Now wait for a reply from the ultimate server 
		current.RSSI = rf95.lastRssi();				// Set these here - will send on next data report
		current.SNR = rf95.lastSNR();
		Log.infoln("Node %d data report delivered with RSSI/SNR of %d / %d ",sysStatus.nodeNumber,current.RSSI, current.SNR);
		LED.off();
		return true;
	}
	else if (result == RH_ROUTER_ERROR_NO_ROUTE) {
        Log.infoln("Node %d - Data report send to gateway %d failed - No Route", sysStatus.nodeNumber, GATEWAY_ADDRESS);
    }
    else if (result == RH_ROUTER_ERROR_UNABLE_TO_DELIVER) {
        Log.infoln("Node %d - Data report send to gateway %d failed - Unable to Deliver", sysStatus.nodeNumber, GATEWAY_ADDRESS);
	}
	else  {
		Log.infoln("Node %d - Data report send to gateway %d failed - Unknown", sysStatus.nodeNumber, GATEWAY_ADDRESS);
	}
	LED.off();
	return false;
}

bool LoRA_Functions::receiveAcknowledmentDataReportNode() {
	// LEDStatus blinkBlue(RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
	// Need to reinstate the blinks

	// contents of response for 1-12 handled in common function above

	if (sysStatus.alertCodeNode == 6) {			// An Alert Code of 6 means the park is closed - reset everything
		Log.infoln("Park is closed - will reset current occupancy");
	}

	Log.infoln("Data report acknowledged %s alert for message %d park is %s and alert code is %d with alert context %d", (sysStatus.alertCodeNode) ? "with":"without", buf[11], (sysStatus.alertCodeNode != 6) ? "open":"closed", sysStatus.alertCodeNode, sysStatus.alertContextNode);

	return true;
}

bool LoRA_Functions::composeJoinRequesttNode() {

	manager.setThisAddress(sysStatus.nodeNumber);				// Join with the right node number

	buf[0] = highByte(sysStatus.magicNumber);					// Needs to equal 128
	buf[1] = lowByte(sysStatus.magicNumber);					// Needs to equal 128
	buf[2] = sysStatus.nodeNumber;								// Node number - typically 255 for a join request
	buf[3] = highByte(sysStatus.token);							// Token for validation - may not be valid - if it is valid - response is to set the clock only
	buf[4] = lowByte(sysStatus.token);							// Token for validation - may not be valid - if it is valid - response is to set the clock only
	buf[5] = sysStatus.sensorType;								// Identifies sensor type to Gateway
	buf[6] = sysStatus.uniqueID >> 24;							// This is a 4-byte identifier that is unique to each node and is only set once
	buf[7] = sysStatus.uniqueID >> 16;							// This is a 4-byte identifier that is unique to each node and is only set once
	buf[8] = sysStatus.uniqueID >> 8;							// This is a 4-byte identifier that is unique to each node and is only set once
	buf[9] = sysStatus.uniqueID;								// This is a 4-byte identifier that is unique to each node and is only set once
	buf[10] = sysStatus.space;									// These next four bytes are sent to the gateway and may be updated in join process
	buf[11] = sysStatus.placement;
	buf[12] = sysStatus.multi;
	buf[13] = 0;												// Reserved for future use
	buf[14] = 0;												// These last two bytes are used by the radiohead library to track re-transmissions and re-transmission delays
	buf[15] = 0;
	
	Log.infoln("Node %d Sending join request with magicNumer = %d, uniqueID = %u and sensorType = %d, and payload %d / %d/ %d",sysStatus.nodeNumber, sysStatus.magicNumber, sysStatus.uniqueID, sysStatus.sensorType, sysStatus.space,sysStatus.placement, sysStatus.multi);

	LED.on();
	unsigned char result = manager.sendtoWait(buf, 16, GATEWAY_ADDRESS, JOIN_REQ);
	LED.off();

	if (result == RH_ROUTER_ERROR_NONE) {						// It has been reliably delivered to the next node.
		current.RSSI = rf95.lastRssi();						// Set these here - will send on next data report
		current.SNR = rf95.lastSNR();
		Log.infoln("Join request sent to gateway successfully RSSI/SNR of %d / %d ",current.RSSI, current.SNR);
		return true;
	}
	else if (result == RH_ROUTER_ERROR_NO_ROUTE) {
        Log.infoln("Node %d - Join request to Gateway %d failed - No Route", sysStatus.nodeNumber, GATEWAY_ADDRESS);
    }
    else if (result == RH_ROUTER_ERROR_UNABLE_TO_DELIVER) {
        Log.infoln("Node %d -  Join request to Gateway %d failed - Unable to Deliver", sysStatus.nodeNumber, GATEWAY_ADDRESS);
	}
	else  {
		Log.infoln("Node %d -  Join request to Gateway %d failed  - Unknown", sysStatus.nodeNumber, GATEWAY_ADDRESS);
	}
	return false;
}

bool LoRA_Functions::receiveAcknowledmentJoinRequestNode() {
	// LEDStatus blinkOrange(RGB_COLOR_ORANGE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
	// Activate LED

	// contents of response for 1-12 handled in common function above
	// In a join request, the gateway will need to confirm the node number, set the uniqueID if this is a virgin device and set the sensor type and send a valid token

	if (sysStatus.nodeNumber == 255 || sysStatus.nodeNumber == 0) sysStatus.nodeNumber = buf[18];		// Set the node number future use

	if(sysStatus.sensorType != buf[13]) {
		sysStatus.sensorType = buf[13];
		asset.setup(sysStatus.sensorType);
		Log.infoln("Node %d Join request acknowledged and sensorType updated to %d", sysStatus.nodeNumber, sysStatus.sensorType);
	} else {
		Log.infoln("Node %d Join request acknowledged - sensorType up to date.", sysStatus.nodeNumber);
	}

	Log.infoln("Testing to see if we have a valid uniqueID of %u with %d and %d", sysStatus.uniqueID, sysStatus.uniqueID >> 24 , sysStatus.uniqueID >> 16);
	if (sysStatus.uniqueID >> 24 == 255 && (0XFF & sysStatus.uniqueID >> 16) == 255) {			// If the uniqueID is not set, set it here
		sysStatus.uniqueID = (buf[14] << 24 | buf[15] << 16 | buf[16] << 8 | buf[17]);
		sysData.updateUniqueID();														// Update the memory with the new uniqueID	
		Log.infoln("Node %d Join request acknowledged and sensor set to %d - received uniqueID %u",sysStatus.nodeNumber, sysStatus.sensorType, sysStatus.uniqueID);
	}
	
	if(sysStatus.space != buf[19]) {
		if(buf[19] > 63){
			sysStatus.space = 0;
		} else {
			sysStatus.space = buf[19];
		}
		Log.infoln("Node %d Join request acknowledged - space set to %d", sysStatus.nodeNumber, sysStatus.space);
	}
	if (sysStatus.sensorType == 10) {
		if(sysStatus.placement != buf[20]) {
			sysStatus.placement = buf[20];
			Log.infoln("Node %d Join request acknowledged - placement set to %d", sysStatus.nodeNumber, sysStatus.placement);
		}
		if(sysStatus.multi != buf[21]) {
			sysStatus.multi = buf[21];
		 	Log.infoln("Node %d Join request acknowledged - multiEntrance set to %d", sysStatus.nodeNumber, sysStatus.multi);
		}
	}
	manager.setThisAddress(sysStatus.nodeNumber);
	sysStatus.alertCodeNode = 0;									// Need to clear so we don't get in a retry cycle

	return true;
}