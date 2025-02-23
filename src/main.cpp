#include <Arduino.h>
#include "components/BatteryManager.h"
#include "components/HeatingManager.h"
#include "components/PowerManager.h"
#include "utils/BLEUtils.h"
#include "components/Thermistor.h"

// Declare global pointers to managers for use in loop()
BatteryManager *batteryManager;
HeatingManager *heatingManager;
PowerManager *powerManager;

int presetBatteryLevel = 90;

void setup()
{
  Serial.begin(115200);

  // Initialize BLE and service
  BLEDevice::init("BootsESP32");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService("12345678-90AB-CDEF-1234-567890ABCDEF");

  // Initialize managers
  batteryManager = new BatteryManager(pService, pServer);
  heatingManager = new HeatingManager(pService, pServer);
  powerManager = new PowerManager(pService, pServer);

  // Initial values for each component
  batteryManager->setBatteryLevel(presetBatteryLevel);
  batteryManager->setChargingStatus(true);
  batteryManager->setBatteryHealth(80);

  heatingManager->setHeatingStatus("ON");
  heatingManager->setTemperature(30);

  powerManager->setPowerStatus("ON");
  powerManager->setLastPoweredOn("2023-11-20T10:00:00Z");

  // Start the service and advertising
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("12345678-90AB-CDEF-1234-567890ABCDEF");
  BLEDevice::startAdvertising();

  Serial.println("BLE Server and Managers Initialized");
}

void loop()
{
  static unsigned long lastUpdateTime = 0; // Track the last update time
  unsigned long currentTime = millis();

  if (currentTime - lastUpdateTime >= 2000)
  {
    lastUpdateTime = currentTime;

    // Generate a random battery level between 0% and 100%
    int newBatteryLevel = presetBatteryLevel--;

    // Generate a random temperature between 20°C and 30°C
    int randomTemperature = random(20, 31);

    // Update managers with the new data
    batteryManager->setBatteryLevel(newBatteryLevel);
    heatingManager->setTemperature(randomTemperature);

    // Log the updates
    Serial.println("Battery Level Updated: " + String(newBatteryLevel));
    Serial.println("Heating Temperature Updated: " + String(randomTemperature));
  }

  delay(1);
}
