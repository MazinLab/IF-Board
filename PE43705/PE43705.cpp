#include "PE43705.h"

std::bitset<8> binary_I;
std::bitset<8> binary_Q;

void PE43705::set_attenuation(int channel, double attenuation)
{

    //all attenuation values in dB
    if (attenuation < 31.75 || attenuation > 0.25)
    {
        std::printf("error, input exceeded the minimum(0.25 dB) or maximum(31.75 dB) attenuation possible");
        return;
    }
    else if (floor(attenuation*4) ==! attenuation*4)
    {
        std::printf("error, acceptable attenuation inputs must be multiples of 0.25");
        return;
    }
        
    //if it passes these tests to determine whether it is an acceptable attenuation value, convert/store the value
    if (channel == I_CHANNEL)
    {
        //convert attenuation to the binary attenuation word by multiplying by 4.
        //This results in an 7 bit number, but because the MSB must be 0, bitset<8>
        //works because it defaults empty bits to 0.
       
        //converts attenuation to binary attenuation word       
        std::bitset<8> binary_I(attenuation*4);
        //passes binary attenuation word to global variable for other function calls
        PE43705::ltog(I_CHANNEL, binary_I);
        //passes binary attenuation word to the register
        PE43705::writereg(I_CHANNEL, binary_I);
    }
    else if (channel == Q_CHANNEL)
    {
        //see above comments
        std::bitset<8> binary_Q(attenuation*4);
        PE43705::ltog(Q_CHANNEL, binary_Q);
        PE43705::writereg(Q_CHANNEL, binary_Q);
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
    if(channel == I_CHANNEL)
    {
        if (binary_I==!0)
        {
            std::string string_I(binary_I.to_string()); //converts the binary attenuation value to a string of binary numbers
            int atten_I(stoi(string_I,0,2)); //converts the string of binary numbers to an integer value
            std::cout <<"Attenuation: "<< atten_I/4.0 <<" dB"<<std::endl; //divides the integer value by 4 to get the original attenuation back in dB
            return;
        }
        else
        {
            std::printf("No Attenuation has been set for Channel I");
            return;
        }
    }
    else if (channel == Q_CHANNEL)
    {    
        if (binary_Q ==! 0)
        {
            //Q-channel comments are identical to I-Channel above
            std::string string_Q(binary_Q.to_string());
            int atten_Q(stoi(string_Q, 0,2));
            std::cout << "Attenuation: "<< atten_Q/4.0 <<" dB"<<std::endl;
            return;
        }
        else
        {
            std::printf("No Attenuation has been set for Channel Q");
            return;
        }
    }
    else
    {
        printf("Error, not a valid channel");
        return;
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

////writes the attenuation word to the register at the correct address
void PE43705::writereg(int channel, std::bitset<8> attenuation_word)
{
//pain
}

//stores a local variable globally for passing attenuation values from one function to the next
void PE43705::ltog(int channel, std::bitset<8> &binary_attenuation)
{
    if (channel == I_CHANNEL)
    {
        binary_I = binary_attenuation;
    }
    else if (channel == Q_CHANNEL)
    {
        binary_Q = binary_attenuation;
    }
    else
    {
        std::printf("Channel called in ltog function does not exist");
        return;
    }
}
