  #ifndef THERMISTOR_H
#define THERMISTOR_H

#include <math.h>

class Thermistor {
public:
    // Constructor
    Thermistor(float beta, float r0, float t0, float rPullup, float vRef, int adcResolution);

    // Function to calculate resistance from ADC value
    float adcToResistance(int adcValue) const;

    // Function to calculate temperature from resistance
    float resistanceToTemperature(float resistance) const;

private:
    float beta;           // Beta value of the thermistor
    float r0;             // Resistance at T0
    float t0;             // Reference temperature in Kelvin
    float rPullup;        // Pull-up resistor value
    float vRef;           // Reference voltage
    int adcResolution;    // ADC resolution
};

#endif // THERMISTOR_H
