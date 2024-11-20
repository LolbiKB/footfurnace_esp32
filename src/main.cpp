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

Thermistor thermistor(
    3950,   // Beta value
    10000,  // Resistance at T0 (10kΩ)
    298.15, // Reference temperature in Kelvin (25°C)
    10000,  // Pull-up resistor value (10kΩ)
    3.3,    // Reference voltage
    4096    // ADC resolution for 12-bit ADC
);

float adcToVoltage(int adcValue, float referenceVoltage = 3.3, int adcResolution = 4096)
{
  // Ensure ADC value is within valid range
  if (adcValue < 0 || adcValue >= adcResolution)
  {
    Serial.println("Error: ADC value out of range.");
    return -1; // Return an invalid value to indicate an error
  }

  // Convert ADC value to voltage
  float voltage = (adcValue / float(adcResolution - 1)) * referenceVoltage;
  return voltage;
}

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
  StaticJsonDocument<256> batteryUpdate;
  batteryUpdate["batteryLevel"] = 90;
  batteryUpdate["chargingStatus"] = true;
  batteryUpdate["batteryHealth"] = 80;
  batteryUpdate["batteryTimeLeft"] = "5:00";
  batteryUpdate["chargingTime"] = "1:30";
  batteryManager->updateBatteryData(batteryUpdate.as<JsonObject>());

  StaticJsonDocument<256> heatingUpdate;
  heatingUpdate["heatingStatus"] = "ON";
  heatingUpdate["temperature"] = 30;
  heatingManager->updateHeatingData(heatingUpdate.as<JsonObject>());

  StaticJsonDocument<256> powerUpdate;
  powerUpdate["powerStatus"] = "ON";
  powerUpdate["lastPoweredOn"] = "2023-11-20T10:00:00Z";
  powerManager->updatePowerData(powerUpdate.as<JsonObject>());

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
    int randomBatteryLevel = random(0, 101);

    // Create a JSON object with only the battery level
    StaticJsonDocument<256> batteryUpdate;
    batteryUpdate["batteryLevel"] = randomBatteryLevel;

    // Update the battery characteristic
    batteryManager->updateBatteryData(batteryUpdate.as<JsonObject>());

    // Calculate resistance and temperature
    float resistance = 10000.0 / ((3.3 / (analogReadMilliVolts(25) / 1000.0)) - 1);
    float temperature = thermistor.resistanceToTemperature(resistance);

    // Limit temperature to two decimal places
    float roundedTemperature = round(temperature * 100.0) / 100.0;

    // Log resistance and temperature
    Serial.println("Resistance: " + String(resistance, 2) + " Ω"); // Resistance to 2 decimal places
    Serial.print("Temperature: ");
    Serial.print(roundedTemperature);
    Serial.println(" °C");

    // Update temperature characteristic
    StaticJsonDocument<256> heatingUpdate;
    heatingUpdate["temperature"] = roundedTemperature;

    heatingManager->updateHeatingData(heatingUpdate.as<JsonObject>());
  }

  delay(1); // Avoid busy-waiting
}
