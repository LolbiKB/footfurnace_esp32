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

  // Helper methods for specific updates
  void setPowerStatus(const char *status);
  void setLastPoweredOn(const char *timestamp);

private:
  BLECharacteristic *powerCharacteristic;
  BLEServer *pServer;
  StaticJsonDocument<256> powerDoc;

  void notifyCharacteristic(); // Encapsulates notification logic
};

#endif // POWER_MANAGER_H
