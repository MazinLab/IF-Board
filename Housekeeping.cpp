#include "Housekeeping.h"

/*---------------
Declaring objects
----------------*/
//defining the local oscillator
TRF3765 oscillator;
//defining the digital attenuator(will take a channel argument)
PE43705 attenuator;

//Currently this architecture is not being used, but I don't want to delete it in case I go back to it later
/*----------------------------------------------------------------------------------------------------------
Command commands[N_COMMANDS]
{
    //Prints this list of commands
    {"print_commands", print_commands,true},

    //If enabled, stored settings will be applied at power up
    {"enable_defaults", enable_defaults, true},

    //Stored settings will be applied else hard coded defaults will be applied and stored
    {"load_defaults", load_defaults, false},

    //Report all info
    {"tell_status", tell_status, false},

    //Clear all data and reset eprom settings
    {"reset", reset, false},

    //Return the temperature sensor reading
    {"temperature", temperature, true}

};
-------------------------------------------------------------------------------------------------------------*/

/*--------------
PE43705 Commands
---------------*/
void set_attenuation(String &device, double &attenuation)
{
    if (device == "ATTEN_I")
    {
        attenuator.set_attenuation(I_CHANNEL, attenuation);
    }
    else if (device == "ATTEN_Q")
    {
        attenuator.set_attenuation(Q_CHANNEL, attenuation);
    }
    else if (device == "ALL")
    {
        attenuator.set_attenuation(I_CHANNEL, attenuation);
        attenuator.set_attenuation(Q_CHANNEL, attenuation);
    }
    else
    {
        Serial.println("Not a valid device. Valid devices are 'ATTEN_I', 'ATTEN_Q', and 'ALL'.");
    }
}


/*--------------
TRF3765 Commands
---------------*/
void set_lo(double &frequency)
{
    oscillator.set_lo(frequency);
}



/*-------------------
Housekeeping Commands
--------------------*/
bool print_commands()
{
    Serial.println("PE43705 COMMANDS:");
    Serial.println("#set_attenuation - Sets the attenuation of the PE43705.");
    Serial.println("");
    Serial.println("TRF3765 COMMANDS:");
    Serial.println("#set_lo - Sets the TRF3765 to oscillate at the frequency input.");
    Serial.println("");
    Serial.println("HOUSEKEEPING COMMANDS:");
    Serial.println("#print_commands - Prints this list of commands.");
    Serial.println("#store_defaults - Current settings will be stored. Hard coded defaults will be stored if current seetings haven't been set.");
    Serial.println("#toggle_defaults - If enabled, stored settings will be applied at power up. If disabled, stored settings will not be applied at power up.");
    Serial.println("#load_defaults - Stored settings will be applied else hard coded defaults will be applied.");
    Serial.println("#tell_status - Report all info.");
    Serial.println("#reset - Resets EEPROM settings.");
    Serial.println("#temperature - Return the temperature sensor reading.");
    Serial.println("Note: Valid device names are 'LO', 'ATTEN_I', 'ATTEN_Q', and 'ALL'.");

    return true;
}

bool store_defaults()
{
    oscillator.store_defaults();
    attenuator.store_defaults();
    Serial.println("Current settings stored.");
    
    return true;
}

bool toggle_defaults(String &device)
{
    if (device == "LO")
    {
        oscillator.toggle_defaults();
    }
    else if (device == "ATTEN_I")
    {
        attenuator.toggle_defaults(I_CHANNEL);
    }
    else if (device == "ATTEN_Q")
    {
        attenuator.toggle_defaults(Q_CHANNEL);
    }
    else if (device == "ALL")
    {
        oscillator.toggle_defaults();
        attenuator.toggle_defaults(I_CHANNEL);
        attenuator.toggle_defaults(Q_CHANNEL);
    }
    else
    {
        Serial.println("Not a valid device. Valid devices are 'LO', 'ATTEN_I', 'ATTEN_Q', and 'ALL'.");
    }
    
    return true;
}

bool load_defaults(String &device)
{
    if (device == "LO")
    {
        oscillator.load_defaults();
    }
    else if (device == "ATTEN_I")
    {
        attenuator.load_defaults(I_CHANNEL);
    }
    else if (device == "ATTEN_Q")
    {
        attenuator.load_defaults(Q_CHANNEL);
    }
    else if (device == "ALL")
    {
        oscillator.load_defaults();
        attenuator.load_defaults(I_CHANNEL);
        attenuator.load_defaults(Q_CHANNEL);
    }
    else
    {
        Serial.println("Not a valid device. Valid devices are 'LO', 'ATTEN_I', 'ATTEN_Q', and 'ALL'.");
    }
    
    return true;
}

bool tell_status()
{
    //Channel I attenuator EEPROM default status
    if ( (EEPROM.read(EEPROM_I_DEFAULT_ATTEN_ADDRESS) >= 1) && (EEPROM.read(EEPROM_I_DEFAULT_ATTEN_ADDRESS) <= 127) )
    {
        Serial.println("");
        Serial.print("I Channel Default Attention(EEPROM) set to ");
        Serial.print(EEPROM.read(EEPROM_ENABLE_ATTEN_I_ADDRESS)/4.0);
        Serial.print(" dB. ");
    }
    else
    {
        Serial.println("I Channel Default Attenuation(EEPROM) not initialized.");
    }
    //Channel I attenuator EEPROM enable status
    if (EEPROM.read(EEPROM_ENABLE_ATTEN_I_ADDRESS)==1)
    {
        Serial.println("I Channel defaults are enabled at power up.");
    }
    else
    {
        Serial.println("I Channel defaults are not enabled at power up.");
    }

    //Channel Q attenuator EEPROM default status
    if ( (EEPROM.read(EEPROM_Q_DEFAULT_ATTEN_ADDRESS) >= 1) && (EEPROM.read(EEPROM_Q_DEFAULT_ATTEN_ADDRESS) <= 127) )
    {
        Serial.println("");
        Serial.print("Q Channel Default Attenuation(EEPROM) set to ");
        Serial.print(EEPROM.read(EEPROM_ENABLE_ATTEN_Q_ADDRESS)/4.0);
        Serial.print(" dB. ");
    }
    else
    {
        Serial.println("Q Channel Default Attenuation(EEPROM) not initialized.");
    }
    //Channel Q attenuator EEPROM enable status
    if (EEPROM.read(EEPROM_ENABLE_ATTEN_Q_ADDRESS)==1)
    {
        Serial.println("Q Channel defaults are enabled at power up.");
    }
    else
    {
        Serial.println("Q Channel defaults are not enabled at power up.");
    }
    
    //LO EEPROM default status
    Serial.println("");
    Serial.print("LO Default Frequency(EEPROM) set to ");
    Serial.print(EEPROM.read(EEPROM_LO_DEFAULT_FREQ_ADDRESS));
    Serial.print(" Hz. ");
    //LO EEPROM enable status
    if (EEPROM.read(EEPROM_ENABLE_LO_ADDRESS)==1)
    {
        Serial.println("LO defaults are enabled at power up.");
    }
    else
    {
        Serial.println("LO defaults are not enabled at power up.");
    }


    //RAM settings status
    oscillator.get_lo(); 
    attenuator.get_attenuation(I_CHANNEL);
    attenuator.get_attenuation(Q_CHANNEL);
    return true;
}

bool reset(String &device)
{
    if (device == "LO")
    {
        oscillator.reset();
    }
    else if (device == "ATTEN_I")
    {
        attenuator.reset(I_CHANNEL);
    }
    else if (device == "ATTEN_Q")
    {
        attenuator.reset(Q_CHANNEL);
    }
    else if (device == "ALL")
    {
        //initializes and clears the entire EEPROM
        for (int adr = 0; adr < EEPROM.length(); adr ++)
        {
            EEPROM.update(adr,0);
        }
        Serial.println("All EEPROM settings cleared.");
    }
    else
    {
        Serial.println("Not a valid device. Valid devices are 'LO', 'ATTEN_I', 'ATTEN_Q', and 'ALL'.");
    }
    return true;
}

bool temperature()
{
    //get temperature from sensor(do this after writing TRF3765 functionality)
    return true;
}




/*---------
Command UI
---------*/
void interpret_command(String &command)
{
    //set_attenuation structure (PE43705 Specific)
    if (command == "set_attenuation")
    {
        Serial.print("Input a device: ");
        while (Serial.available() == 0) {}
        String device = Serial.readString();
        Serial.print(device);
        Serial.println("");

        Serial.print("Input an attenuation: ");
        while (Serial.available() == 0) {}
        double attenuation = Serial.parseFloat();
        Serial.print(attenuation);
        Serial.println("");

        set_attenuation(device, attenuation);
        Serial.println("");
    }

    //set_lo structure (TRF3765 Specific)
    else if (command == "set_lo")
    {
        Serial.print("Input a frequency: ");
        while (Serial.available() == 0) {}
        double frequency = Serial.parseFloat();
        Serial.print(frequency);
        Serial.println("");
        
        set_lo(frequency);
        Serial.println("");
    }

    //print_commands structure
    else if (command == "print_commands")
    {
        print_commands();
        Serial.println("");
    }

    //store_defaults structure
    else if (command == "store_defaults")
    {
        store_defaults();
        Serial.println();
    }

    //enable_defaults structure
    else if (command == "toggle_defaults")
    {
        Serial.print("Input a device: ");
        while (Serial.available() == 0) {}
        String device = Serial.readString();
        Serial.print(device);
        Serial.println("");
        
        toggle_defaults(device);
        Serial.println("");
    }

    //load_defaults structure
    else if (command == "load_defaults")
    {
        Serial.print("Input a device: ");
        while (Serial.available() == 0) {}
        String device = Serial.readString();
        Serial.print(device);
        Serial.println("");
        
        load_defaults(device);
        Serial.println("");
    }

    //tell_status structure
    else if (command == "tell_status")
    {
        tell_status();
        Serial.println("");
    }

    //reset structure
    else if (command == "reset")
    {
        Serial.print("Input a device: ");
        while (Serial.available() == 0) {}
        String device = Serial.readString();
        Serial.print(device);
        Serial.println("");
        
        reset(device);
        Serial.println("");
    }

    //temperature structure
    else if(command == "temperature")
    {
        temperature();
        Serial.println("");
    }

    //error message
    else
    {
        Serial.println("Command received is not a command. Type 'print_commands' to see the command list.");
        Serial.println("Also, make sure your Serial Monitor is set to 'No line ending', the default is 'Newline'");
    }
}
