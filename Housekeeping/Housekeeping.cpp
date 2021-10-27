#include "Housekeeping.h"


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
    std::cout<< printf("#print_commands - Prints this list of commands\n");
    std::cout<< printf("#enable_defaults - If enabled, stored settings will be applied at power up\n");
    std::cout<< printf("#load_defaults - Stored settings will be applied else hard coded defaults will be applied and stored\n");
    std::cout<< printf("#tell_status - Report all info\n");
    std::cout<< printf("#reset - Clear all data and reset eprom settings\n");
    std::cout<< printf("#temperature - Return the temperature sensor reading\n");

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
    oscillator.get_register(); 
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
