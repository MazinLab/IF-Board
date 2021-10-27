#include "TRF3765.h"


//Gets contents of register address
void TRF3765::get_register(int address)
{
    //set readback mode with register 0
    //address bits for register 0:
    //bit0 = 0
    //bit1 = 0
    //bit2 = 0
    //bit3 = 1
    //bit4 = 0
    
    //other bits for register 0:
    //bit5:26 = 0

    //VCO frequency bit for register 0:
    //bit27 = 0 for maximum frequency, set = 1 for minimum frequency

    //register address selection
    if (address == 0b001)
    {
        //set bits 30..28 LSB to 001
    }
    else if (address == 0b010)
    {
        //set bits 30..28 LSB to 010
    }
    else if (address == 0b011)
    {
        //set bits 30..28 LSB to 011
    }
    else if (address == 0b100)
    {
        //set bits 30..28 LSB to 100
    }
    else if (address == 0b101)
    {
        //set bits 30..28 LSB to 101
    }
    else if (address == 0b110)
    {
        //set bits 30..28 LSB to 110
    }
    else if (address == 0b111)
    {
        //set bits 30..28 LSB to 111
    }
    else
    {
        std::printf("invalid register address");
    }

    //set into readback mode
    //bit31 = 1

    //unsure of how to actually tell computer to send/receive bits

    

}

//Sets contents of register address, no safety check performed, read datasheet?
void TRF3765::set_register(int address)
{
    //unsure of how to actually tell computer to send/receive bits
    //set register(logic for address?)

}

//Set LO to run at value
void TRF3765::lo_value(int value)
{
    //set_register()
    //i am in over my head
}

//applies defaults
void TRF3765::defaults()
{
    // :(
}

//load defaults: Stored settings will be applied else hard coded defaults will be applied and stored
void TRF3765::load_defaults()
{
//no clue how to do this without taking up large chunks of memory; focus on learning register stuff first
}

//enable defaults: If enabled, stored settings will be applied at power up
void TRF3765::enable_defaults()
{
//no clue how to do this, focus on learning register stuff first
}