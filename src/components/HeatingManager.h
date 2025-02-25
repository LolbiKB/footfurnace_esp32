#ifndef HEATING_MANAGER_H
#define HEATING_MANAGER_H

#include <BLEServer.h>
#include <BLEService.h>
#include <ArduinoJson.h>

class HeatingManager {
private:
    BLECharacteristic* heatingCharacteristic;
    BLEServer* pServer;
    StaticJsonDocument<256> heatingDoc;
    double targetTemperature;  // Changed from int to double
    void notifyCharacteristic();

    class HeatingCallbacks : public BLECharacteristicCallbacks {
    private:
        HeatingManager* manager;
    public:
        HeatingCallbacks(HeatingManager* mgr) : manager(mgr) {}
        void onWrite(BLECharacteristic* pCharacteristic) override;
    };

public:
    HeatingManager(BLEService* service, BLEServer* server);
    void updateHeatingData(const JsonObject& newData);
    void setTemperature(double temperature);  // Changed from int to double
    void setHeatingStatus(const char* status);
    void setTargetTemperature(double temperature);  // Changed from int to double
    double getTargetTemperature() const;  // Changed from int to double
    String getHeatingStatus() const;
    BLEServer* getServer() { return pServer; }
};

#endif