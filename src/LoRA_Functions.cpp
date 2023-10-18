#include "LoRA_Functions.h"

// Singleton instantiation - from template
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
// In this implementation - we have one gateway numde number 0 and up to 10 nodes with node numbers 1-10
// Node numbers greater than 10 initiate a join request
const uint8_t GATEWAY_ADDRESS = 0;
// const double RF95_FREQ = 915.0;				 	// Frequency - ISM
const double RF95_FREQ = 926.84;				// Center frequency for the omni-directional antenna I am using

// Define the message flags
typedef enum { NULL_STATE, JOIN_REQ, JOIN_ACK, DATA_RPT, DATA_ACK, ALERT_RPT, ALERT_ACK} LoRA_State;
char loraStateNames[7][16] = {"Null", "Join Req", "Join Ack", "Data Report", "Data Ack", "Alert Rpt", "Alert Ack"};
static LoRA_State lora_state = NULL_STATE;

// Singleton instance of the radio driver
RH_RF95 driver(gpio.RFM95_CS, gpio.RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh manager(driver, GATEWAY_ADDRESS);

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

	Log.infoln("in LoRA setup - node number %d",sysStatus.nodeNumber);

	if (gatewayID == true) {
		sysStatus.nodeNumber = GATEWAY_ADDRESS;							// Gateway - Manager is initialized by default with GATEWAY_ADDRESS - make sure it is stored in FRAM
		Log.infoln("LoRA Radio initialized as a gateway with a deviceID of %i", sysStatus.deviceID);
	}
	else if (sysStatus.nodeNumber > 0 && sysStatus.nodeNumber <= 10) {
		manager.setThisAddress(sysStatus.nodeNumber);// Node - use the Node address in valid range from memory
		Log.infoln("LoRA Radio initialized as node %i and a deviceID of %i", manager.thisAddress(), sysStatus.deviceID);
	}
	else {																						// Else, we will set as an unconfigured node
		sysStatus.nodeNumber = 11;
		manager.setThisAddress(11);
		sysStatus.alertCodeNode = 1;															// Join request required
		Log.infoln("LoRA Radio initialized as an unconfigured node %i and a deviceID of %i and alert code %d", manager.thisAddress(), sysStatus.deviceID, sysStatus.alertCodeNode);
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
	digitalWrite(gpio.RFM95_RST,LOW);					// Reset the radio module before setup
	delay(10);
	digitalWrite(gpio.RFM95_RST,HIGH);
	delay(10);

	if (!manager.init()) {
		Log.infoln("init failed");					// Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
		return false;
	}
	driver.setFrequency(RF95_FREQ);					// Frequency is typically 868.0 or 915.0 in the Americas, or 433.0 in the EU - Are there more settings possible here?
	driver.setTxPower(23, false);                   // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then you can set transmitter powers from 5 to 23 dBm (13dBm default).  PA_BOOST?

	driver.setModemConfig(RH_RF95::Bw125Cr45Sf2048);
	//driver.setModemConfig(RH_RF95::Bw125Cr48Sf4096);	// This optimized the radio for long range - https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html
	driver.setLowDatarate();						// https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html#a8e2df6a6d2cb192b13bd572a7005da67
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
		Log.infoln("Received from node %d with RSSI / SNR of %d / %d - a %s message with %d hops", from, driver.lastRssi(), driver.lastSNR(), loraStateNames[lora_state], hops);

		tm.setTime(((buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | buf[5]),0);  // Set time based on response from gateway
		sysStatus.frequencyMinutes = (buf[6] << 8 | buf[7]);			// Frequency of reporting set by Gateway

		// The gateway may set an alert code for the node
		sysStatus.alertCodeNode = buf[8];
		sysStatus.alertTimestampNode = tm.getTime();

		Log.infoln("Set clock to %i and report frequency to %d minutes", tm.getTime(),sysStatus.frequencyMinutes);

		if (lora_state == DATA_ACK) { if(LoRA_Functions::instance().receiveAcknowledmentDataReportNode()) return true;}
		else if (lora_state == JOIN_ACK) { if(LoRA_Functions::instance().receiveAcknowledmentJoinRequestNode()) return true;}
		else {Log.infoln("Invaled LoRA message flag"); return false;}

	}
	else LoRA_Functions::clearBuffer();
	return false;
}


bool LoRA_Functions::composeDataReportNode() {
	float successPercent;

	if (current.messageCount == 0) {		// 8-bit number so need to protect against divide by zero on reset or wrap around
		successPercent = 0.0;	
		current.messageCount = 0;
		current.successCount = 0;
	}
	else successPercent = ((current.successCount+1.0)/(float)current.messageCount) * 100.0;  // Add one to success because we are receving the message
	current.messageCount = current.messageCount+1;

	LED.on();

	int deviceIDCheckSum = sysStatus.deviceID;

	buf[0] = highByte(sysStatus.magicNumber);
	buf[1] = lowByte(sysStatus.magicNumber);			
	buf[2] = highByte(deviceIDCheckSum);
	buf[3] = lowByte(deviceIDCheckSum);
	buf[4] = highByte(current.hourlyCount);
	buf[5] = lowByte(current.hourlyCount); 
	buf[6] = highByte(current.dailyCount);
	buf[7] = lowByte(current.dailyCount); 
	buf[8] = sysStatus.sensorType;
	buf[9] = current.internalTempC;
	buf[10] = current.stateOfCharge;
	buf[11] = current.batteryState;	
	buf[12] = sysStatus.resetCount;
	buf[13] = current.messageCount;
	buf[14] = current.successCount;
	buf[15] = highByte(current.RSSI);
	buf[16] = lowByte(current.RSSI);
	buf[17] = highByte(current.SNR);
	buf[18] = lowByte(current.SNR);

	// Send a message to manager_server
  	// A route to the destination will be automatically discovered.
	unsigned char result = manager.sendtoWait(buf, 19, GATEWAY_ADDRESS, DATA_RPT);
	
	if ( result == RH_ROUTER_ERROR_NONE) {
		// It has been reliably delivered to the next node.
		// Now wait for a reply from the ultimate server 
		current.successCount = current.successCount + 1;
		current.RSSI = driver.lastRssi();				// Set these here - will send on next data report
		current.SNR = driver.lastSNR();
		Log.infoln("Node %d data report delivered - success rate %4.2f and  RSSI/SNR of %d / %d ",sysStatus.nodeNumber,successPercent,current.RSSI, current.SNR);
		LED.off();
		return true;
	}
	else if (result == RH_ROUTER_ERROR_NO_ROUTE) {
        Log.infoln("Node %d - Data report send to gateway %d failed - No Route - success rate %4.2f", sysStatus.nodeNumber, GATEWAY_ADDRESS, successPercent);
    }
    else if (result == RH_ROUTER_ERROR_UNABLE_TO_DELIVER) {
        Log.infoln("Node %d - Data report send to gateway %d failed - Unable to Deliver - success rate %4.2f", sysStatus.nodeNumber, GATEWAY_ADDRESS,successPercent);
	}
	else  {
		Log.infoln("Node %d - Data report send to gateway %d failed  - Unknown - success rate %4.2f", sysStatus.nodeNumber, GATEWAY_ADDRESS,successPercent);
	}
	LED.off();
	return false;
}

bool LoRA_Functions::receiveAcknowledmentDataReportNode() {
	// LEDStatus blinkBlue(RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
	// Need to reinstate the blinks

	// contents of response for 1-7 handled in common function above
	sysStatus.alertCodeNode = buf[8];

	if (sysStatus.alertCodeNode == 7) {		// This alert triggers an update to the sensor type on the node - handle it here
		Log.infoln("The gatway is updating sensor type from %d to %d", sysStatus.sensorType, buf[9]);
		sysStatus.sensorType = buf[9];
		sysStatus.alertCodeNode = 0;				// Sensor updated - clear alert
	}
	else if (sysStatus.alertCodeNode) {
		Log.infoln("The gateway set an alert %d", sysStatus.alertCodeNode);
		sysStatus.alertTimestampNode = tm.getTime();	
	}

	sysStatus.openHours = buf[10];				// The Gateway tells us whether the park is open or closed

	if (sysStatus.openHours == 0) {			// Open Hours Processing
		currentData.resetEverything();
		Log.infoln("Park is closed - reset everything");
	}
	else sysStatus.openHours = true;

	Log.infoln("Data report acknowledged %s alert for message %d park is %s and alert code is %d", (sysStatus.alertCodeNode) ? "with":"without", buf[11], (buf[10] ==1) ? "open":"closed", sysStatus.alertCodeNode);
	
	// blinkBlue.setActive(true);
	// Reactivate blinking
	unsigned long strength = (unsigned long)(map(current.RSSI,-10,-140,3000,100));
	strength = constrain(strength,100UL,3000UL);
    delay(strength);
    // blinkBlue.setActive(false);

	return true;
}

bool LoRA_Functions::composeJoinRequesttNode() {


	manager.setThisAddress(sysStatus.nodeNumber);				// Join with the right node number

	buf[0] = highByte(sysStatus.magicNumber);					// Needs to equal 128
	buf[1] = lowByte(sysStatus.magicNumber);					// Needs to equal 128
	buf[2] = highByte(sysStatus.deviceID);
	buf[3] = lowByte(sysStatus.deviceID);
	buf[4] = highByte(sysStatus.deviceID);
	buf[5] = lowByte(sysStatus.deviceID);
	// Don't need all of this
	buf[29] = sysStatus.sensorType;

	LED.on();
	unsigned char result = manager.sendtoWait(buf, 30, GATEWAY_ADDRESS, JOIN_REQ);
	LED.off();


	Log.infoln("Progress");

	if (result == RH_ROUTER_ERROR_NONE) {					// It has been reliably delivered to the next node.
		current.RSSI = driver.lastRssi();				// Set these here - will send on next data report
		current.SNR = driver.lastSNR();
		Log.infoln("Join request sent to gateway successfully RSSI/SNR of %d / %d ",current.RSSI, current.SNR);
		return true;
	}
	else {
		Log.infoln("Join request to Gateway failed");
		return false;
	}
}

bool LoRA_Functions::receiveAcknowledmentJoinRequestNode() {
	//LEDStatus blinkOrange(RGB_COLOR_ORANGE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
	// Activate LED

	if (sysStatus.nodeNumber > 10) sysStatus.nodeNumber = buf[9];
	sysStatus.sensorType = buf[10];
	Log.infoln("Node %d Join request acknowledged and sensor set to %d", sysStatus.nodeNumber, sysStatus.sensorType);
	manager.setThisAddress(sysStatus.nodeNumber);

    // blinkOrange.setActive(true);
	unsigned long strength = (unsigned long)(map(current.RSSI,-10,-140,3000,100));
	strength = constrain(strength,100UL,3000UL);
    delay(strength);
    // blinkOrange.setActive(false);

	return true;
}


int LoRA_Functions::stringCheckSum(String str){												// This function is made for the Particle DeviceID
    int result = 0;
    for(unsigned int i = 0; i < str.length(); i++){
      int asciiCode = (int)str[i];

      if (asciiCode >=48 && asciiCode <58) {              // 0-9
        result += asciiCode - 48;
      } 
      else if (asciiCode >=65 && asciiCode < 71) {        // A-F
        result += 10 + asciiCode -65;
      }
      else if (asciiCode >=97 && asciiCode < 103) {       // a - f
        result += 10 + asciiCode -97;
      }
    }
    return result;
}

