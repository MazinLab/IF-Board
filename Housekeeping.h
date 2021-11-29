#ifndef __HOUSEKEEPING_H__
#define __HOUSEKEEPING_H__

//Arduino includes
#include <Arduino.h>
#include <EEPROM.h>
//Project includes
#include "pins.h"
#include "PE43705.h"
#include "TRF3765.h"




/*------------
Default Defines
-------------*/
#define DELAY_mS 1 //millisecond delay
#define DEFAULT_ATTENUATION 31.75 //Hard codes the default attenuation to be set for the PE43705


/*-------------------
Housekeeping Commands
---------------------*/

//Currently this architecture is not being used, but I don't want to delete it in case I go back to it later
/*----------------------------------------------------------------------------------------------------------
#define N_COMMANDS 6

struct Command
{
    String name;
    bool (*reference_command)(int device);
    bool allow_offline;
};
------------------------------------------------------------------------------------------------------------*/
//PE43705 Commands  
void set_attenuation(String &device, double &attenuation);

//TRF3765 Commands
void set_lo(double &frequency);

//Housekeeping commands
bool print_commands();
bool enable_defaults(String &device);
bool load_defaults(String &device);
bool tell_status();
bool reset(String &device);
bool temperature();

//Command UI
void interpret_command(String &command);



#endif
