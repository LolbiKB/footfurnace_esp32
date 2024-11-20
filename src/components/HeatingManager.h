#ifndef HEATING_MANAGER_H
#define HEATING_MANAGER_H

#include <Arduino.h>
#include <BLEServer.h>
#include <BLECharacteristic.h>
#include <ArduinoJson.h>

class HeatingManager {
public:
  HeatingManager(BLEService *service, BLEServer *server);
  void updateHeatingData(const JsonObject &newData);

private:
  BLECharacteristic *heatingCharacteristic;
  BLEServer *pServer;
  StaticJsonDocument<256> heatingDoc;
};

#endif // HEATING_MANAGER_H
