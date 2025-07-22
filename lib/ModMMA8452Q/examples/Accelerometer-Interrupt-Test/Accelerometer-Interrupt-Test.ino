/*	Accelerometer-Interrupt-Test.ino
	Chip McClelland (chip@seeinsights.com)
	July 15, 2021
	https://github.com/sparkfun/SparkFun_MMA8452Q_Particle_Library

	In this skech we use the interrupt capabilities of the MMA8452Q

	Development environment specifics:
	Particle Build environment (https://www.particle.io/build)
	PAdafruit Feather m0
	Distributed as-is; no warranty is given. 
*/
// Include the library:
//Include Libraries:
#include <arduino.h>
#include <ArduinoLowPower.h>
#include <ArduinoLog.h>     // https://github.com/thijse/Arduino-Log
#include <Wire.h>
#include "Accelerometer/ModMMA8452Q.h"
#include "pinout.h"
#include "stsLED.h"

// Create an MMA8452Q object, used throughout the rest of the sketch.
MMA8452Q accel; // Default constructor, SA0 pin is HIGH

// Prototype Functions
void sensorISR();

// The above works if the MMA8452Q's address select pin (SA0) is high.
// If SA0 is low (if the jumper on the back of the SparkFun MMA8452Q breakout
// board is closed), initialize it like this:
// MMA8452Q accel(MMA8452Q_ADD_SA0_);

// Sensor Variables
volatile bool sensorDetect = false;                 // This is the flag that an interrupt is triggered
byte sensitivity = 1;                              // This is the sensitivity of the accelerometer - 0 is lowest and 10 is the highest

void setup()
{

	Wire.begin(); 										// Establish Wire.begin for I2C communication
	Serial.begin(115200);								//Establish Serial connection if connected for debugging
	delay(2000);

	// Log.begin(LOG_LEVEL_SILENT, &Serial);
	Log.begin(LOG_LEVEL_INFO, &Serial);
	Log.infoln("PROGRAM: See Insights LoRa Node - Accelerometer Test");

	gpio.setup(); // Setup the pins
 	LED.setup(gpio.STATUS);								// Led used for status
	LED.on();                      // Turn on the LED to show the program is running

	// Initialize the accelerometer with begin():
	// begin can take two parameters: full-scale range, and output data rate (ODR).
	// Full-scale range can be: SCALE_2G, SCALE_4G, or SCALE_8G (2, 4, or 8g)
	// ODR can be: ODR_800, ODR_400, ODR_200, ODR_100, ODR_50, ODR_12, ODR_6 or ODR_1
  	accel.begin(SCALE_2G, ODR_100); // Set up accel with +/-2g range, and 100Hz ODR

  	accel.setupTapIntsLatch(sensitivity);                               // Set up the tap interrupt

  	accel.clearTapInts();

  	attachInterrupt(gpio.I2C_INT, sensorISR, RISING);                   // Accelerometer interrupt from low to high

  	LED.off();                                                          //  End of the setup routine
}

void loop()
{
  	if (sensorDetect == true) {                                         // Interrupt flag raised - need to report a tap
    	LED.on();
    	Log.infoln("Accelerometer Interrupt Triggered");                  // Report entering tap handling
    	sensorDetect = false;                                           // Reset the flag
    	delay(5000);                                                    // debounce
    	accel.clearTapInts();                                           // Reads the interrupt register clearing the latched interrupt
    	LED.off();
  	}
}

void sensorISR() {
	sensorDetect = true;                                              // sets the sensor flag for the main loop
}
