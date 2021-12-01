#ifndef __PE43705_h__ 
#define __PE43705_h__

//Project includes
#include "IC.h"
#include "Housekeeping.h"

//One channel per each individual physical multiple of the chip(ie: we have two chips)
#define I_CHANNEL 0b00000001 // A0 = 1, A1 = 0, A2 = 0 (labelled on schematic as IF2RFa, U2) 
#define Q_CHANNEL 0b00000010 // A0 = 0, A1 = 1, A2 = 0 (labelled on schematic as IF2RFb, U1) 






//inheriting from IC in case there are commonalities to be added at a later date(ie: never)
class PE43705 : public IC
{
    public:
        
        //declaring attenuation binary storage variables
        uint8_t attenbyte_I;
        uint8_t attenbyte_Q;

        /*----------------------------
        Low Level Methods for PE43705
        -----------------------------*/
        //writes the attenuation word to the register
        void writereg(uint8_t channel, uint8_t attenbyte);

        //stores a local variable globally for passing attenuation values from one function to the next
        void ltog(uint8_t channel, uint8_t &attenuation_byte);

        /*----------------------------
        High Level Methods for PE43705
        -----------------------------*/
        
        //clears eeprom settings
        void reset(uint8_t channel);

        //store defaults: Current settings will be stored. Hard coded defaults will be stored if current seetings haven't been set.
        void store_defaults();

        //load defaults: Stored settings will be applied else hard coded defaults will be applied
        void load_defaults(uint8_t channel);

        //enable defaults: If enabled, stored settings will be applied at power up. If disabled, stored settings will be applied at power up. 
        void toggle_defaults(uint8_t channel);
        
        //set the attenuation of the chip
        void set_attenuation(uint8_t channel, double attenuation);

        //'gets' the attenuation of the chip from the previously set attenuation
        void get_attenuation(uint8_t channel);

};



#endif
