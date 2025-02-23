#include "PowerManager.h"

PowerManager::PowerManager(BLEService *service, BLEServer *server) : pServer(server) {
  powerCharacteristic = service->createCharacteristic(
      "923202f1-68ce-42c8-bf28-df8a38f37d86",
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
}

void PowerManager::updatePowerData(const JsonObject &newData) {
  for (JsonPair pair : newData) {
    powerDoc[pair.key()] = pair.value();
  }
  notifyCharacteristic();
}

void PowerManager::setPowerStatus(const char *status) {
  powerDoc["powerStatus"] = status;
  notifyCharacteristic();
}

void PowerManager::setLastPoweredOn(const char *timestamp) {
  powerDoc["lastPoweredOn"] = timestamp;
  notifyCharacteristic();
}

void PowerManager::notifyCharacteristic() {
  char jsonBuffer[256];
  serializeJson(powerDoc, jsonBuffer);
  powerCharacteristic->setValue(jsonBuffer);

  if (pServer->getConnectedCount() > 0) {
    powerCharacteristic->notify();
  }
}
