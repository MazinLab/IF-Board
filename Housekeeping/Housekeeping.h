#ifndef HOUSE
#define HOUSE




#include "TRF3765.h"
#include "PE43705.h"

/*---------------
Declaring objects
----------------*/

//defining the local oscillator
TRF3765 oscillator;

//defining the digital attenuator(will take a channel argument)
PE43705 attenuator;
    



/*-------------------
Housekeeping Commands
---------------------*/
#define N_COMMANDS 6

struct Command
{
    std::string name;
    bool (*reference_command)();
    bool allow_offline;
};


bool print_commands();
bool enable_defaults();
bool load_defaults();
bool tell_status();
bool reset();
bool temperature();







#endif