#include "Battery.h"
#include <Arduino.h>

Battery::Battery(int adcPin, float dividerRatio, float vMax, float vMin) {
  this->adcPin = adcPin;
  this->dividerRatio = dividerRatio;
  voltageMax = vMax;
  voltageMin = vMin;
  
  // Initialize buffer
  bufferIndex = 0;
  validReadings = 0;
  for (int i = 0; i < VOLTAGE_BUFFER_SIZE; i++) {
    voltageBuffer[i] = 0.0;
  }
}

int Battery::readRawValue() {
  return analogRead(adcPin);
}

float Battery::readVoltage() {
  int raw = readRawValue();
  
  // Convert raw ADC value to voltage and apply divider ratio
  float currentVoltage = (raw / 4095.0 * 1.0263) * 3.3 * dividerRatio + .53;
  
  // Update buffer with new reading
  voltageBuffer[bufferIndex] = currentVoltage;
  
  // Update buffer index and valid reading count
  bufferIndex = (bufferIndex + 1) % VOLTAGE_BUFFER_SIZE;
  if (validReadings < VOLTAGE_BUFFER_SIZE) {
    validReadings++;
  }
  
  // Return average voltage
  return calculateAverageVoltage();
}

float Battery::calculateAverageVoltage() {
  float sum = 0.0;
  int count = 0;
  
  // Sum all non-zero values in the buffer
  for (int i = 0; i < VOLTAGE_BUFFER_SIZE; i++) {
    if (voltageBuffer[i] > 0.0 && voltageBuffer[i] < 10.0) {
      sum += voltageBuffer[i];
      count++;
    }
  }
  
  // Return average, avoiding division by zero
  return (count > 0) ? (sum / count) : 0.0;
}

int Battery::calculatePercentage() {
  float voltage = readVoltage();
  int percentage = ((voltage - voltageMin) / (voltageMax - voltageMin)) * 100;
  return constrain(percentage, 0, 100);
}

bool Battery::isLow() {
  return calculatePercentage() < 30;
}

bool Battery::isDead() {
  return calculatePercentage() <= 0;
}