#include "hall.h"

hallSens::hallSens(PinName pin, float offset) : AnalogIn(pin){
    this -> offset = offset;
}

float hallSens::read(){
    //return AnalogIn::read(); //direct pin ADC read debug

    float buffer = 0;
    for(uint8_t i = 0; i < 10; i++){
            buffer += 3.3 * AnalogIn::read();
        }

    //return fround(buffer / 10); //ADC read debug after averaging

    buffer = (fround(buffer / 10) - offset) / calibration_k;
    if((buffer <= .1)&&(buffer >= -0.1)){
        return 0;
    }
    //return fround(buffer / 10);
    return buffer;
}

float fround(float val){
    val = (int)(val*100 + .5);
    return (float)val / 100;
}