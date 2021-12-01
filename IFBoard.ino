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

    //Checks EEPROM Defaults. If enabled, it applies the default settings
    //LO enable check
    if(EEPROM.read(EEPROM_ENABLE_LO_ADDRESS)==1)
    {   
        Serial.println("LO Defaults are enabled at startup.");
        String device = "LO";
        load_defaults(device);
    }
    else
    {
        Serial.println("LO Defaults are not enabled at startup.");
    }

    //ATTEN_I enable check
    if(EEPROM.read(EEPROM_ENABLE_ATTEN_I_ADDRESS)==1)
    {   
        Serial.println("I channel Attenuator Defaults are enabled at startup.");
        String device = "ATTEN_I";
        load_defaults(device);
    }
    else
    {
        Serial.println("I channel Attenuator Defaults are not enabled at startup.");
    }

    //ATTEN_Q enable check
    if(EEPROM.read(EEPROM_ENABLE_ATTEN_Q_ADDRESS)==1)
    {   
        Serial.println("Q channel Attenuator Defaults are enabled at startup.");
        String device = "ATTEN_Q";
        load_defaults(device);
    }
    else
    {
        Serial.println("Q channel Attenuator Defaults are not enabled at startup.");
    }

    //Checks to make sure Serial Port is initialized
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
