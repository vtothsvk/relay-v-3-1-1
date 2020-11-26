#include "mbed.h"
#include "PAC.h"
#include "SHTC3.h"
#include "hall.h"
#include <iostream>

#define CAN_ADDR    0x0006
#define CAN_devType 0x0010
#define CAN_Idmask  0x3FF800

#define DEAD_TIME   300 //s

#define RM_MODE_DC
#define DEBUG_ON

#define relays  4

#define relay1  0
#define relay2  1
#define relay3  2
#define relay4  3

#define RMS 0
#define PtP 1

//Pin Setup//
PinName relayPin[relays] = {D3, D6, D11, D12};
PinName hallPin[relays] = {A3, A2, A1, A0};

#define relayCtrlId 0x10
#define PACId       0x20
#define SHTCId      0x30

#define R1Id    0x08
#define R2Id    0x04
#define RIDmask 0x0C
#define stateId 0x02
#define RW      0x01

typedef struct relay_ch{
    bool state = false;
    float i = 0.0;
}relay_t;

//Booting//
void Relay_init(void);
void CAN_init(void);
void relayTest();

void LED(void);

//Relay control//
void relayCtrl(uint8_t rel, bool state);

float getCurrent(uint8_t rel);

//CAN callback//
void CAN_cb(void);
void CAN_incoming_data_cb(CANMessage *msg);
status_t CAN_pub(CANMessage msg);

void blink(void);

//CAN//
CAN can(PA_11, PA_12);
CANMessage msgBuffer;

//I2C//
I2C i2c(D4, D5);

//SHTC3 temp&hum//
SHTC shtc(&i2c);

//GPIO//
DigitalOut led(LED1);
DigitalOut relay[relays]{
    DigitalOut(relayPin[relay1]), 
    DigitalOut(relayPin[relay2]), 
    DigitalOut(relayPin[relay3]), 
    DigitalOut(relayPin[relay4])
};

//Timers//
Ticker LEDtim;

//CAN Thread//
EventQueue CAN_cb_queue;
Thread CANThread;

//PAC//
PAC pac(&i2c);

//Hall//
hallSens hall[4]{
    hallSens(hallPin[0]), 
    hallSens(hallPin[1]), 
    hallSens(hallPin[2]),
    hallSens(hallPin[3], 1.55),  
};

//Relay channels//
relay_t relayCh[relays];
volatile int elapsed = 0;

int main(void){
    cout << "Booting Relay module..." << endl;

    //Booting sequence
    LEDtim.attach(&LED, 1);
    Relay_init();
    CAN_init();
    if(shtc.init()){
        cout << "shtc error" << endl;
    }

    relayTest();
    
    relay[0] = 1;
    relay[1] = 1;
    relay[2] = 1;
    relay[3] = 1;
    cout << "Booting sequence completed" << endl;

    while(elapsed <= DEAD_TIME){
        #ifdef RM_MODE_AC
        for(uint8_t i = 0; i < 4; i++){
            relayCh[i].i = getCurrent(i);
        }
        elapsed += 2;
        #endif

        #ifdef RM_MODE_DC
        for(uint8_t i = 0; i < 4; i++){
            relayCh[i].i = getCurrent(i);
        }

        cout << relayCh[0].i << ", " << relayCh[1].i << ", " << relayCh[2].i << ", " <<relayCh[3].i << endl;

        ThisThread::sleep_for(1000);
        elapsed++;
        #endif
    }//loop end

    NVIC_SystemReset();
}

void Relay_init(){
    for(uint8_t i = 0; i < 4; i++){
        relayCtrl(i, 0);
    }
    if(pac.init()){
        cout << "PAC failed to initialise..." << endl;
    }
}//Relay_init

void CAN_init(){
    int canID = (CAN_devType << 17)|(CAN_ADDR << 11);    //set device CAN address
    can.frequency(250000);                               //set CAN inteface frequency
    CANThread.start(callback(&CAN_cb_queue, &EventQueue::dispatch_forever));    //initialise thread runnning incoming CAN data callback
    can.filter(canID, CAN_Idmask, CANExtended);          //initialise CAN filter on given address
    can.attach(CAN_cb_queue.event(&CAN_cb), CAN::RxIrq); //attach interrupt on incoming CAN transmissions
}//CAN_init

void CAN_cb(){
    if(can.read(msgBuffer, 0)){ //read incoming message, if available
        elapsed = 0;            //reset watchdog timelout
        CAN_cb_queue.call(&CAN_incoming_data_cb, &msgBuffer); //callback on incoming message data
    }
    blink();
}//CAN_cb

//CAN incoming data callback
void CAN_incoming_data_cb(CANMessage *msg){
    int commandId = msg -> id; //incoming message CAN ID contains commandID

    if((commandId&relayCtrlId) == relayCtrlId){ 
        //Relay state control (R/W)
        if(commandId&RW){
            //Read
            char buffer[8];
            long relayCurrents;
            for(uint8_t i = 0; i < 4; i++){
                buffer[i] = relayCh[i].state;
                long iBuffer = (long)(relayCh[i].i * 10);
                relayCurrents += (iBuffer << (8 * i));
            }//for end

            memcpy(&buffer[4], &relayCurrents, 4);
            CAN_cb_queue.call(&CAN_pub, CANMessage(relayCtrlId + 1, &buffer[0], 8, CANData, CANExtended));
        }//if R/W
        else{
            //Write
            cout << "Relay " << ((commandId&RIDmask) >> 2) + 1 << ": " << (((commandId&stateId) >> 1) ? "ON" : "OFF") << endl;
            relayCtrl((commandId&RIDmask) >> 2, (commandId&stateId) >> 1);
        }//else R/W
    }

    else if((commandId&PACId) == PACId){
        //PAC VIP readidng (R)
        cout << pac.getV() << endl;
        cout << pac.getI() << endl;

        char buffer[8];
        float v = pac.voltage();
        float i = pac.current();;

        memcpy(&buffer[0], &v, 4);
        memcpy(&buffer[4], &i, 4);

        CAN_cb_queue.call(&CAN_pub, CANMessage(PACId, &buffer[0], 8, CANData, CANExtended));
    }//VIP

    else if((commandId&SHTCId) == SHTCId){
        //SHTC ambient temperature and humidity reading (R)
        shtc.read();
        char buffer[8];
        float ambientTemp = shtc.lastTemp();
        float ambientHum = shtc.lastHum();

        memcpy(&buffer[0], &ambientTemp, 4);
        memcpy(&buffer[4], &ambientHum, 4);

        CAN_cb_queue.call(&CAN_pub, CANMessage(SHTCId, &buffer[0], 8, CANData, CANStandard));
    }//SHTC
}//CAN_incoming_data_cb

status_t CAN_pub(CANMessage msg){
    if(can.write(msg)){
        return STATUS_TX_ERROR;
    }

    return STATUS_OK;
}

void LED(void){
    led = !led;
}

void relayCtrl(uint8_t rel, bool state){
    relay[rel] = state;
    relayCh[rel].state = state;
}

float getCurrent(uint8_t relay){
    #ifdef RM_MODE_AC
    Timer amp;
    float maxval = 0, minval = 999;
    float buffer;

    amp.start();
    int begin = amp.read_ms();

    while((amp.read_ms() - begin) <= 500){
        hall[relay].read();
        buffer = hall[relay].current();

        if(buffer > maxval){
            maxval = buffer;
        }

        if(buffer < minval){
            minval = buffer;
        }
    }

    return ((maxval - minval)*0.707);
    #endif

    #ifdef RM_MODE_DC
    return hall[relay].read();
    #endif
};

void blink(void){
    led = 1;
    ThisThread::sleep_for(25);
    led = 0;
    ThisThread::sleep_for(25);
    led = 1;
    ThisThread::sleep_for(25);
    led = 0;
}

void relayTest(){
    for (uint8_t i = 0; i < 4; i++) {
        relay[i] = 1;
        ThisThread::sleep_for(200);
    }

    for (uint8_t i = 0; i < 4; i++) {
        relay[i] = 0;
        ThisThread::sleep_for(200);
    }
}