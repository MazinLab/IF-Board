#include "Housekeeping.h"

/*---------------
Declaring objects
----------------*/
//defining the local oscillator
TRF3765 oscillator;
//defining the digital attenuator(will take a channel argument)
PE43705 attenuator;

Command commands[N_COMMANDS]
{
    //Prints this list of commands
    {"print_commands", print_commands, true},

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




bool print_commands()
{
    Serial.println("#print_commands - Prints this list of commands\n");
    Serial.println("#enable_defaults - If enabled, stored settings will be applied at power up\n");
    Serial.println("#load_defaults - Stored settings will be applied else hard coded defaults will be applied and stored\n");
    Serial.println("#tell_status - Report all info\n");
    Serial.println("#reset - Clear all data and reset eprom settings\n");
    Serial.println("#temperature - Return the temperature sensor reading\n");

    return true;
}

bool enable_defaults()
{
    oscillator.enable_defaults();
    attenuator.enable_defaults(I_CHANNEL);
    attenuator.enable_defaults(Q_CHANNEL);
    return true;
}

bool load_defaults()
{
    oscillator.load_defaults();
    attenuator.load_defaults(I_CHANNEL);
    attenuator.load_defaults(Q_CHANNEL);
    return true;
}

bool tell_status()
{
    oscillator.get_register(0b00010); 
    attenuator.get_attenuation(I_CHANNEL);
    attenuator.get_attenuation(Q_CHANNEL);
    return true;
}

bool reset()
{
    oscillator.defaults();
    attenuator.get_attenuation(I_CHANNEL);
    attenuator.get_attenuation(Q_CHANNEL);
    return true;
}

bool temperature()
{
    //get temperature from sensor
    return true;
}
