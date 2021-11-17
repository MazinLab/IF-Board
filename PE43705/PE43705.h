#ifndef DIGITAL_ATTENUATOR
#define DIGITAL_ATTENUATOR

#include "IC.h"


//One channel per each individual physical multiple of the chip(ie: we have two chips)
#define I_CHANNEL 0b0000000100000000 // A0 = 1, A1 = 0, A2 = 0 (labelled on schematic as IF2RFa, U2) 
#define Q_CHANNEL 0b0000001000000000 // A0 = 0, A1 = 1, A2 = 0 (labelled on schematic as IF2RFb, U1) 

//Pin Definitions(current pin numbers are placeholders and don't correspond to the actual pinouts of the chip)
#define SERIAL_IN 1
#define LATCH_ENABLE 2
#define CLOCK 3
#define PARALLEL_OR_SERIAL_MODE_SELECT 4









//inheriting from IC in case there are commonalities to be added at a later date(ie: never)
class PE43705 : public IC
{
    public:
        
        //declaring attenuation binary storage variables
        std::bitset<16> binary_I;
        std::bitset<16> binary_Q;

        /*----------------------------
        Low Level Methods for PE43705
        -----------------------------*/
        //writes the attenuation word to the register
        void writereg(std::bitset<16> channel, std::bitset<16> attenuation);

        //stores a local variable globally for passing attenuation values from one function to the next
        void ltog(std::bitset<16> channel, std::bitset<16> &binary_attenuation);

        /*----------------------------
        High Level Methods for PE43705
        -----------------------------*/
        
        //Applies hard coded defaults
        void defaults(std::bitset<16> channel);

        //load defaults: Stored settings will be applied else hard coded defaults will be applied and stored
        void load_defaults(std::bitset<16> channel);

        //enable defaults: If enabled, stored settings will be applied at power up
        void enable_defaults(std::bitset<16> channel);
        
        //set the attenuation of the chip
        void set_attenuation(std::bitset<16> channel, double attenuation);

        //'gets' the attenuation of the chip from the previously set attenuation
        double get_attenuation(std::bitset<16> channel);

};



#endif