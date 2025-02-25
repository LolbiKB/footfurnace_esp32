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

// Modify constants
const double HEATING_RATE = 0.4;      // °C per second when heating
const double COOLING_RATE = 0.05;     // °C per second when cooling
const double BATTERY_DRAIN_RATE = 0.04; // % per second when heating
const double AMBIENT_TEMP = 20.0;     // Room temperature in °C
const double AMBIENT_INFLUENCE = 0.03; // Rate at which ambient temp affects current temp
const double MAINTENANCE_THRESHOLD = 1.0; // ±1°C threshold for maintenance mode
const unsigned long UPDATE_INTERVAL = 1000;
const int HEATING_PIN = GPIO_NUM_18;  // Define heating control pin using GPIO9

const int LED_BLE_PIN = GPIO_NUM_4;   // LED for BLE status (flashing when disconnected)
const int LED_BATTERY_OK_PIN = GPIO_NUM_2;   // LED for battery > 30%
const int LED_BATTERY_LOW_PIN = GPIO_NUM_15;  // LED for battery < 20%
unsigned long previousLEDMillis = 0;  // For LED flashing
const int LED_FLASH_INTERVAL = 500;   // Flash every 500ms
bool ledState = false;  // For LED flashing state

const unsigned long BLE_CONNECTED_LED_DURATION = 5000;  // 5 seconds in milliseconds
unsigned long bleConnectedTime = 0;
bool wasDisconnected = true;  // Track connection state changes

double currentTemperature = AMBIENT_TEMP;
unsigned long previousMillis = 0;

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
  batteryManager->setChargingStatus(false);
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

  // Initialize heating control pin
  pinMode(HEATING_PIN, OUTPUT);
  digitalWrite(HEATING_PIN, LOW);  // Start with heating off

  // Initialize LED pins
  pinMode(LED_BLE_PIN, OUTPUT);
  pinMode(LED_BATTERY_OK_PIN, OUTPUT);
  pinMode(LED_BATTERY_LOW_PIN, OUTPUT);
  
  // Start with all LEDs off
  digitalWrite(LED_BLE_PIN, LOW);
  digitalWrite(LED_BATTERY_OK_PIN, LOW);
  digitalWrite(LED_BATTERY_LOW_PIN, LOW);

  Serial.println("BLE Server and Managers Initialized");
}

void loop()
{
  unsigned long currentMillis = millis();
  double deltaTime = (currentMillis - previousMillis) / 1000.0;
  
  // Handle LED states with priority
  if (!heatingManager->getServer()->getConnectedCount()) {
    // Priority 1: No BLE connection - flash LED
    digitalWrite(LED_BATTERY_OK_PIN, LOW);
    digitalWrite(LED_BATTERY_LOW_PIN, LOW);
    wasDisconnected = true;
    
    if (currentMillis - previousLEDMillis >= LED_FLASH_INTERVAL) {
      previousLEDMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_BLE_PIN, ledState);
    }
  } else {
    // Handle initial connection
    if (wasDisconnected) {
      bleConnectedTime = currentMillis;
      wasDisconnected = false;
    }
    
    // Keep BLE LED on for 5 seconds after connection
    if (currentMillis - bleConnectedTime < BLE_CONNECTED_LED_DURATION) {
      digitalWrite(LED_BLE_PIN, HIGH);
    } else {
      digitalWrite(LED_BLE_PIN, LOW);
      
      // Handle battery status LEDs
      int batteryLevel = batteryManager->getBatteryLevel();
      if (batteryLevel < 30) {
        digitalWrite(LED_BATTERY_OK_PIN, LOW);
        digitalWrite(LED_BATTERY_LOW_PIN, HIGH);
      } else {
        digitalWrite(LED_BATTERY_OK_PIN, LOW);
        digitalWrite(LED_BATTERY_LOW_PIN, LOW);
      }
    }
  }

  if (currentMillis - previousMillis >= UPDATE_INTERVAL) {
    previousMillis = currentMillis;
    
    double targetTemp = heatingManager->getTargetTemperature();  // Changed to double
    int batteryLevel = batteryManager->getBatteryLevel();
    
    // Calculate temperature difference
    double tempDiff = abs(currentTemperature - targetTemp);
    
    // Update UI status 
    if (batteryLevel <= 0) {
      heatingManager->setHeatingStatus("OFF");  // Force OFF when battery dead
    } else if (tempDiff <= MAINTENANCE_THRESHOLD) {
      heatingManager->setHeatingStatus("MTN");
    } else if (currentTemperature < targetTemp) {
      heatingManager->setHeatingStatus("ON");
    } else {
      heatingManager->setHeatingStatus("OFF");
    }
    
    // Heating logic
    if (currentTemperature < targetTemp && batteryLevel > 0) {
      // Active heating only when battery has power
      currentTemperature += HEATING_RATE * deltaTime;
      batteryLevel -= BATTERY_DRAIN_RATE * deltaTime;
      batteryManager->setBatteryLevel(max(0, batteryLevel));
      digitalWrite(HEATING_PIN, HIGH);  // Turn on heating
    } else {
      digitalWrite(HEATING_PIN, LOW);   // Turn off heating
    }
    
    // Natural cooling always happens regardless of battery
    double tempDiffAmbient = AMBIENT_TEMP - currentTemperature;
    currentTemperature += tempDiffAmbient * AMBIENT_INFLUENCE * deltaTime;
    
    // Update temperature reading (round to 2 decimal places)
    heatingManager->setTemperature(round(currentTemperature * 100.0) / 100.0);
    
    // Log system status
    Serial.println("System Status:");
    Serial.println("Ambient Temp: " + String(AMBIENT_TEMP, 2) + "°C");
    Serial.println("Current Temp: " + String(currentTemperature, 2) + "°C");
    Serial.println("Target Temp: " + String(targetTemp) + "°C");
    Serial.println("Temp Diff: " + String(tempDiff, 2) + "°C");
    Serial.println("Heating: " + heatingManager->getHeatingStatus());
    Serial.println("Battery: " + String(batteryLevel) + "%");
    Serial.println("------------------------");
  }
  
  delay(10);  // Reduced delay for smoother simulation
}
