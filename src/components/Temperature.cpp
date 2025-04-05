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
  // Convert raw ADC value to voltage
  return (raw / 4095.0) * 3.3 + 0.115; // Assuming 12-bit ADC and 3.3V reference
}

float Temperature::readTemperature() {
  float voltage = readVoltage();
  
  // Calculate resistance of thermistor
  float resistance = seriesResistor / (3.3 / voltage - 1.0);
  
  // Use Steinhart-Hart equation to calculate temperature
  float steinhart = resistance / rNominal;  // (R/Ro)
  steinhart = log(steinhart);              // ln(R/Ro)
  steinhart /= bCoefficient;               // 1/B * ln(R/Ro)
  steinhart += 1.0 / (referenceTemp + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;            // Invert
  steinhart -= 273.15;                    // Convert to Celsius
  
  return steinhart + 21.1; // Adjust for offset
}