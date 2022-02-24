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
    if (channel != ADC1 && channel != ADC2 &&
        channel != DAC1 && channel != DAC2)
        return false;

    uint8_t atten = round(attenuation*4);

    if (atten > 127) {
        Serial.println(F("#ERROR: Attenuation must be between 0-31.75 dB"));
        return false;
    }

    _atten[channel-1] = atten;
    _send(channel, atten);
    return true;
}


void PE43705::set_attens(attens_t attens) {
  set_atten(DAC1, attens.dac1);
  set_atten(DAC2, attens.dac2);
  set_atten(ADC1, attens.adc1);
  set_atten(ADC2, attens.adc2);
}

attens_t PE43705::get_attens() {
    attens_t ret;
    ret.dac1=_atten[DAC1-1]/4.0;
    ret.dac2=_atten[DAC2-1]/4.0;
    ret.adc1=_atten[ADC1-1]/4.0;
    ret.adc2=_atten[ADC2-1]/4.0;
    return ret;
}


void PE43705::_send(uint8_t channel, uint8_t attenbyte) {
  SPI.beginTransaction(SPISettings(PE_SPI_FREQ, LSBFIRST, SPI_MODE0));
  SPI.transfer(attenbyte & 0x7f); //p7 of datasheet specifies D7 must be logic low
  SPI.transfer(channel & 0b00000111);
  SPI.endTransaction();
  digitalWrite(PIN_SS, HIGH);
  digitalWrite(PIN_SS, LOW);
  SPI.end();
}
