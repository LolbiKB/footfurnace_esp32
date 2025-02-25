#include "HeatingManager.h"

HeatingManager::HeatingManager(BLEService* service, BLEServer* server) 
  : pServer(server), targetTemperature(25) {
  
  heatingCharacteristic = service->createCharacteristic(
    "4664c97b-ecc4-40c3-81a2-4789f8ed5e1c",
    BLECharacteristic::PROPERTY_READ | 
    BLECharacteristic::PROPERTY_WRITE | 
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  heatingCharacteristic->setCallbacks(new HeatingCallbacks(this));
  heatingDoc["targetTemperature"] = targetTemperature;
  heatingDoc["temperature"] = 25;
  heatingDoc["heatingStatus"] = "OFF";
}

void HeatingManager::HeatingCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
  std::string value = pCharacteristic->getValue();
  
  if (value.length() > 0) {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, value);
    
    if (!error && doc.containsKey("targetTemperature")) {
      int newTemp = doc["targetTemperature"].as<int>();
      // Ensure temperature is within safe range
      if (newTemp >= 0 && newTemp <= 30) {
        manager->setTargetTemperature(newTemp);
      }
    }
  }
}

void HeatingManager::updateHeatingData(const JsonObject &newData) {
  for (JsonPair pair : newData) {
    heatingDoc[pair.key()] = pair.value();
  }
  notifyCharacteristic();
}

void HeatingManager::setTemperature(double temperature) {
  heatingDoc["temperature"] = temperature;
  notifyCharacteristic();
}

void HeatingManager::setHeatingStatus(const char *status) {
  heatingDoc["heatingStatus"] = status;
  notifyCharacteristic();
}

void HeatingManager::setTargetTemperature(double temperature) {
  targetTemperature = temperature;
  heatingDoc["targetTemperature"] = temperature;
  
  // Added heating control logic
  int currentTemp = heatingDoc["temperature"] | 25;
  if (currentTemp < targetTemperature) {
      setHeatingStatus("ON");
  } else {
      setHeatingStatus("OFF");
  }
  
  notifyCharacteristic();  // Send update back to app
}

double HeatingManager::getTargetTemperature() const {
  return targetTemperature;
}

void HeatingManager::notifyCharacteristic() {
  char jsonBuffer[256];
  serializeJson(heatingDoc, jsonBuffer);
  heatingCharacteristic->setValue(jsonBuffer);

  if (pServer->getConnectedCount() > 0) {
    heatingCharacteristic->notify();
  }
}

// Add this new method implementation
String HeatingManager::getHeatingStatus() const {
    return heatingDoc["heatingStatus"] | "OFF";
}
