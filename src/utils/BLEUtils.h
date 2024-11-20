#ifndef BLEUTILS_H
#define BLEUTILS_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEService.h>

// Function to initialize the BLE server and return the service
BLEService* initializeBLEService() {
    BLEDevice::init("BootsESP32"); // Initialize BLE with a device name
    BLEServer *pServer = BLEDevice::createServer(); // Create a BLE server
    BLEService *pService = pServer->createService("12345678-90AB-CDEF-1234-567890ABCDEF"); // Create a service with the provided UUID

    Serial.println("BLE service initialized.");
    return pService; // Return the service for further use
}

#endif // BLEUTILS_H
