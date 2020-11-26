#ifndef PAC_h
#define PAC_h

#define PAC_DEFAULT_ADDR    0b1001100

#ifndef PAC_DEFAULT_SHUNT
#define PAC_DEFAULT_SHUNT 0.02
#endif

#define pacConfigReg            0x00
#define pacConverionReg         0x01
#define pacSourceSamplingReg    0x0A
#define pacSenseSamplingReg     0x0B

#define sensReg                 0x0D
#define sourceReg               0x11
#define powerReg                0x15

#define pacConfiguration    0b01100000
#define pacConversion       0b00
#define pacSourceSampling   0b00001100
#define pacSenseSampling    0b01110011

#include "mbed.h"
#include <cstdint>

#ifndef status_type
#define status_type
typedef enum status{
    STATUS_OK = 0,
    STATUS_NO_DATA = -1000,
    STATUS_TX_ERROR = -1001,
    STATUS_RX_ERROR = -1002,
    STATUS_RESERVED = -1003,
    STATUS_RESERVED2 = -1004,
    STATUS_UNKNOWN = -1005
}status_t;
#endif

class PAC{
public:
    PAC(I2C *boardI2C, uint8_t ADDR = PAC_DEFAULT_ADDR, float shunt = PAC_DEFAULT_SHUNT);
    status_t init(void);
    status_t getV(void);
    status_t getI(void);
    status_t getP(void);
    float voltage(void);
    float current(void);
    float power(void);

private:
    I2C *pacI2C;
    float shunt, v, i, p;
    uint8_t pacAddr;
};

#endif