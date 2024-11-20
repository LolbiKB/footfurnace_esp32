#include "BatteryManager.h"

BatteryManager::BatteryManager(BLEService *service, BLEServer *server) : pServer(server) {
  batteryCharacteristic = service->createCharacteristic(
      "1d61b289-e2f0-4af4-99e5-6de4370c8083",
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
}

void BatteryManager::updateBatteryData(const JsonObject &newData) {
  for (JsonPair pair : newData) {
    batteryDoc[pair.key()] = pair.value();
  }

  char jsonBuffer[256];
  serializeJson(batteryDoc, jsonBuffer);
  batteryCharacteristic->setValue(jsonBuffer);

  if (pServer->getConnectedCount() > 0) {
    batteryCharacteristic->notify();
  }
}
