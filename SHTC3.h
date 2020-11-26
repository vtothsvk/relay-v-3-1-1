#ifndef SHTC3_h
#define SHTC3_h

#include "mbed.h"
#include <cstdint>

#define shtcADDR    (0x70 << 1)

#define rstMSB      0x80
#define rstLSB      0x5D
#define wakeMSB     0x35
#define wakeLSB     0x17
#define slpMSB      0xB0
#define slpLSB      0x98
#define idMSB       0xEF
#define idLSB       0xC8
#define readMSB     0x5C
#define readLSB     0x24

#define idMask      0b100000111111

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

#ifndef shtc_data_buffer_type
#define shtc_data_buffer_type
typedef union shtc_data_buffer{
        struct{
            char hum[2];
            char crc1;
            char temp[2];
            char crc2;
        };
        char data[6];
    }shtc_data_buffer_t;
#endif

class SHTC{

public:
    SHTC(I2C *boardI2C);
    status_t init(void);
    status_t read(void);
    status_t reset(void);
    float lastTemp(void);
    float lastHum(void);
    int sensID(void);

private:
    status_t wake(void);
    status_t sleep(void);
    I2C *sensI2C;
    int id;
    float T;
    float H;
};

#endif