#ifndef hall_h
#define hall_h

#include "mbed.h"

#ifndef HALL_CALIBRATION
#define HALL_CALIBRATION
#define calibration_k   0.12
#define calibration_y0  1.58
#endif

class hallSens : public AnalogIn{
public:
    hallSens(PinName pin, float offset = calibration_y0);
    float read();
    float current();
private:
    float offset;
};

float fround(float val);

#endif