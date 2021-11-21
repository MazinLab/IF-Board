//Arduino includes
#include <Arduino.h>
//Project includes
#include "PE43705/PE43705.h"
#include "TRF3765/TRF3765.h"
#include "Housekeeping/Housekeeping.h"
#include "Housekeeping/pins.h"



//runs once to setup
void setup()
{
    
    Serial.begin(115200);
    pinMode(DATAOUT, OUTPUT);
    pinMode(DATAIN, INPUT);
    pinMode(CLK, OUTPUT);
    pinMode(CS, OUTPUT);
    pinMode(LE, OUTPUT);
    

}

//running on loop
void loop()
{
    //borrow UI from ifushoe.ino

}
