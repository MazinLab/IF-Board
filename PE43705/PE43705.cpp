#include "PE43705.h"



void PE43705::set_attenuation(uint8_t channel, double attenuation)
{

    //all attenuation values in dB
    if (attenuation < 31.75 || attenuation > 0.25)
    {
        Serial.println("error, input exceeded the minimum(0.25 dB) or maximum(31.75 dB) attenuation possible");
        return;
    }
    else if (floor(attenuation*4) ==! attenuation*4)
    {
        Serial.println("error, acceptable attenuation inputs must be multiples of 0.25");
        return;
    }
        
    //if it passes these tests to determine whether it is an acceptable attenuation value, convert/store the value
    if (channel == I_CHANNEL)
    {
        //convert attenuation to the binary attenuation word by multiplying by 4.
        //The valid range of attenuations multiplied by 4 converted to binary is a 7 bit number.
        //The MSB must be 0, and typecasting it to a uint8_t works because it defaults empty bits to 0.
    
        //converts attenuation to binary attenuation word       
        uint8_t attenbyte_I(attenuation*4);
        //passes binary attenuation to class variable for use by other function calls
        PE43705::ltog(I_CHANNEL, attenbyte_I);
        //passes binary attenuation word to the register
        PE43705::writereg(I_CHANNEL, attenbyte_I);
        Serial.println("I_CHANNEL Attenuation set");
    }
    else if (channel == Q_CHANNEL)
    {
        //see above comments
        uint8_t attenbyte_Q(attenuation*4);
        PE43705::ltog(Q_CHANNEL, attenbyte_Q);
        PE43705::writereg(Q_CHANNEL, attenbyte_Q);
        Serial.println("Q_CHANNEL Attenuation set");
    }

}




double PE43705::get_attenuation(uint8_t channel)
{
    if(channel == I_CHANNEL)    
    {
        if (attenbyte_I ==! 0)
        {
            int atten_I = attenbyte_I; //typecasts the byte to an int
            Serial.print("Attenuation: ");
            Serial.print(atten_I/4.0 ); //divides the integer value by 4 to get the original attenuation back in dB
            Serial.print(" dB"); 
            return atten_I/4.0; //returns the value of the attenuation
        }
        else
        {
            Serial.println  ("No Attenuation has been set for Channel I");
            return;
        }
    }
    else if (channel == Q_CHANNEL)
    {    
        if (attenbyte_Q ==! 0)
        {
            //Q-channel comments are identical to I-Channel above
            int atten_Q = attenbyte_Q;
            Serial.print("Attenuation: ");
            Serial.print(atten_Q/4.0 ); 
            Serial.print(" dB"); 
            return atten_Q/4.0;
        }
        else
        {
            Serial.println("No Attenuation has been set for Channel Q");
            return;
        }
    }
    else
    {
        Serial.println("Error, not a valid channel");
        return;
    }

}

//applies defaults, clears eeprom
void PE43705::reset(uint8_t channel)
{
    //clear eeprom

    set_attenuation(channel, DEFAULT_ATTENUATION);
}

//load defaults: Stored settings will be applied else hard coded defaults will be applied and stored
void PE43705::load_defaults(uint8_t channel)
{

}

//enable defaults: If enabled, stored settings will be applied at power up
void PE43705::enable_defaults(uint8_t channel)
{
    /*if (enable == true)
    {
        load_defaults(channel); //if defaults are enabled, load defaults
    }
    else
    {
        return; //if defaults are not enabled, don't do anything
    }*/
}

////writes the attenuation word to the register at the correct address
void PE43705::writereg(uint8_t channel, uint8_t attenbyte)
{
    if (channel ==! I_CHANNEL || Q_CHANNEL)
    {
        Serial.println("Channel called in writereg function does not exist");
        return;
    }
    
    
    uint16_t atten_word; //defines the 16 bit attenuation word
    atten_word = (uint16_t)channel; //typecasts the 8bit channel to a 16bit number, and stores it in atten_word
    atten_word << 8; //bitshifts the channel so that the channel is the leftmost byte: (XXXX XXXX) 0000 0000
    atten_word = atten_word | (uint16_t)attenbyte; //bitwise inclusive OR to add the typecasted attenuation byte to the bitshifted channel to obtain the final 16 bit attenuation word


    //Transfer attenuation word to register
    digitalWrite(LE, LOW);
    for (int i = 0; i < 16; i++)
    {
        digitalWrite(DATAOUT, ((atten_word>>i)&1));
        delay(DELAY_mS);
        digitalWrite(LE, HIGH);
        digitalWrite(LE,LOW);
    }
    digitalWrite(LE, HIGH);
    
    
}

//alters the scope of the variable from local to class(global) for passing attenuation values from one function to the next
void PE43705::ltog(uint8_t channel, uint8_t &attenuation_byte)
{
    if (channel == I_CHANNEL)
    {
        attenbyte_I = attenuation_byte;
    }
    else if (channel == Q_CHANNEL)
    {
        attenbyte_Q = attenuation_byte;
    }
    else
    {
        Serial.println("Channel called in ltog function does not exist");
        return;
    }
}

