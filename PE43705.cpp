#include "PE43705.h"



void PE43705::set_attenuation(uint8_t channel, double attenuation)
{

    //all attenuation values in dB
    if (attenuation < 0.25)
    {
        Serial.println("ERROR: input exceeded the minimum(0.25 dB)attenuation possible");
        return;
    }
    else if (attenuation > 31.99) //For some reason, it's including 31.75 even without a less than equals sign, so I 'fixed' this bug by giving it the highest threshold that won't trigger an illegal value
    {
        Serial.println("ERROR: input exceeded the maximum(31.75 dB)attenuation possible");
        return;
    }
    else if (floor(attenuation*4) ==! attenuation*4)
    {
        Serial.println("ERROR: acceptable attenuation inputs must be multiples of 0.25");
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
    }
    else if (channel == Q_CHANNEL)
    {
        //see above comments
        uint8_t attenbyte_Q(attenuation*4);
        PE43705::ltog(Q_CHANNEL, attenbyte_Q);
        PE43705::writereg(Q_CHANNEL, attenbyte_Q);
    }

}




void PE43705::get_attenuation(uint8_t channel)
{
    if(channel == I_CHANNEL)    
    {
        if (attenbyte_I > 0)
        {
            int atten_I = attenbyte_I; //typecasts the byte to an int
            Serial.print("Channel I Attenuation: ");
            Serial.print(atten_I/4.0 ); //divides the integer value by 4 to get the original attenuation back in dB
            Serial.print(" dB"); 
            Serial.println("");
            return;
        }
        else
        {
            Serial.println  ("No Attenuation has been set for Channel I");
            return;
        }
    }
    else if (channel == Q_CHANNEL)
    {    
        if (attenbyte_Q > 0)
        {
            //Q-channel comments are identical to I-Channel above
            int atten_Q = attenbyte_Q;
            Serial.print("Channel Q Attenuation: ");
            Serial.print(atten_Q/4.0 ); 
            Serial.print(" dB"); 
            Serial.println("");
            return;
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

//clears eeprom settings
void PE43705::reset(uint8_t channel)
{
    if (channel == I_CHANNEL)
    {
        EEPROM.update(EEPROM_ENABLE_ATTEN_I_ADDRESS,0);
        EEPROM.update(EEPROM_I_DEFAULT_ATTEN_ADDRESS,0);
        Serial.println("I Channel EEPROM settings reset.");
    }
    else if (channel == Q_CHANNEL)
    {
        EEPROM.update(EEPROM_ENABLE_ATTEN_Q_ADDRESS,0);
        EEPROM.update(EEPROM_Q_DEFAULT_ATTEN_ADDRESS,0);
        Serial.println("Q Channel EEPROM settings reset.");
    }
    else
    {
        Serial.println("ERROR: Invalid channel passed in EEPROM Reset function");
    }
}

//store defaults: Current settings will be stored. Hard coded defaults will be stored if current setings haven't been set.
void PE43705::store_defaults()
{
    if (attenbyte_I > 0)
    {
        EEPROM.update(EEPROM_I_DEFAULT_ATTEN_ADDRESS, attenbyte_I);
    }
    else
    {
        EEPROM.update(EEPROM_I_DEFAULT_ATTEN_ADDRESS, DEFAULT_ATTENUATION);
    }

    if (attenbyte_Q > 0)
    {
        EEPROM.update(EEPROM_Q_DEFAULT_ATTEN_ADDRESS, attenbyte_Q);
    }
    else
    {
        EEPROM.update(EEPROM_Q_DEFAULT_ATTEN_ADDRESS, DEFAULT_ATTENUATION);
    }
}

//load defaults: Stored settings will be applied else hard coded defaults will be applied
void PE43705::load_defaults(uint8_t channel)
{
    if (channel == I_CHANNEL)
    {
        if ( (EEPROM.read(EEPROM_I_DEFAULT_ATTEN_ADDRESS) >= 1) && (EEPROM.read(EEPROM_I_DEFAULT_ATTEN_ADDRESS) <= 127) )
        {
            set_attenuation(I_CHANNEL,EEPROM.read(EEPROM_I_DEFAULT_ATTEN_ADDRESS)/4.0);
        }
        else
        {
            return;
        }
    }
    else if (channel == Q_CHANNEL)
    {
        if ( (EEPROM.read(EEPROM_Q_DEFAULT_ATTEN_ADDRESS) >= 1) && (EEPROM.read(EEPROM_Q_DEFAULT_ATTEN_ADDRESS) <= 127) )
        {
            set_attenuation(Q_CHANNEL,EEPROM.read(EEPROM_Q_DEFAULT_ATTEN_ADDRESS)/4.0);
        }
        else 
        {
            return;
        }
    }
    else
    {
        Serial.println("ERROR: Invalid channel called in load_defaults function");
    }
}

//enable defaults: If enabled, stored settings will be applied at power up. If disabled, stored settings will not be applied at power up.
void PE43705::toggle_defaults(uint8_t channel)
{
    if (channel == I_CHANNEL)
    {
        if (EEPROM.read(EEPROM_ENABLE_ATTEN_I_ADDRESS)==1)
        {
            EEPROM.write(EEPROM_ENABLE_ATTEN_I_ADDRESS,0);
            Serial.println("I Channel Attenuator Defaults are now disabled.");
        }
        else
        {
            EEPROM.write(EEPROM_ENABLE_ATTEN_I_ADDRESS,1);
            Serial.println("I Channel Attenuator Defaults are now enabled.");
        }
    }
    else if (channel == Q_CHANNEL)
    {
        if (EEPROM.read(EEPROM_ENABLE_ATTEN_Q_ADDRESS)==1)
        {
            EEPROM.write(EEPROM_ENABLE_ATTEN_Q_ADDRESS,0);
            Serial.println("Q Channel Attenuator Defaults are now disabled.");
        }
        else
        {
            EEPROM.write(EEPROM_ENABLE_ATTEN_Q_ADDRESS,1);
            Serial.println("Q Channel Atteunuator Defaults are now enabled.");
        }
    }
    else 
    {
        Serial.println("ERROR: Channel called in toggle_defaults function is invalid.");
    }
}

////writes the attenuation word to the register at the correct address
void PE43705::writereg(uint8_t channel, uint8_t attenbyte)
{
    if (channel == I_CHANNEL)
    {
      Serial.println("I_CHANNEL Attenuation set");
    }
    else if (channel == Q_CHANNEL)
    {
      Serial.println("Q_CHANNEL Attenuation set");
    }
    else
    {
        Serial.println("ERROR: Channel called in writereg function does not exist");
        return;
    }
    
    //parses arguments to form the attenuation word
    uint16_t atten_word;                                            //defines the 16 bit attenuation word
    atten_word = (uint16_t)channel;                                 //typecasts the 8bit channel to a 16bit number, and stores it in atten_word
    atten_word = atten_word << 8;                                   //bitshifts the 8bit channel so that the channel is the leftmost byte: (XXXX XXXX) 0000 0000
    atten_word = (uint16_t)atten_word | (uint16_t)attenbyte;        //bitwise inclusive OR to add the typecasted attenuation byte to the bitshifted channel to obtain the final 16 bit attenuation word


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
