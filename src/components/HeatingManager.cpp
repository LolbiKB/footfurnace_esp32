#include "HeatingManager.h"

HeatingManager::HeatingManager(BLEService *service, BLEServer *server) : pServer(server) {
  heatingCharacteristic = service->createCharacteristic(
      "4664c97b-ecc4-40c3-81a2-4789f8ed5e1c",
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
}

void HeatingManager::updateHeatingData(const JsonObject &newData) {
  for (JsonPair pair : newData) {
    heatingDoc[pair.key()] = pair.value();
  }
  notifyCharacteristic();
}

void HeatingManager::setTemperature(int temperature) {
  heatingDoc["temperature"] = temperature;
  notifyCharacteristic();
}

void HeatingManager::setHeatingStatus(const char *status) {
  heatingDoc["heatingStatus"] = status;
  notifyCharacteristic();
}

void HeatingManager::notifyCharacteristic() {
  char jsonBuffer[256];
  serializeJson(heatingDoc, jsonBuffer);
  heatingCharacteristic->setValue(jsonBuffer);

  if (pServer->getConnectedCount() > 0) {
    heatingCharacteristic->notify();
  }
}
