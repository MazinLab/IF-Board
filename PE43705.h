#ifndef __PE43705_h__
#define __PE43705_h__
#include <Arduino.h>

//One channel per chip
#define ADC1 0b011  // RF2IF  3
#define ADC2 0b100  // RF2IF  4
#define DAC1 0b001  // IF2RFa  1
#define DAC2 0b010  // IF2RFb  2
#define N_ATTEN_CHAN 4

#define PE_SPI_FREQ 1000000

typedef struct attens_t {
  float adc1;
  float adc2;
  float dac1;
  float dac2;
} attens_t;

class PE43705 {
    private:
        uint8_t _atten[N_ATTEN_CHAN];
        void _send(uint8_t channel, uint8_t attenbyte);

    public:
        PE43705();
        void power_off();
        bool set_atten(uint8_t channel, float attenuation);
        void set_attens(attens_t attens);
        attens_t get_attens();
};
#endif
