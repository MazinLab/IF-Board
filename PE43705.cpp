#include "PE43705.h"
#include "pins.h"
#include <SPI.h> 

PE43705::PE43705(){
  power_off();
}


void PE43705::power_off() {
  for (int i=0;i<N_ATTEN_CHAN;i++) _atten[i]=127;
}


bool PE43705::set_atten(uint8_t channel, float attenuation) {
    if (channel > N_ATTEN_CHAN || channel < 1) return false;
    uint8_t atten = round(attenuation*4);

    if (attenuation > 127) {
        Serial.println(F("#ERROR:  Attenuation must be between 0-31.75 dB"));
        return false;
    }

    _atten[channel-1] = atten;
    _send(channel, atten);
    return true;
}


bool PE43705::set_attens(attens_t attens) {
  set_atten(DAC1, attens.dac1);
  set_atten(DAC2, attens.dac2);
  set_atten(ADC1, attens.adc1);
  set_atten(ADC2, attens.adc2);
}

float PE43705::get_atten(uint8_t channel) {
    if (channel > N_ATTEN_CHAN || channel < 1) return false;
    return _atten[channel-1]/4.0;
}


void PE43705::_send(uint8_t channel, uint8_t attenbyte) {
  uint16_t atten_word;
  atten_word = (((uint16_t) channel) <<8) | (attenbyte&0x7f);  //p7 of datasheet specifies D7 must be logic low

  digitalWrite(PIN_SS, HIGH);
  SPI.beginTransaction(SPISettings(PE_SPI_FREQ, LSBFIRST, SPI_MODE0));
  SPI.transfer(atten_word);
  SPI.endTransaction();
  digitalWrite(PIN_SS, LOW);
}

void PE43705::tell_status() {

  Serial.print(F("RF -- "));Serial.print(_atten[ADC1-1]/(double) 4.0, 2);Serial.print(F(" dB -- "));
  Serial.print(_atten[ADC2-1]/(double) 4.0,2);Serial.println(F(" dB -- IF"));

  Serial.print(F("IF -- "));Serial.print(_atten[DAC1-1]/(double) 4.0, 2);Serial.print(F(" dB -- "));
  Serial.print(_atten[DAC2-1]/(double) 4.0,  2);Serial.println(F(" dB -- RF"));
}
