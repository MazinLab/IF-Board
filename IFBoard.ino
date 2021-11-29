//Arduino includes
#include <Arduino.h>
#include <EEPROM.h>
//Project includes
#include "Housekeeping.h"
#include "PE43705.h"
#include "TRF3765.h"
#include "pins.h"



//runs once to setup
void setup()
{
    Serial.begin(115200);
    pinMode(DATAOUT, OUTPUT);
    pinMode(DATAIN, INPUT);
    pinMode(CLK, OUTPUT);
    pinMode(CS, OUTPUT);
    pinMode(LE, OUTPUT);
    
    while (!Serial)
    {
        Serial.println("Initializing Serial Port...");
    }
}

//running on loop
void loop()
{
    Serial.println("Ready for Command!");
    while (Serial.available() == 0){}                 //Waits for user input
    String command = Serial.readString();             //Stores user input in Command
    Serial.println("Command Received: " + command);   //Reads back user input in console so user can see if they made a typo
    Serial.println("");
    interpret_command(command);                       //Passes the command to a function which interprets(See: too many if/else if statements) which command it is, then executes it if it's valid
    Serial.println(""); 
}
