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
                      
  temperature = new Temperature(THERMISTOR_PIN,
                             THERMISTOR_R_NOMINAL, THERMISTOR_B_COEFFICIENT,
                             THERMISTOR_SERIES_RESISTOR, REFERENCE_TEMP);

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

      // LED handling - prioritize battery status
    if (battery->isDead()) {
      digitalWrite(LED_BLE_PIN, LOW);
      digitalWrite(LED_BATTERY_LOW_PIN, LOW);
      digitalWrite(LED_BATTERY_FULL_PIN, LOW); // Turn off battery full LED when dead
    } else if (!heatingManager->getServer()->getConnectedCount()) {
      digitalWrite(LED_BATTERY_LOW_PIN, LOW);
      wasDisconnected = true;

      if (currentMillis - previousLEDMillis >= LED_FLASH_INTERVAL) {
        previousLEDMillis = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_BLE_PIN, ledState);
      }
      digitalWrite(LED_BATTERY_FULL_PIN, LOW); // Keep full LED off during BLE advertising
    } else {
      // BLE connected
      if (wasDisconnected) {
        bleConnectedTime = currentMillis;
        wasDisconnected = false;
      }

      if (currentMillis - bleConnectedTime < BLE_CONNECTED_LED_DURATION) {
        digitalWrite(LED_BLE_PIN, HIGH);
      } else {
        digitalWrite(LED_BLE_PIN, LOW);

        if (battery->isLow()) {
          digitalWrite(LED_BATTERY_LOW_PIN, HIGH);
          digitalWrite(LED_BATTERY_FULL_PIN, LOW); // Turn off full LED when battery is low
        } else {
          digitalWrite(LED_BATTERY_LOW_PIN, LOW);
          
          // Turn on battery full LED when battery is at 100%
          // digitalWrite(LED_BATTERY_FULL_PIN, batteryPercent >= 100 ? HIGH : LOW);
        }
      }
    }

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
    if (batteryPercent <= 0) {
      heatingManager->setHeatingStatus("OFF");
      digitalWrite(HEATING_PIN, LOW);  // Ensure heating is off when battery is dead
      digitalWrite(LED_HEATING_PIN, LOW); // Turn off heating LED when battery is dead
    } else if (tempDiff <= MAINTENANCE_THRESHOLD) {
      heatingManager->setHeatingStatus("MTN");
      // Flash the heating LED in maintenance mode
      if (currentMillis - previousHeatingLEDMillis >= LED_FLASH_INTERVAL) {
        previousHeatingLEDMillis = currentMillis;
        heatingLedState = !heatingLedState;
      }
    } else if (currentTemp < targetTemp) {
      heatingManager->setHeatingStatus("ON");
      digitalWrite(HEATING_PIN, HIGH);  // Turn heating on
      digitalWrite(LED_HEATING_PIN, HIGH); // Turn heating LED on
    } else {
      heatingManager->setHeatingStatus("OFF");
      digitalWrite(HEATING_PIN, LOW);  // Turn heating off
      digitalWrite(LED_HEATING_PIN, LOW); // Turn heating LED off
    }

    digitalWrite(HEATING_PIN, HIGH);  // Alway turn on heating pin for testing

    // Log system status using carriage returns to update in place
    if (currentMillis - statusDisplayMillis >= STATUS_DISPLAY_INTERVAL) {
      statusDisplayMillis = currentMillis;
      
      // Use carriage returns to update the same lines
      Serial.print("\r\033[K"); // Carriage return and clear line
      Serial.println("SYSTEM STATUS:");
      Serial.print("\033[K"); // Clear line
      Serial.println("Temperature: " + String(currentTemp, 1) + "°C / " + String((currentTemp * 9/5) + 32, 1) + "°F");
      Serial.print("\033[K"); // Clear line
      Serial.println("Target Temp: " + String(targetTemp, 1) + "°C / " + String((targetTemp * 9/5) + 32, 1) + "°F");
      Serial.print("\033[K"); // Clear line
      Serial.println("Raw Temp Voltage: " + String(temperature->readVoltage()));
      Serial.print("\033[K"); // Clear line
      Serial.println("Battery: " + String(batteryPercent) + "% (" + String(batteryVoltage, 2) + "V)");
      Serial.print("\033[K"); // Clear line
      Serial.println("Battery Raw Voltage: " + String(battery->readRawValue()));
      Serial.print("\033[K"); // Clear line
      Serial.println("Heating: " + heatingManager->getHeatingStatus());
      Serial.print("\033[K"); // Clear line
      Serial.println("BLE Connected: " + String(heatingManager->getServer()->getConnectedCount() > 0 ? "YES" : "NO"));
      Serial.print("\033[K"); // Clear line
      
      // Move cursor back up 6 lines to overwrite the same area next time
      Serial.print("\033[6A");
    }
  }

  delay(10);
}
