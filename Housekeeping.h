#ifndef __HOUSEKEEPING_H__
#define __HOUSEKEEPING_H__

//Arduino includes
#include <Arduino.h>
#include <EEPROM.h>

//Project includes
#include "pins.h"
#include "PE43705.h"
#include "TRF3765.h"

/*---------------
EEPROM addresses
---------------*/
//Atmega32u4 has 512 bytes of EEPROM, so valid addresses are from 0x0000 to 0x01FF

//PE43705 EEPROM
#define EEPROM_ENABLE_ATTEN_I_ADDRESS 0x0000      //1byte   0=unconfigured(disabled) 1=enabled anything else -> 0
#define EEPROM_ENABLE_ATTEN_Q_ADDRESS 0x0001      //1byte   0=unconfigured(disabled) 1=enabled anything else -> 0
#define EEPROM_I_DEFAULT_ATTEN_ADDRESS 0x0002     //1byte   0=unconfigured(disabled) 1-127=atten_I*4 anything else -> 0
#define EEPROM_Q_DEFAULT_ATTEN_ADDRESS 0x0003     //1byte   0=unconfigured(disabled) 1-127=atten_Q*4 anything else -> 0
//TRF3765 EEPROM
#define EEPROM_ENABLE_LO_ADDRESS 0x0004           //1byte   0=unconfigured(disabled) 1=enabled anything else -> 0
#define EEPROM_LO_DEFAULT_FREQ_ADDRESS 0x0005     //not sure how many bytes needed to store frequency defaults, come back to after TRF3765 coded


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
bool store_defaults();
bool toggle_defaults(String &device);
bool load_defaults(String &device);
bool tell_status();
bool reset(String &device);
bool temperature();

//Command UI
void interpret_command(String &command);



#endif
