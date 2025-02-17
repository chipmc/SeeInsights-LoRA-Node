#include "Accelerometer.h"

// Declare instance as null
Accelerometer* Accelerometer::_instance = nullptr;

// Default constructor, SA0 pin is HIGH
MMA8452Q accel; 

// Constructor
Accelerometer::Accelerometer() {}

// Destructor
Accelerometer::~Accelerometer() {}

// Singleton pattern [static]
Accelerometer& Accelerometer::instance() {
    if (!_instance) {
        _instance = new Accelerometer();
    }
    return *_instance;
}

bool Accelerometer::setup() {
    // Initialize the accelerometer with begin():
	// begin can take two parameters: full-scale range, and output data rate (ODR).
	// Full-scale range can be: SCALE_2G, SCALE_4G, or SCALE_8G (2, 4, or 8g)
	// ODR can be: ODR_800, ODR_400, ODR_200, ODR_100, ODR_50, ODR_12, ODR_6 or ODR_1
    if (!accel.begin(SCALE_2G, ODR_100)) {
        Log.infoln("Communication with accelerometer failed");     // Set up accel with +/-2g range, and 100Hz ODR
        return false;
    }

    sysStatus.sensitivity = 1;

    accel.setupTapIntsLatch(sysStatus.sensitivity);                        // Set up the tap interrupt

    accel.clearTapInts();

    // To update acceleration values from the accelerometer, call accel.read();
    accel.read();

	// After reading, six class variables are updated: x, y, z, cx, cy, and cz.
	// Those are the raw, 12-bit values (x, y, and z) and the calculated
	// acceleration's in units of g (cx, cy, and cz).

    Log.infoln("Tap Sensor initialized with sensitivity %d and (%F,%F,%F)g's",sysStatus.sensitivity, accel.cx, accel.cy, accel.cz);
    return true;
}

bool Accelerometer::readData() {
    static bool lastOccupancyState = false;
    static time_t occupancyPeriodStart = 0;
    
    if (digitalRead(gpio.I2C_INT)) {                                
        Log.infoln("Occupancy detected");
        accel.clearTapInts();
        current.occupancyNet = 1;
        LED.on();                                               // Turn on the indicator LED - take out for low power operations
    }

    time_t now = timeFunctions.getTime();
    if (current.occupancyNet == 1) {                            // Occupancy detected
        LED.on();                                               // Turn on the LED - can get turned off in the LoRA class
        if (!lastOccupancyState) {                              // This is a new occupancy period
            lastOccupancyState = true;                          // Set the last state to true  
            occupancyPeriodStart = timeFunctions.getTime();		// Begin a new period of occupancy  
            Log.infoln("Starting a new occupancy period at %d", occupancyPeriodStart);
        }
        else if (lastOccupancyState && ((now - occupancyPeriodStart) > sysStatus.debounceMin * 60UL)) {   // Occupancy is no longer detected
            lastOccupancyState = false;                             // End the period of occupancy
            current.occupancyNet = 0;                               // State that we are occupied
            current.occupancyGross = timeFunctions.getTime() - occupancyPeriodStart;     // Gross occupancy is net occupancy time
            currentData.currentDataChanged = true;                  // Set the flag to save the data
            Log.infoln("Occupancy period has ended - total occupancy today is currently %d seconds", current.occupancyGross);
            LED.off();                                              // Turn off the LED now that occupancy is over
        }
        else {
            Log.infoln("Continue current occupancy for %d more seconds", (sysStatus.debounceMin * 60UL) -(timeFunctions.getTime() - occupancyPeriodStart)); // This will run every loop if occupancy is detected
        }
    }
    return true;
}