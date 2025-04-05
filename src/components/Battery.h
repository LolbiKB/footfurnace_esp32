#ifndef BATTERY_H
#define BATTERY_H

#define VOLTAGE_BUFFER_SIZE 10

class Battery {
private:
  int adcPin;
  float dividerRatio;
  float voltageMax;
  float voltageMin;
  
  // Buffer for voltage readings
  float voltageBuffer[VOLTAGE_BUFFER_SIZE];
  int bufferIndex;
  int validReadings;
  
  // Helper method to calculate average voltage from buffer
  float calculateAverageVoltage();
  
public:
  Battery(int adcPin, float dividerRatio, float vMax, float vMin);
  int readRawValue();
  float readVoltage();
  int calculatePercentage();
  bool isLow();
  bool isDead();
};

#endif
