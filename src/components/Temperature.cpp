#include "Temperature.h"
#include <Arduino.h>
#include <math.h>

Temperature::Temperature(int adcPin, float nominal, float beta, float series, float refTemp) {
  this->adcPin = adcPin;
  rNominal = nominal;
  bCoefficient = beta;
  seriesResistor = series;
  referenceTemp = refTemp;
}

int Temperature::readRawValue() {
  return analogRead(adcPin);
}

float Temperature::readVoltage() {
  int raw = readRawValue();
  
  float voltage = (raw / 1023.0) * 3.3;
  return voltage;
}

float Temperature::readResistance() {
  float voltage = readVoltage();

  float resistance = (voltage * seriesResistor) / (3.3 - voltage);
  return resistance;
}

float Temperature::readTemperature() {
  float resistance = readResistance();

  // Use Steinhart-Hart equation to calculate temperature
  float steinhart = resistance / rNominal;  // (R/Ro)
  steinhart = log(steinhart);              // ln(R/Ro)
  steinhart /= bCoefficient;               // 1/B * ln(R/Ro)
  steinhart += 1.0 / (referenceTemp + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;            // Invert
  steinhart -= 273.15;                    // Convert to Celsius
  
  return steinhart + 21.1; // Adjust for offset
}