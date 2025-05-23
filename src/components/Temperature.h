#ifndef TEMPERATURE_H
#define TEMPERATURE_H

// Define a function pointer type for battery voltage callback
typedef float (*BatteryVoltageCallback)();

class Temperature {
private:
  int adcPin;
  float rNominal;
  float bCoefficient;
  float seriesResistor;
  float referenceTemp;
  
public:
  Temperature(int adcPin, float nominal, float beta, float series, float refTemp);
  int readRawValue();
  float readVoltage();
  float readResistance();
  float readTemperature();
};

#endif