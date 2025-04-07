#include <Arduino.h>
#include "components/BatteryManager.h"
#include "components/HeatingManager.h"
#include "components/PowerManager.h"
#include "utils/BLEUtils.h"
#include "components/Battery.h"
#include "components/Temperature.h"
class ServerCallbacks: public BLEServerCallbacks {
    void onDisconnect(BLEServer* pServer) {
        Serial.println("Client disconnected");
        pServer->getAdvertising()->start();
    }
};

// Declare global pointers to managers and components
BatteryManager *batteryManager;
HeatingManager *heatingManager;
PowerManager *powerManager;
Battery *battery;
Temperature *temperature;

// Hardware configuration
const double MAINTENANCE_THRESHOLD = 1.0; // ±1°C threshold for maintenance mode
const unsigned long UPDATE_INTERVAL = 1000;
const int HEATING_PIN = GPIO_NUM_15;

// ADC configuration
const int THERMISTOR_PIN = GPIO_NUM_4;  // ADC pin for thermistor
const int BATTERY_PIN = GPIO_NUM_32;    // ADC pin for battery voltage

// LED configuration
const int LED_BLE_PIN = GPIO_NUM_26;
const int LED_BATTERY_FULL_PIN = GPIO_NUM_27;
const int LED_BATTERY_LOW_PIN = GPIO_NUM_12;
const int LED_HEATING_PIN = GPIO_NUM_25;
const int LED_PIN_GROUND = GPIO_NUM_13;
unsigned long previousLEDMillis = 0;
const int LED_FLASH_INTERVAL = 500;
bool ledState = false;
unsigned long previousHeatingLEDMillis = 0; // Add timer for heating LED flashing
bool heatingLedState = false; // State for heating LED when flashing

// BLE connection tracking
const unsigned long BLE_CONNECTED_LED_DURATION = 5000;
unsigned long bleConnectedTime = 0;
bool wasDisconnected = true;

unsigned long previousMillis = 0;
unsigned long statusDisplayMillis = 0;
const unsigned long STATUS_DISPLAY_INTERVAL = 1000; // Update display every second

// Sensor parameters
const float BATTERY_VOLTAGE_MAX = 8.4;
const float BATTERY_VOLTAGE_MIN = 6.0;
const float BATTERY_VOLTAGE_DIVIDER = 3.921;

const float THERMISTOR_R_NOMINAL = 10000.0;
const float THERMISTOR_B_COEFFICIENT = 3950.0;
const float THERMISTOR_SERIES_RESISTOR = 50000.0;
const float REFERENCE_TEMP = 25.0;

void setup() {
  Serial.begin(115200);
  
  // ADC configuration
  analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
  
  // Initialize Battery and Temperature components with direct GPIO pins
  battery = new Battery(BATTERY_PIN, BATTERY_VOLTAGE_DIVIDER,
                      BATTERY_VOLTAGE_MAX, BATTERY_VOLTAGE_MIN);
                      
  // Create lambda function to get battery voltage
  auto getBatteryVoltage = []() -> float {
    return battery->readVoltage();
  };

  // Then create temperature component with the callback
  temperature = new Temperature(THERMISTOR_PIN,
                             THERMISTOR_R_NOMINAL, THERMISTOR_B_COEFFICIENT,
                             THERMISTOR_SERIES_RESISTOR, REFERENCE_TEMP,
                             getBatteryVoltage);

  // Get initial readings
  float battVoltage = battery->readVoltage();
  int battPercent = battery->calculatePercentage();
  float temp = temperature->readTemperature();

  // Initialize BLE
  BLEDevice::init("BootsESP32");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  BLEService *pService = pServer->createService("12345678-90AB-CDEF-1234-567890ABCDEF");

  // Initialize managers with actual sensor values
  batteryManager = new BatteryManager(pService, pServer);
  heatingManager = new HeatingManager(pService, pServer);
  powerManager = new PowerManager(pService, pServer);

  batteryManager->setBatteryLevel(battPercent);
  batteryManager->setChargingStatus(false);
  batteryManager->setBatteryHealth(80);

  heatingManager->setHeatingStatus("OFF");
  heatingManager->setTemperature(temp);

  powerManager->setPowerStatus("ON");
  powerManager->setLastPoweredOn("2023-11-20T10:00:00Z");

  // Start BLE advertising
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("12345678-90AB-CDEF-1234-567890ABCDEF");
  BLEDevice::startAdvertising();

  // Initialize pins
  pinMode(HEATING_PIN, OUTPUT);
  pinMode(LED_BLE_PIN, OUTPUT);
  pinMode(LED_BATTERY_LOW_PIN, OUTPUT);
  pinMode(LED_BATTERY_FULL_PIN, OUTPUT); // Initialize battery full LED
  pinMode(LED_HEATING_PIN, OUTPUT); // Initialize heating LED
  pinMode(LED_PIN_GROUND, OUTPUT);

  digitalWrite(HEATING_PIN, LOW);
  digitalWrite(LED_BLE_PIN, LOW);
  digitalWrite(LED_BATTERY_LOW_PIN, LOW);
  digitalWrite(LED_BATTERY_FULL_PIN, LOW); // Initialize battery full LED to off
  digitalWrite(LED_HEATING_PIN, LOW); // Initialize heating LED to off
  digitalWrite(LED_PIN_GROUND, LOW);

  Serial.println("System Initialized");
}

void loop() {
  unsigned long currentMillis = millis();

  // Periodic sensor reading and control updates
  if (currentMillis - previousMillis >= UPDATE_INTERVAL) {
    previousMillis = currentMillis;

    // Simplified LED control - only handle BLE LED flashing
    if (!heatingManager->getServer()->getConnectedCount()) {
      // No BLE connection - flash BLE LED
      if (currentMillis - previousLEDMillis >= LED_FLASH_INTERVAL) {
        previousLEDMillis = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_BLE_PIN, ledState);
      }
      wasDisconnected = true;
    } else {
      // BLE connected - LED solid for 5 seconds, then off
      if (wasDisconnected) {
        bleConnectedTime = currentMillis;
        wasDisconnected = false;
      }
      
      digitalWrite(LED_BLE_PIN, (currentMillis - bleConnectedTime < BLE_CONNECTED_LED_DURATION) ? HIGH : LOW);
    }

    // Turn off other LEDs for now
    digitalWrite(LED_BATTERY_LOW_PIN, LOW);
    digitalWrite(LED_BATTERY_FULL_PIN, LOW);

    // Read sensor values
    float currentTemp = temperature->readTemperature();
    int batteryPercent = battery->calculatePercentage();
    float batteryVoltage = battery->readVoltage();

    // Update BLE characteristics
    batteryManager->setBatteryLevel(batteryPercent);
    heatingManager->setTemperature(round(currentTemp * 100.0) / 100.0);

    double targetTemp = heatingManager->getTargetTemperature();
    double tempDiff = abs(currentTemp - targetTemp);

    // Update heating status with LED control
    if (tempDiff <= MAINTENANCE_THRESHOLD) {
      heatingManager->setHeatingStatus("MTN");
      // Flash the heating LED in maintenance mode
      if (currentMillis - previousHeatingLEDMillis >= LED_FLASH_INTERVAL) {
        previousHeatingLEDMillis = currentMillis;
        heatingLedState = !heatingLedState;
        digitalWrite(LED_HEATING_PIN, heatingLedState);
      }
    } else if (currentTemp < targetTemp) {
      heatingManager->setHeatingStatus("ON");
      digitalWrite(HEATING_PIN, HIGH);
      digitalWrite(LED_HEATING_PIN, HIGH);
    } else {
      heatingManager->setHeatingStatus("OFF");
      digitalWrite(HEATING_PIN, LOW);
      digitalWrite(LED_HEATING_PIN, LOW);
    }

    // Log system status
    if (currentMillis - statusDisplayMillis >= STATUS_DISPLAY_INTERVAL) {
      statusDisplayMillis = currentMillis;
      
      // Clear screen and reset cursor position
      Serial.print("\033[2J\033[H");
      
      // System title
      Serial.println("SYSTEM STATUS");
      Serial.println("-------------");
      
      // Temperature section
      Serial.println("\nTEMPERATURE");
      Serial.println("Current: " + String(currentTemp, 1) + "°C / " + String((currentTemp * 9/5) + 32, 1) + "°F");
      Serial.println("Resistance: " + String(temperature->readResistance(), 2) + " Ohms");
      Serial.println("Target:  " + String(targetTemp, 1) + "°C / " + String((targetTemp * 9/5) + 32, 1) + "°F");
      Serial.println("Raw ADC: " + String(temperature->readRawValue()));
      
      // Battery section
      Serial.println("\nBATTERY");
      Serial.println("Level:   " + String(batteryPercent) + "% (" + String(batteryVoltage, 2) + "V)");
      Serial.println("Raw ADC: " + String(battery->readRawValue()));
      
      // System status section
      Serial.println("\nSYSTEM");
      Serial.println("Heating: " + heatingManager->getHeatingStatus());
      Serial.println("BLE:     " + String(heatingManager->getServer()->getConnectedCount() > 0 ? "Connected" : "Disconnected"));
      
      // Uptime
      Serial.println("\nUptime: " + String(millis() / 1000) + " seconds");
    }
  }

  delay(10);
}
