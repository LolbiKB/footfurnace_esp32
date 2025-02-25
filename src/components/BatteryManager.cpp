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
  notifyCharacteristic();
}

void BatteryManager::setBatteryLevel(int level) {
  batteryDoc["batteryLevel"] = level;
  notifyCharacteristic();
}

void BatteryManager::setChargingStatus(bool status) {
  batteryDoc["chargingStatus"] = status;
  notifyCharacteristic();
}

void BatteryManager::setBatteryHealth(int health) {
  batteryDoc["batteryHealth"] = health;
  notifyCharacteristic();
}

int BatteryManager::getBatteryLevel() const {
    return batteryDoc["batteryLevel"] | 0;
}

void BatteryManager::notifyCharacteristic() {
  char jsonBuffer[256];
  serializeJson(batteryDoc, jsonBuffer);
  batteryCharacteristic->setValue(jsonBuffer);

  if (pServer->getConnectedCount() > 0) {
    batteryCharacteristic->notify();
  }
}
