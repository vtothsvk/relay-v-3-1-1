#include "PAC.h"

PAC::PAC(I2C *boardI2C, uint8_t ADDR, float shunt){
    this -> pacI2C = boardI2C;
    this -> shunt = shunt;
    this -> pacAddr = ADDR << 1;
}

status_t PAC::init(){
    char message[2];
    
    message[0] = pacConfigReg;
    message[1] = pacConfiguration;

    if(this -> pacI2C -> write(this -> pacAddr, &message[0], 2, false)){
        return STATUS_TX_ERROR;
    }

    wait_ms(100);
    message[0] = pacConverionReg;
    message[1] = pacConversion;

    if(this -> pacI2C -> write(this -> pacAddr, &message[0], 2, false)){
        return STATUS_TX_ERROR;
    }

    wait_ms(100);
    message[0] = pacSourceSamplingReg;
    message[1] = pacSourceSampling;

    if(this -> pacI2C -> write(this -> pacAddr, &message[0], 2, false)){
        return STATUS_TX_ERROR;
    }

    wait_ms(100);
    message[0] = pacSenseSamplingReg;
    message[1] = pacSenseSampling;

    if(this -> pacI2C -> write(this -> pacAddr, &message[0], 2, false)){
        return STATUS_TX_ERROR;
    }

    wait_ms(10);
    return STATUS_OK;
}

status_t PAC::getI(){
        char buffer[2];
        int Vsense;
        buffer[0] = sensReg;
        buffer[1] = sensReg + 1;

        if(this -> pacI2C -> write(this -> pacAddr, &buffer[0], 1, false)){
            return STATUS_TX_ERROR;
        }

        if(this -> pacI2C -> read(this -> pacAddr, &buffer[0], 1, false)){
            return STATUS_RX_ERROR;
        }

        if(this -> pacI2C -> write(this -> pacAddr, &buffer[1], 1, false)){
            return STATUS_TX_ERROR;
        }

        if(this -> pacI2C -> read(this -> pacAddr, &buffer[1], 1, false)){
            return STATUS_RX_ERROR;
        }

        Vsense = buffer[1] | (buffer[0] << 8);
        Vsense = Vsense >> 4;
        this -> i = (0.08 / this -> shunt) * (Vsense / 2047.0);
        float test = 8.00195;
        if(this -> i > 7.9){
            this -> i = 0;
        }
        wait_ms(10);
        return STATUS_OK;
}

status_t PAC::getV(){
    char buffer[2];
    float v;
    int Vsource;
    buffer[0] = sourceReg;
    buffer[1] = sourceReg + 1;

    if(this -> pacI2C -> write(this -> pacAddr, &buffer[0], 1, false)){
        return STATUS_TX_ERROR;
    }

    if(this -> pacI2C -> read(this -> pacAddr, &buffer[0], 1, false)){
        return STATUS_RX_ERROR;
    }

    if(this -> pacI2C -> write(this -> pacAddr, &buffer[1], 1, false)){
        return STATUS_TX_ERROR;
    }

    if(this -> pacI2C -> read(this -> pacAddr, &buffer[1], 1, false)){
        return STATUS_RX_ERROR;
    }

    Vsource = buffer[1] | (buffer[0] << 8);
    Vsource = Vsource >> 5;
    v = (40.0 - (40.0 / 2048.0)) * (Vsource / 2047.0);  //meni sa hodnot Rshunt
    if (v < 0) {
        v = v + 40.0;
    }
    this -> v = v;
    wait_ms(10);
    return STATUS_OK;
}

status_t PAC::getP(){
    char buffer[2];
    int P;
    buffer[0] = powerReg;
    buffer[1] = powerReg + 1;

    if(this -> pacI2C -> write(this -> pacAddr, &buffer[0], 1, false)) {
        return STATUS_TX_ERROR;
    }

    if(this -> pacI2C -> read(this -> pacAddr, &buffer[0], 1, false)){
        return STATUS_RX_ERROR;
    }

    if(this -> pacI2C -> write(this -> pacAddr, &buffer[1], 1, false)){
        return STATUS_TX_ERROR;
    }

    if(this -> pacI2C -> read(this -> pacAddr, &buffer[1], 1, false)){
        return STATUS_RX_ERROR;
    }

   P = buffer[1] | (buffer[0] << 8);
  //Vsource = Vsource >> 5;
  this -> p = ((40.0 - (40.0 / 2048.0)) * (0.08 / this -> shunt)) * (P / 65535.0);
  return STATUS_OK;
}

float PAC::voltage(){
    return this -> v;
}

float PAC::current(){
    return this -> i;
}

float PAC::power(){
    return this -> p;
}

