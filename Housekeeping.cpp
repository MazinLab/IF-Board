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
    Serial.println("#enable_defaults - If enabled, stored settings will be applied at power up.");
    Serial.println("#load_defaults - Stored settings will be applied else hard coded defaults will be applied and stored.");
    Serial.println("#tell_status - Report all info.");
    Serial.println("#reset - Clear all data and reset eprom settings.");
    Serial.println("#temperature - Return the temperature sensor reading.");
    Serial.println("Note: Valid device names are 'LO', 'ATTEN_I', 'ATTEN_Q', and 'ALL'.");

    return true;
}

bool enable_defaults(String &device)
{
    if (device == "LO")
    {
        oscillator.enable_defaults();
    }
    else if (device == "ATTEN_I")
    {
        attenuator.enable_defaults(I_CHANNEL);
    }
    else if (device == "ATTEN_Q")
    {
        attenuator.enable_defaults(Q_CHANNEL);
    }
    else if (device == "ALL")
    {
        oscillator.enable_defaults();
        attenuator.enable_defaults(I_CHANNEL);
        attenuator.enable_defaults(Q_CHANNEL);
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
    //need to tell eeprom status commands? (implement this later)
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
        oscillator.reset();
        attenuator.reset(I_CHANNEL);
        attenuator.reset(Q_CHANNEL);
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

    //enable_defaults structure
    else if (command == "enable_defaults")
    {
        Serial.print("Input a device: ");
        while (Serial.available() == 0) {}
        String device = Serial.readString();
        Serial.print(device);
        Serial.println("");
        
        enable_defaults(device);
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
