#ifndef __HOUSEKEEPING_H__
#define __HOUSEKEEPING_H__

//Arduino includes
#include <Arduino.h>
//Project includes
#include "pins.h"
//#include "../PE43705/PE43705.h"
//#include "../TRF3765/TRF3765.h"


/*------------
Timing Defines
-------------*/
#define delay_ms 10 //millisecond delay


/*-------------------
Housekeeping Commands
---------------------*/
#define N_COMMANDS 6

struct Command
{
    String name;
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