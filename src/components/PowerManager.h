#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <ArduinoJson.h>

class PowerManager {
public:
  PowerManager(BLEService *service, BLEServer *server);
  void updatePowerData(const JsonObject &newData);

private:
  BLECharacteristic *powerCharacteristic;
  BLEServer *pServer;
  StaticJsonDocument<256> powerDoc;
};

#endif // POWER_MANAGER_H
