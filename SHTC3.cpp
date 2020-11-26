#include "SHTC3.h"
#include <iostream>

SHTC::SHTC(I2C *boardI2C){
    sensI2C = boardI2C;
}

status_t SHTC::wake(){
    char wkBuffer[2];
    wkBuffer[0] = wakeMSB;
    wkBuffer[1] = wakeLSB;

    for(uint8_t i = 0; i < 3; i++){
        if(this -> sensI2C -> write(shtcADDR, &wkBuffer[0], 2, false)) {
            return STATUS_TX_ERROR;
        }
    }

    return STATUS_OK;
}

status_t SHTC::sleep(){
    char slpBuffer[2];
    slpBuffer[0] = slpMSB;
    slpBuffer[1] = slpLSB;

    if(this -> sensI2C -> write(shtcADDR, &slpBuffer[0], 2, false)){
        return STATUS_TX_ERROR;
    }

    return STATUS_OK;
}

status_t SHTC::init(){
    char buffer[3];
    buffer[0] = idMSB;
    buffer[1] = idLSB;

    status_t status = this -> wake();

    if(status){
        return status;
    };

    if(this -> sensI2C -> write(shtcADDR, &buffer[0], 2, false)){
        return STATUS_TX_ERROR;
    }

    if(this -> sensI2C -> read(shtcADDR, &buffer[0], 3, false)){
        return STATUS_RX_ERROR;
    }   

    buffer[2] = NULL;

    this -> id = (*(int*)&buffer)&idMask;

    status = this -> sleep();

    if(status){
        return status;
    };

    return STATUS_OK;
}

status_t SHTC::read(){
    shtc_data_buffer_t buffer;

    status_t status = this -> wake();

    if(status){
        return status;
    };

    buffer.data[0] = readMSB;
    buffer.data[1] = readLSB;

    if(this -> sensI2C -> write(shtcADDR, &buffer.data[0], 2, false)){
        return STATUS_TX_ERROR;
    }

    if(this -> sensI2C -> read(shtcADDR, &buffer.data[0], 6, false)){
        return STATUS_RX_ERROR;
    }

    char intBuffer[2];
    for (uint8_t i = 0; i < 2; i++){
        intBuffer[i] = buffer.temp[1 - i];
    }

    this -> T = (((*(int*)&intBuffer) * 175.0) / 65536.0) - 45.0;

    for (uint8_t i = 0; i < 2; i++){
        intBuffer[i] = buffer.hum[1 - i];
    }

    this -> H = ((*(int*)&intBuffer) * 100.0) / 65536.0;

    status = this -> sleep();

    if(status){
        return status;
    };

    return STATUS_OK;
}

status_t SHTC::reset(){
    char buffer[2];
    buffer[0] = rstMSB;
    buffer[1] = rstLSB;

    if(this -> sensI2C -> write(shtcADDR, &buffer[0], 2, false)){
        return STATUS_TX_ERROR;
    }

    return STATUS_OK;
}

float SHTC::lastTemp(){
    return this -> T;
}

float SHTC::lastHum(){
    return this -> H;
}

int SHTC::sensID(){
    return this -> id;
}