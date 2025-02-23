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

  // Helper methods for specific updates
  void setBatteryLevel(int level);
  void setChargingStatus(bool status);
  void setBatteryHealth(int health);

private:
  BLECharacteristic *batteryCharacteristic;
  BLEServer *pServer;
  StaticJsonDocument<256> batteryDoc;

  void notifyCharacteristic(); // Encapsulates notification logic
};

#endif // BATTERY_MANAGER_H
