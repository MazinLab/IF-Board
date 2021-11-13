#ifndef DIGITAL_ATTENUATOR
#define DIGITAL_ATTENUATOR

#include "IC.h"


//One channel per each individual physical multiple of the chip(ie: we have two chips)
#define I_CHANNEL 0b00000000 //Upon startup, pin a0 a1 a2 of the I channel attenuator should be set low to easily reference it's address
#define Q_CHANNEL 0b11111111 //Upon startup, pin a0 a1 a2 of the Q channel attenuator should be set high to easily reference it's address

//Pin Definitions
#define SERIAL_IN 0b1
#define LATCH_ENABLE 0b10
#define CLOCK 0b11
#define PARALLEL_OR_SERIAL_MODE_SELECT 0b100

//Address pins
#define A0 0
#define A1 1
#define A2 2






//inheriting from IC in case there are commonalities to be added at a later date(ie: never)
class PE43705 : public IC
{
    public:
        
        int channel;
        /*----------------------------
        Low Level Methods for PE43705
        -----------------------------*/
        //writes the attenuation word to the register
        void writereg(int channel, std::bitset<8> attenuation);

        //stores a local variable globally for passing attenuation values from one function to the next
        void ltog(int channel, std::bitset<8> &binary_attenuation);

        /*----------------------------
        High Level Methods for PE43705
        -----------------------------*/
        
        //Applies hard coded defaults
        void defaults(int channel);

        //load defaults: Stored settings will be applied else hard coded defaults will be applied and stored
        void load_defaults(int channel);

        //enable defaults: If enabled, stored settings will be applied at power up
        void enable_defaults(int channel);
        
        //set the attenuation of the chip
        void set_attenuation(int channel, double attenuation);

        //'gets' the attenuation of the chip from the previously set attenuation
        void get_attenuation(int channel);

};



#endif