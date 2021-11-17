#include "Housekeeping.h"



//global variables
PE43705 attenuator;
TRF3765 oscillator;  


//runs once to setup
void setup()
{
    Serial.begin(115200);
    pinMode(DATAOUT, Output);
    pinMode(DATAIN, Input);
    pinMode(CLK, Output);
    pinMode(CS, Output);
    pinMode(LE, Output);

    
}

//running on loop
void loop()
{
    //borrow UI from ifushoe.ino

}