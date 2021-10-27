#include "PE43705.h"






double attenuation_I;
double attenuation_Q;

void PE43705::set_attenuation(int channel, double &attenuation)
{

    
    //all attenuation values in dB
    if (attenuation > 31.75 || attenuation < 0.25)
    {
        std::printf("error, input exceeded the minimum(0.25 Db) or maximum(31.75 dB) attenuation possible");
    }
    else if (floor(attenuation*4) ==! attenuation*4)
    {
        std::printf("error, acceptable attenuation inputs must be multiples of 0.25");
    }
        
    //if it passes these tests to determine whether it is an acceptable attenuation value, store the value
    if (channel == I_CHANNEL)
    {
        attenuation_I *attenuation;
    }
    else if (channel == Q_CHANNEL)
    {
        attenuation_Q *attenuation;
    }
        
    
    
    
    /*
    The attenuation word is given by multiplying by 4 and converting to binary.
    
    For example, the attenuation of 18.25 Db is multiplied by 4 to yield 73.
    73 in binary is 1001001. However, the attenuation word consists of 8 bits.
    The most significant bit MUST be 0, so the attenuation word for this state is
    01001001.

    
    The serial interface is controlled using three CMOS compatible signals: 
    serial-in (SI), clock (CLK), and latch
    enable (LE). The SI and CLK inputs allow data to be
    serially entered into the shift register. Serial data is
    clocked in LSB first, beginning with the attenuation
    word.
    The shift register must be loaded while LE is held
    LOW to prevent the attenuator value from changing
    as data is entered. The LE input should then be
    toggled HIGH and brought LOW again, latching the
    new data into the DSA
    */
   
   
   //Todo list?
   //unsure how to actually tell the computer to input bits into the register
   //unsure how to convert attenuation inputs into binary

}




void PE43705::get_attenuation(int channel)
{
    if(attenuation_I || attenuation_Q ==!0)
    {
        if (channel == I_CHANNEL)
        {
            std::cout << attenuation_I;
        }
        else if (channel == Q_CHANNEL)
        {
            std::cout << attenuation_Q;
        }
    }
    else if(attenuation_I && attenuation_Q == 0)
        {
            std::printf("No Attenuation has been set");
        }
        

}

//applies defaults
void PE43705::defaults(int channel)
{
    //I_channel attenuator address pins A0, A1, A2 need to be held low
    //Q_Channel attenuator address pins A0, A1, A2 need to be held high

    //what should default attenuation be?
    
}

//load defaults: Stored settings will be applied else hard coded defaults will be applied and stored
void PE43705::load_defaults(int channel)
{
//no clue how to do this without taking up large chunks of memory; focus on learning register stuff first
}

//enable defaults: If enabled, stored settings will be applied at power up
void PE43705::enable_defaults(int channel)
{
//no clue how to do this, focus on learning register stuff first
}