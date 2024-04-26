#include "SerialConnection.h"

char buffer[256];   // Buffer to hold serial data

// Declaration of null instance
SerialConnection* SerialConnection::_instance = nullptr;

// Constructor
SerialConnection::SerialConnection() {}

// Destructor
SerialConnection::~SerialConnection() {}

// Singleton pattern [static]
SerialConnection &SerialConnection::instance() {
    if (!_instance) {
        _instance = new SerialConnection();
    }
    return *_instance;
}

bool SerialConnection::initialize() {
    Serial1.begin(115200);                              // Open serial port to communicate with Serial1 device
    Log.info("Initializing Serial Connection");
    return true;
}

bool SerialConnection::sendMessage(const char *message) {       // This function will send a message to the serial asset                         
    if (Serial1.available()) {
        Serial1.println(message);
        return true;
    }
    Log.info("Could not send message - Serial1 is not available");
    return false;
}

bool SerialConnection::receiveMessage(char *response, int responseSize) {       // This function will return the response from the Serial1 device
                                 
    unsigned long readStart = millis();   // Safely wait here until there is data available to read - takes the device a beat to respond.
    while(millis() - readStart < 4000){}; // Give plenty of time for the device to output.

    while(Serial1.available()) {                                             // Check if there is data available to read                 
        int numBytes = Serial1.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
        if (numBytes > 0) {
            buffer[numBytes] = 0;
            strncpy(response, buffer, responseSize);                         // Copy the response to the response buffer
            buffer[0] = 0;                                                   // Clear the buffer
            // Clear any remaining data in the Serial1 output buffer
            while (Serial1.available()) {
                delay(10);
                Serial1.read();
            }
            return true;                                                     // Return true to indicate that there was a response
        }
        else {
            Log.info("SerialConnection received no bytes");
            return false;
        }
    }
    return false;
}

