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
#include "config.h"
#include "timing.h"
#include "stsLED.h"
#include "sensors.h"

#define IRQ_Invalid 0
#define IRQ_AB1805 1


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
  //Establish Serial connection if connected for debugging
  Serial.begin(9600);
  while (!Serial && !Serial.available()) {};
  randomSeed(analogRead(0));

  delay(2000);

  Serial.println("Starting up...!");
  // Log.begin(LOG_LEVEL_SILENT, &Serial);
  Log.begin(LOG_LEVEL_TRACE, &Serial);


  Wire.begin(); //Establish Wire.begin for I2C communication

  gpio.setup();


  delay(2000);
  Log.infoln("Testing the LED");
  LED.setup(gpio.PB_LT);
  // LED.pwm(20); // Turn on the status LED but at a dim level
  digitalWrite(gpio.PB_LT, HIGH); // Turn on the status LED but at a dim level
  // LED.on();
  delay(5000);
  LED.off();
  digitalWrite(gpio.PB_LT, LOW); // Turn on the status LED but at a dim level
  delay(1000);



  Log.infoln("PROGRAM: See Insights LoRa Node!" CR);
  Log.info("Starting up..." CR);

  Log.notice("This is a notice message" CR);
  

  // LowPower.attachInterruptWakeup(gpio.RFM95_INT, wakeUp_RFM95_IRQ, RISING);
  // LowPower.attachInterruptWakeup(gpio.IRQ, wakeUp_IRQ, FALLING);
  // DIO0 is an extra interrupt output from the radio. Currently not used. Could be used for LoRaWAN and/or CAD sleep in the future. 
  // LowPower.attachInterruptWakeup(gpio.RFM95_DIO0, wakeUp_RFM95_DIO0, RISING);

  //Initialize each class used in this program
  cfg.setup();
  sns.setup();
  tm.setup();

}

/*************************************************************************************/
//                        LOOP
/*************************************************************************************/
void loop()
{ 

  tm.loop();          // Pet the hardware watchdog
  LED.loop();         // Update the Status LED
  sns.loop();         // Update the sensors

}


