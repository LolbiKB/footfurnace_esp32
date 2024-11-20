#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <ArduinoJson.h>

class BatteryManager {
public:
  BatteryManager(BLEService *service, BLEServer *server);
  void updateBatteryData(const JsonObject &newData);

private:
  BLECharacteristic *batteryCharacteristic;
  BLEServer *pServer;
  StaticJsonDocument<256> batteryDoc;
};

#endif // BATTERY_MANAGER_H
