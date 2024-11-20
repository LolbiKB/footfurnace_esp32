#include "Thermistor.h"

// Constructor
Thermistor::Thermistor(float beta, float r0, float t0, float rPullup, float vRef, int adcResolution)
    : beta(beta), r0(r0), t0(t0), rPullup(rPullup), vRef(vRef), adcResolution(adcResolution) {}

// Function to calculate temperature from resistance
float Thermistor::resistanceToTemperature(float resistance) const {
    float tempK = 1.0 / (1.0 / t0 + log(resistance / r0) / beta); // Temperature in Kelvin
    return tempK - 273.15; // Convert Kelvin to Celsius
}
