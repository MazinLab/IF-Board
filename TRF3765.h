#ifndef __TRF3765_h__
#define __TRF3765_h__
#include <Arduino.h>
#include "trf3765regs.h"


#define TRF_SPI_FREQ 1000000
#define DEFAULT_FREF 10 //MHz
#define LOCK_TIME_MS 20

#define CAL_CLOCK_FREQ_MHZ 0.3

#define XSP_REG0_WRITE      0x80000028  /* Test register write value */
#define XSP_REG1_WRITE      0x68300149  /* Test register write value */
#define XSP_REG2_WRITE_ENC  0x88A0BB8A  /* Test register write value 6GHz*/
#define XSP_REG2_WRITE      0x08A0BB8A  /* Test register write value 6GHz*/
#define XSP_REG4_WRITE      0x0800E00C  /* Test register write value */
#define XSP_REG5_WRITE      0x908E968D  /* Test register write value */
#define XSP_REG6_WRITE      0x5441100E  /* Test register write value */
#define XSP_REG4_WRITE_FRAC 0b11001010100111001110000000001100 //ANA = 1 (both in and out) Speedup is on
//Used to be 0x1086968D, changed 1086 -> 108E Jul 14 Buffout bias changed to 01 (from 11)
#define XSP_REG5_WRITE_FRAC 0b00010000100011101001011010001101
#define XSP_REG6_WRITE_FRAC 0x5441100e /*ISOURCE_TRIM = 100 - optimal may be different*/



class TRF3765 {
    private:
      regmap_t _rm;
      uint32_t _rise;
      double fref;  //MHZ
      uint32_t _request(uint32_t x);
      
      void _send(uint32_t x);
      void g2_regmap(double freq, regmap_t &rm);
      void g3_regmap(double freq, bool fractional, bool calibrate, regmap_t &rm);
      void program(regmap_t &rm);
      void default_regmap(double freq, bool fractional, regmap_t &rm);
      void compute_nfrac_nint(double freq, bool fractional, regmap_t &rm);
      bool validate_regmap(double freq, regmap_t &rm);

    public:
      //bool isfractional(); r4.en_frac
      TRF3765();
      regmap_t get_config();
//      void tell_reg(genericreg_t r);
//      void tell_regmap(regmap_t &rm);
//      void tell_r0(reg0_t r);
//      void tell_r1(reg1_t r);
//      void tell_r2(reg2_t r);
//      void tell_r3(reg3_t r);
//      void tell_r4(reg4_t r);
//      void tell_r5(reg5_t r);
//      void tell_r6(reg6_t r);
//      void tell_rX(uint8_t x, uint32_t val);
      bool locked();
      void power_off();
      void set_rise(uint32_t rise);


      double f_PFD(regmap_t &rm);
      double cal_clock(regmap_t &rm);
      double f_RFSTEP(regmap_t &rm);
      double f_NMAX(double freq, regmap_t &rm);
      void self_test(double freq, bool fractional);
      bool set_fref(double frequency);
      bool set_freq(double frequency, bool fractional, bool calibrate, bool gen2);
      double get_freq();
      double get_freq(regmap_t &rm);
      void tell_status();
      void tell_status(regmap_t &rm);
      uint32_t get_register(uint8_t id);
      void set_register(uint8_t id, uint32_t reg);
      void reset(bool fractionl);
};
#endif
