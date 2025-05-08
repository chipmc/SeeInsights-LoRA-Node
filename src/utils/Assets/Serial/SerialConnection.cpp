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
    Serial1.begin(9600);  // Match OpenMV UART3 baud rate
    unsigned long startTime = millis();
    Log.infoln("Initializing Serial1 connection...");

    // Send PING and wait for ACK within 20 seconds
    while (millis() - startTime < 10000) {
        Serial1.println("PING");
        delay(500);  // Give OpenMV time to respond

        if (Serial1.available()) {
            int numBytes = Serial1.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
            if (numBytes > 0) {
                buffer[numBytes] = '\0';  // Null-terminate

                if (strcmp(buffer, "ACK") == 0) {
                    Log.infoln("Serial1 connection established successfully!");
                    return true;
                }
            }
        }
    }

    Log.errorln("Failed to establish Serial1 connection - Timeout after 10 seconds");
    return false;
}

bool SerialConnection::sendMessage(const char *message) {       
    Serial1.println(message);
    Log.infoln("Sent message \"%s\" over Serial1.", message);
    return true;
}

bool SerialConnection::receiveMessage(char *response, int responseSize) {      
    unsigned long readStart = millis();

    while (millis() - readStart < 4000) {  // Wait up to 4 seconds for data
        if (Serial1.available()) {
            int numBytes = Serial1.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
            if (numBytes > 0) {
                buffer[numBytes] = '\0';  // Null-terminate
                strncpy(response, buffer, responseSize);
                response[responseSize - 1] = '\0';  // Ensure null termination

                // Clear any remaining data in the buffer
                while (Serial1.available()) {
                    Serial1.read();
                    delay(10);
                }

                Log.infoln("Received object detection data: %s - SerialConnection::receiveMessage()", response);
                return true;
            }
        }
    }

    Log.errorln("Timeout: No response received - SerialConnection::receiveMessage()");
    return false;
}
