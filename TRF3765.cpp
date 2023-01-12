#include <ArduinoJson.h>
#include "MemoryFree.h"
#include "TRF3765.h"
#include <SPI.h>
#include "pins.h"


uint8_t factor_2_cal_clk_sel(float factor){
  uint8_t ret=0, f=0, v=0, n=0;
  bool lt1=false;
  if (factor<=1) {
    lt1=true;
    factor=1/factor;
    ret|=0b1000;
  }
  f = round(factor);
  v=f;
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v++; // next power of 22
  v=(v - f) > (f - (v >> 1)) ? (v >> 1) : v;
  f=v;
  while (v >>= 1) ++n;
  if (lt1) ret|=n;
  else ret|=7-n;
 return ret;
}


bool TRF3765::set_freq(double freq, bool fractional, bool calibrate, bool gen2, bool nudge) {
// freq is in MHz and is doubled, fractional and calibrate have no impact in gen2 mode

//After programming, then set bit r2.en_cal to initiate calibration
// see pg. 23 for details of chips calibration process per the ~5 calibration related settings
// cal completes when pll is locked
// then you read trim and vco from reg0
// see r_sat_err for calibration overflow

//alternatively its possible to simply tell the device which vco and capacitor to use
//read vco_sel_mode from reg2 if 1 then
//vco_sel and vcotrim values set with reg 2 & 6 are used but
//must be read back with reg0
//note cal_bypass from reg6 matters


    regmap_t rm=REGMAP();
    freq /= 2; // Output frequency is doubled on the PCB)
    fractional|=gen2;
    default_regmap(freq, fractional, rm);

    if (nudge) {
      rm=_rm;
      compute_nfrac_nint(freq, fractional, rm);
    } else if (gen2) {
//        Serial.print(F("#Gen2"));
        g2_regmap(freq, rm);
        //reset(fractional);
    } else {
//        Serial.print(F("#Gen3"));
//        if (calibrate) Serial.print(F(" calibrate"));
        g3_regmap(freq, fractional, calibrate, rm);
    }
//    if (fractional) Serial.print(F(" fractional mode"));
    Serial.print('\n');
    tell_status(rm);
    if (!validate_regmap(freq, rm)) {
      return false;
    }
    program(rm);
    return true;
}

void TRF3765::default_regmap(double freq, bool fractional, regmap_t &rm) {

  //lower lo dividers improve phase noise performance
  //smaller pll dividers improve performace
  // See plots f_PFD=f_RFSTEP *lo_div/pll_div = f_VCOSTEP/pll_div
  // integer mode want max f_PFD possible to a target RF step
  // fractional mode rf step comes via fractional mode divider "A large
  // fPFD should be used to minimize the effects of fractional controller
  // noise in the output spectrum. In this case, fPFD may vary according
  // to the reference clock and fractional spur requirements" (p26)

  uint8_t loDivSel, pllDiv;

  if(freq>=2400){
    loDivSel = 0;
  } else if(freq>=1200) {
    loDivSel = 1;
  } else if(freq>=600) {
    loDivSel = 2;
  } else if(freq>=300) {
    loDivSel = 3;
  }

  rm.regs.r1=(reg1_t)REG_FROM_RAW(1, 0b01101001010100000000000000001001);
//  if (fractional)rm.regs.r2=(reg2_t)REG_FROM_RAW(2, 0x8800000A);
//  else rm.regs.r2=(reg2_t)REG_FROM_RAW(2, 0x8000000A);
  //NB gen2 put rm.regs.r2.field.vco_sel=2 if in fractional mode but
  // DID NOT set vcosel_mode so the setting was ignored
  rm.regs.r2.field.en_cal=true;
  rm.regs.r6=(reg6_t)REG_FROM_RAW(6, XSP_REG6_WRITE);

  rm.regs.r6.field.lo_div_sel=loDivSel; //NB fVCO = (1<<LO_DIV_SEL) * freq;

  // For our cases plldiv will be either LO < 6GHz ? 1:2
  // also known as rf divider in pdf
  pllDiv = (uint8_t) ceil(((double)(1<<rm.regs.r6.field.lo_div_sel))*freq/3000.0);

  rm.regs.r2.field.pll_div_sel=pllDiv>>1;
}

void TRF3765::g2_regmap(double freq, regmap_t &rm) {
//freq is desired output of trf chip in MHz

  rm.regs.r1.field.rdiv=1;
  compute_nfrac_nint(freq, true, rm);
  rm.regs.r4=(reg4_t)REG_FROM_RAW(4,XSP_REG4_WRITE_FRAC);
  rm.regs.r5=(reg5_t)REG_FROM_RAW(5,XSP_REG5_WRITE_FRAC);
}

void TRF3765::g3_regmap(double freq, bool fractional, bool calibrate, regmap_t &rm) {
//freq is desired output of trf chip in MHz

  // Calculate parameter and register values
//  f_RFSTEP = 1e-6;
//  double f_pfd = f_RFSTEP * (double)(1<<rm.regs.r6.field.lo_div_sel) / (double)(1<<rm.regs.r2.field.pll_div_sel);
  rm.regs.r1.field.rdiv=1; //fref/f_pfd
  compute_nfrac_nint(freq, fractional, rm);


  /*
  * for fREF=10MHz this constraint implies scale >=/32 at rdiv=1 and max rdiv=25600
  * but max rdiv is 8191 as 13bits (and rdiv probably doesn't need to exceed 9 anyway)
  * per p31
  */
  rm.regs.r1.field.cal_clk_sel=factor_2_cal_clk_sel(CAL_CLOCK_FREQ_MHZ/f_PFD(rm)); //default=0b1000, no scaling=FREF/rdiv
  rm.regs.r2.field.en_cal=true;
  rm.regs.r2.field.cal_acc=0;
  rm.regs.r6.field.cal_bypass=false;
  rm.regs.r2.field.vco_sel_mode=!calibrate;
  rm.regs.r2.field.vco_sel=3; //highest freq vco
  rm.regs.r4.field.pwd_buff2=true; //power down unused buffer
  rm.regs.r4.field.pwd_buff3=true; //power down unused buffer
  rm.regs.r4.field.pwd_buff4=true; //power down unused buffer
  rm.regs.r5.field.vcobias_rtirm=4;
  rm.regs.r5.field.pllbias_rtrim=2;
  rm.regs.r5.field.vco_bias=5;
  rm.regs.r5.field.vcobuf_bias=2;
  rm.regs.r5.field.vcomux_bias=2;
  rm.regs.r5.field.bufout_bias=3;
  rm.regs.r5.field.vco_cal_ref=1;
  rm.regs.r5.field.vco_vb_ctrl=1;



  rm.regs.r4.field.en_frac=fractional;
  if (fractional) {
    //per pg 22
    rm.regs.r4.field.ld_dig_prec=false;
    rm.regs.r4.field.ld_ana_prec=3;
    //per pg 28
    rm.regs.r4.field.en_isource=true;
    rm.regs.r4.field.en_dith=true;
    rm.regs.r4.field.dith_sel=false;
    rm.regs.r4.field.del_sd_clk=2;
    rm.regs.r5.field.en_ld_isource=false;
    rm.regs.r1.field.icpdouble=false;
    // Optimal performance may require tuning the
    // MOD_ORD, ISOURCE_SINK, and ISOURCE_TRIM values according to the chosen frequency band.
    rm.regs.r4.field.mod_ord=2;
    rm.regs.r6.field.isource_sink=false;
    rm.regs.r6.field.isource_trim=4;  //or 7, isouce bias current (nb en_isource=true)
  } else {
    //per pg 22
    rm.regs.r4.field.ld_dig_prec=false;
    rm.regs.r4.field.ld_ana_prec=0;
  }

  rm.regs.r4.field.en_extvco=false;
  rm.regs.r4.field.speedup=false;
}

void TRF3765::compute_nfrac_nint(double freq, bool fractional, regmap_t &rm) {
// freq is output freq of trf chip in MHz
// must have valid r1.field.rdiv, r6.field.lo_div_sel, and r2.field.pll_div_sel
// computes r2.field.prescale_sel, r2.field.nint, and r3.field.nfrac
  double frac_tmp;
  frac_tmp=freq*(double)rm.regs.r1.field.rdiv*(double)(1<<rm.regs.r6.field.lo_div_sel)/(fref*(double)(1<<rm.regs.r2.field.pll_div_sel));

  rm.regs.r2.field.nint=frac_tmp;
  rm.regs.r3.field.nfrac=(frac_tmp-rm.regs.r2.field.nint)*(double)(((uint32_t)1)<<25);
  // prscSel==0 ? 4/5ths : 8/9ths; limit 72 in integer mode, 75 in frac
  rm.regs.r2.field.prescale_sel=rm.regs.r2.field.nint>=(fractional ? 75:72);
}

bool TRF3765::validate_regmap(double freq, regmap_t &rm) {
  bool fail=true;
  if (freq<300 || freq >4800) {
    Serial.println(F("ERROR: Frequency out of range."));
    fail=false;
  }
  if (rm.regs.r2.field.pll_div_sel>2) {
    Serial.println(F("ERROR: Invalid PLL_DIV!"));
    fail=false;
  }
  if (rm.regs.r2.field.nint < (rm.regs.r4.field.en_frac ? 23:20)) {
    Serial.println(F("ERROR: nint too low, see pg26!"));
    fail=false;
  }
  if (f_NMAX(freq, rm) > 375){
    Serial.println(F("ERROR: Digital divider freq. (fN) to high!"));
    fail=false;
  }
  double cc = cal_clock(rm);
  if(.05 > cc || cc > .6) {
    Serial.println(("ERROR: Slow cal_clock by increasing rdiv, CAL_CLK_SEL, or lowering Fref"));
    fail=false;
  }
  return fail;
}

void TRF3765::program(regmap_t &rm) {
    //Note that gen2 wrote XSP_REG0_WRITE before these writes
    _send(rm.regs.r1.raw);
    _send(rm.regs.r3.raw);
    _send(rm.regs.r4.raw);
    _send(rm.regs.r5.raw);
    _send(rm.regs.r6.raw);
    _send(rm.regs.r2.raw);   //must write reg2 last as it kicks off the calibration when en_cal is true
    _rm=rm;
}

//Gets LO value
double TRF3765::get_freq() {
  //must have current regmap at _rm
  return get_freq(_rm);
}

double TRF3765::get_freq(regmap_t &rm) {

  //F_PFD = fref/rm.regs.r1.field.rdiv;
  //F_PFD //phase frequnecy detector frequency
  double fvco;
  double pll_divider = (1<<rm.regs.r2.field.pll_div_sel);
  if (rm.regs.r4.field.en_frac) {
    fvco = fref*pll_divider*((double)rm.regs.r2.field.nint+(double)rm.regs.r3.field.nfrac/(double)(((uint32_t) 1)<<25))/(double)rm.regs.r1.field.rdiv;
  } else {
    fvco = fref*pll_divider*(double)rm.regs.r2.field.nint/(double)rm.regs.r1.field.rdiv;
  }
  return 2.0*fvco/(double)(1<<rm.regs.r6.field.lo_div_sel);
}

void TRF3765::reset(bool fractional=true) {
  regmap_t init_data=REGMAP();
//  init_data.regs.r0=(reg0_t) REG_FROM_RAW(0,XSP_REG0_WRITE);
  init_data.regs.r1=(reg1_t) REG_FROM_RAW(1,XSP_REG1_WRITE);
  init_data.regs.r4=(reg4_t) REG_FROM_RAW(4,XSP_REG4_WRITE);

  if (fractional) {
      init_data.regs.r2=(reg2_t) REG_FROM_RAW(2, XSP_REG2_WRITE_ENC);
      init_data.regs.r5=(reg5_t) REG_FROM_RAW(5, XSP_REG5_WRITE_FRAC);
      init_data.regs.r6=(reg6_t) REG_FROM_RAW(6, XSP_REG6_WRITE_FRAC);
  } else {
      init_data.regs.r2=(reg2_t) REG_FROM_RAW(2, XSP_REG2_WRITE);
      init_data.regs.r5=(reg5_t) REG_FROM_RAW(5, XSP_REG5_WRITE);
      init_data.regs.r6=(reg6_t) REG_FROM_RAW(6, XSP_REG6_WRITE);
  }

  Serial.println(F("#Init registers"));
  tell_status(init_data);

  for (int i=1;i<7;i++) _send(init_data.data[i]);
}

void TRF3765::tell_status() {
  for (int i=0;i<7;i++)
    _rm.data[i]=_request(RB_REQUEST(i).raw);
  tell_status(_rm);
}

//void TRF3765::tell_status(regmap_t &rm) {

//  Serial.print("en_frac=");
//  if (rm.regs.r4.field.en_frac) Serial.println(F("true"));
//  else Serial.println(F("false"));
//  Serial.print("vco_sel_mode=");
//  if (rm.regs.r2.field.vco_sel_mode) Serial.println(F("true"));
//  else Serial.println(F("false"));
//  Serial.print("f_REF=");Serial.print(fref);Serial.println(" MHz");
//  Serial.print("f_PFD=");Serial.print(f_PFD(rm));Serial.println(" MHz");
//  Serial.print("f_calclk=");Serial.print(cal_clock(rm));Serial.println(" MHz");
//  Serial.print("df_RF=");Serial.print(2e6*f_RFSTEP(rm));Serial.println(" Hz");  //freq. doubler
//  Serial.print("f_LO=");Serial.print(get_freq(rm), 3);Serial.println(" MHz");
//  Serial.print(F("pll_locked="));
//  if (locked()) Serial.print(F("true"));
//  else Serial.print(F("false"));
//  Serial.print(" (pin=");Serial.print(digitalRead(PIN_LOCKED));
//  Serial.print(" rise=");Serial.print(_rise);Serial.println(")");
//  tell_regmap(rm);
//
//
////  Serial.print("Reg7: 0x");Serial.println(_request(RB_REQUEST(7).raw),HEX);
//}


void TRF3765::tell_status(regmap_t &rm) {

    StaticJsonDocument<512> doc;

    doc[F("f_REF")]=fref;
    doc[F("f_PFD")]=f_PFD(rm);
    doc[F("f_calclk")]=cal_clock(rm);
    doc[F("df_RF")]=2*f_RFSTEP(rm);  //freq. doubler
    doc[F("f_LO")]=get_freq(rm);
    doc[F("pll_locked")]=locked();
    Serial.print("[\n");
    serializeJson(doc, Serial);Serial.println(",");
    doc.clear();
    JsonObject r0=doc.createNestedObject(F("r0"));
    r0[F("raw")]=rm.regs.r0.raw;
    r0[F("chip_id")]=rm.regs.r0.field.chip_id;
    r0[F("r_sat_err")]=rm.regs.r0.field.r_sat_err;
    r0[F("vco_trim")]=(uint16_t)rm.regs.r0.field.vco_trim;
    r0[F("vco_sel")]=(uint16_t)rm.regs.r0.field.vco_sel;
    r0[F("count_mode_mux_sel")]=rm.regs.r0.field.count_mode_mux_sel;
    r0[F("count")]=rm.regs.r0.tst_mode.count;
    serializeJson(doc, Serial);Serial.println(",");
    doc.clear();
    JsonObject r1 = doc.createNestedObject(F("r1"));
    r1[F("raw")]=rm.regs.r1.raw;
    r1[F("rdiv")]=rm.regs.r1.field.rdiv;
    r1[F("invert_ref_clock")]=rm.regs.r1.field.invert_ref_clock;
    r1[F("neg_vco")]=rm.regs.r1.field.neg_vco;
    r1[F("icp")]=(uint16_t)rm.regs.r1.field.icp;
    r1[F("icpdouble")]=rm.regs.r1.field.icpdouble;
    r1[F("cal_clk_sel")]=(uint16_t)rm.regs.r1.field.cal_clk_sel;
    serializeJson(doc, Serial);Serial.println(",");
    doc.clear();
    JsonObject r2 = doc.createNestedObject(F("r2"));
    r2[F("raw")]=rm.regs.r2.raw;
    r2[F("nint")]=rm.regs.r2.field.nint;
    r2[F("pll_div_sel")]=(uint16_t)rm.regs.r2.field.pll_div_sel;
    r2[F("prescale_sel")]=rm.regs.r2.field.prescale_sel;
    r2[F("vco_sel")]=(uint16_t)rm.regs.r2.field.vco_sel;
    r2[F("vcosel_mode")]=rm.regs.r2.field.vco_sel_mode;
    r2[F("cal_acc")]=(uint16_t)rm.regs.r2.field.cal_acc;
    r2[F("en_cal")]=rm.regs.r2.field.en_cal;
    serializeJson(doc, Serial);Serial.println(",");
    doc.clear();
    JsonObject r3 = doc.createNestedObject(F("r3"));
    r3[F("raw")]=rm.regs.r3.raw;
    r3[F("nfrac")]=rm.regs.r3.field.nfrac;
    serializeJson(doc, Serial);Serial.println(",");
    doc.clear();
    JsonObject r4 = doc.createNestedObject(F("r4"));
    r4[F("raw")]=rm.regs.r4.raw;
    r4[F("pwd_pll")]=rm.regs.r4.field.pwd_pll;
    r4[F("pwd_cp")]=rm.regs.r4.field.pwd_cp;
    r4[F("pwd_vco")]=rm.regs.r4.field.pwd_vco;
    r4[F("pwd_vcomux")]=rm.regs.r4.field.pwd_vcomux;
    r4[F("pwd_rfdiv")]=rm.regs.r4.field.pwd_rfdiv;
    r4[F("pwd_presc")]=rm.regs.r4.field.pwd_presc;
    r4[F("pwd_lodiv")]=rm.regs.r4.field.pwd_lodiv;
    r4[F("pwd_buff1")]=rm.regs.r4.field.pwd_buff1;
    r4[F("pwd_buff2")]=rm.regs.r4.field.pwd_buff2;
    r4[F("pwd_buff3")]=rm.regs.r4.field.pwd_buff3;
    r4[F("pwd_buff4")]=rm.regs.r4.field.pwd_buff4;
    r4[F("en_extvco")]=rm.regs.r4.field.en_extvco;
    r4[F("ext_vco_ctrl")]=rm.regs.r4.field.ext_vco_ctrl;
    r4[F("en_isource")]=rm.regs.r4.field.en_isource;
    r4[F("ld_ana_prec")]=(uint16_t)rm.regs.r4.field.ld_ana_prec;
    r4[F("cp_tristate")]=(uint16_t)rm.regs.r4.field.cp_tristate;
    r4[F("speedup")]=rm.regs.r4.field.speedup;
    r4[F("ld_dig_prec")]=rm.regs.r4.field.ld_dig_prec;
    r4[F("en_dith")]=rm.regs.r4.field.en_dith;
    r4[F("mod_ord")]=(uint16_t)rm.regs.r4.field.mod_ord;
    r4[F("dith_sel")]=rm.regs.r4.field.dith_sel;
    r4[F("del_sd_clk")]=(uint16_t)rm.regs.r4.field.del_sd_clk;
    r4[F("en_frac")]=rm.regs.r4.field.en_frac;
    serializeJson(doc, Serial);Serial.println(",");
    doc.clear();
    JsonObject r5 = doc.createNestedObject(F("r5"));
    r5[F("raw")]=rm.regs.r5.raw;
    r5[F("vcobias_rtirm")]=(uint16_t)rm.regs.r5.field.vcobias_rtirm;
    r5[F("pllbias_rtrim")]=(uint16_t)rm.regs.r5.field.pllbias_rtrim;
    r5[F("vco_bias")]=(uint16_t)rm.regs.r5.field.vco_bias;
    r5[F("vcobuf_bias")]=(uint16_t)rm.regs.r5.field.vcobuf_bias;
    r5[F("vcomux_bias")]=(uint16_t)rm.regs.r5.field.vcomux_bias;
    r5[F("bufout_bias")]=(uint16_t)rm.regs.r5.field.bufout_bias;
    r5[F("vco_cal_ib")]=rm.regs.r5.field.vco_cal_ib;
    r5[F("vco_cal_ref")]=(uint16_t)rm.regs.r5.field.vco_cal_ref;
    r5[F("vco_ampl_ctrl")]=(uint16_t)rm.regs.r5.field.vco_ampl_ctrl;
    r5[F("vco_vb_ctrl")]=(uint16_t)rm.regs.r5.field.vco_vb_ctrl;
    r5[F("en_ld_isource")]=rm.regs.r5.field.en_ld_isource;
    serializeJson(doc, Serial);Serial.println(",");
    doc.clear();
    JsonObject r6 = doc.createNestedObject(F("r6"));
    r6[F("raw")]=rm.regs.r6.raw;
    r6[F("vco_trim")]=(uint16_t)rm.regs.r6.field.vco_trim;
    r6[F("en_lockdet")]=rm.regs.r6.field.en_lockdet;
    r6[F("vco_test_mode")]=rm.regs.r6.field.vco_test_mode;
    r6[F("cal_bypass")]=rm.regs.r6.field.cal_bypass;
    r6[F("mux_ctrl")]=(uint16_t)rm.regs.r6.field.mux_ctrl;
    r6[F("isource_sink")]=rm.regs.r6.field.isource_sink;
    r6[F("isource_trim")]=(uint16_t)rm.regs.r6.field.isource_trim;
    r6[F("lo_div_sel")]=(uint16_t)rm.regs.r6.field.lo_div_sel;
    r6[F("lo_div_ib")]=(uint16_t)rm.regs.r6.field.lo_div_ib;
    r6[F("div_mux_ref")]=(uint16_t)rm.regs.r6.field.div_mux_ref;
    r6[F("div_mux_out")]=(uint16_t)rm.regs.r6.field.div_mux_out;
    r6[F("div_mux_bias_ovrd")]=rm.regs.r6.field.div_mux_bias_ovrd;
    serializeJson(doc, Serial);Serial.print("]");
}


void TRF3765::set_rise(uint32_t rise) {
  _rise=rise;
}

TRF3765::TRF3765() {
  fref=DEFAULT_FREF;
}

regmap_t TRF3765::get_config() {
  return _rm;
}

uint32_t TRF3765::get_register(uint8_t address) {
  if (address > 6)
    return 0;

  uint32_t ret;

  reg0_t reg=RB_REQUEST(address);
//  tell_r0(reg);
  ret = _request(reg.raw);
//  tell_rX(address, ret);
  return ret;
}

//Sets contents of register address, no safety check performed, read datasheet
// low 5 bits will be overwritten with appropriate address
void TRF3765::set_register(uint8_t address, uint32_t x) {
  if (address > 6) return;
  genericreg_t r=REG_FROM_RAW(address, x);
  _send(r.raw);
  uint32_t rb = get_register(address);
  if (rb!=r.raw) {

    Serial.println("Mismatch. Sent ");
    Serial.print(r.raw, HEX);
    Serial.print(" got:");
    Serial.println(rb, HEX);
  }

  _rm.data[address]=r.raw;
}

uint32_t TRF3765::_request(uint32_t x) {
  SPI.beginTransaction(SPISettings(TRF_SPI_FREQ, LSBFIRST, SPI_MODE0));
  SPI.transfer((uint8_t *)&x, 4); //32 write clks
  SPI.endTransaction();
  digitalWrite(PIN_SS_LO, HIGH);

  SPI.end();

  digitalWrite(PIN_CLK, HIGH);
  digitalWrite(PIN_CLK, LOW);

/*
 * In the proper reading phase, at each rising clock edge,
 * the internal data are transferred to the READBACK pin where
 * it can be read at the following falling edge (LSB first).
 * The first clock after the latch enable PIN_SS_LO goes
 * high (that is, the end of the write cycle) is idle and
 * the following 32 clock pulses transfer the internal
 * register contents.
 */
  digitalWrite(PIN_SS_LO, LOW);
  x=0;
  SPI.beginTransaction(SPISettings(TRF_SPI_FREQ, LSBFIRST, SPI_MODE1));
  SPI.transfer((uint8_t *)&x, 4);
  SPI.endTransaction();
  SPI.end();
  return x;
}

void TRF3765::_send(uint32_t x) {
  SPI.beginTransaction(SPISettings(TRF_SPI_FREQ, LSBFIRST, SPI_MODE0));
  SPI.transfer((uint8_t *)&x, 4);
  //20ns delay for free as arduino runs at <= 8MHz
  digitalWrite(PIN_SS_LO, HIGH);
  //50ns delay free
  digitalWrite(PIN_SS_LO, LOW);
  SPI.endTransaction();
  SPI.end();
}


bool TRF3765::locked() {
//  return (millis()-_rise > LOCK_TIME_MS);

  uint32_t tic=millis();
  while (millis()-tic < LOCK_TIME_MS)
    if (!digitalRead(PIN_LOCKED))
      return false;
  return true;
}

bool TRF3765::set_fref(double freq) {
  fref=freq;
  return true;
}

void TRF3765::power_off() {

}

void TRF3765::self_test(double freq, bool fractional) {
  regmap_t rm;

  for (int i=0;i<4;i++ ) { //each vco
    rm.regs.r2.field.vco_sel=i;
    for (int j=0;j<2;j++) { //min=0  max=1
      Serial.print("#vco=");Serial.print(i);
      rm.regs.r6.field.vco_test_mode=true;
      rm.regs.r2.field.en_cal=true;
      //write regs and wait
      for(int i=0;i<7;i++) _send(rm.data[i]);

      delay(5);  //calibration takes a max of 4.2ms

      //read reg0
      reg0_t rbreq=RB_REQUEST(0);
      rbreq.rb_mode.count_mode_mux_sel=j;
      rbreq.raw = _request(rbreq.raw);
      if (j) Serial.print(F(" max_"));
      else Serial.print(F(" min_"));
      Serial.print(F("count="));Serial.print(rbreq.tst_mode.count);
      Serial.print(F(" sat_err=")); Serial.println(rbreq.field.r_sat_err);
    }
  }

  rm.regs.r6.field.vco_test_mode=false;
  rm.regs.r2.field.en_cal=true;
  //write regs and wait
  for(int i=0;i<7;i++) _send(rm.data[i]);

  delay(5);
  //read reg0
  reg0_t rbreq=RB_REQUEST(0);
  rbreq.raw = _request(rbreq.raw);

  Serial.print(F("#Result: sat_err="));Serial.print(rbreq.field.r_sat_err);
  Serial.print(F(" vco_trim="));Serial.print((uint16_t) rbreq.field.vco_trim);
  Serial.print(F(" vco_sel="));Serial.println((uint16_t) rbreq.field.vco_sel);


}

double TRF3765::f_PFD(regmap_t &rm) {
  return fref/rm.regs.r1.field.rdiv;
}

double TRF3765::cal_clock(regmap_t &rm) {
  double f_pfd = f_PFD(rm);
  if (rm.regs.r1.field.cal_clk_sel & 0b1000)
    return f_pfd/(1<<(rm.regs.r1.field.cal_clk_sel&7));
  else
    return f_pfd*(1<<(7-rm.regs.r1.field.cal_clk_sel));
}


double TRF3765::f_RFSTEP(regmap_t &rm) {
  double step=f_PFD(rm)*(1<<rm.regs.r2.field.pll_div_sel)/(double) (1<<rm.regs.r6.field.lo_div_sel);
  if (rm.regs.r4.field.en_frac)
    step/=((uint32_t)1)<<25;
  return step;
}

double TRF3765::f_NMAX(double freq, regmap_t &rm) {
  return (1<<rm.regs.r6.field.lo_div_sel) * freq/(1<<rm.regs.r2.field.pll_div_sel)/ (rm.regs.r2.field.prescale_sel ? 8.0:4.0);
}


//void TRF3765::tell_regmap(regmap_t &rm) {
//  tell_r0(rm.regs.r0);
//  tell_r1(rm.regs.r1);
//  tell_r2(rm.regs.r2);
//  tell_r3(rm.regs.r3);
//  tell_r4(rm.regs.r4);
//  tell_r5(rm.regs.r5);
//  tell_r6(rm.regs.r6);
//}

//void TRF3765::tell_reg(genericreg_t r){
//  Serial.print(F("Reg:\n"));
//  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
//  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
//  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
//  Serial.print(F("\tdata: "));Serial.println(r.data.val);
//}

//void TRF3765::tell_rX(uint8_t x, uint32_t val){
//  genericreg_t r={.raw=val};
//  switch (x) {
//    case 0:
//      tell_r0(*((reg0_t*)&r));
//      break;
//    case 1:
//      tell_r1(*((reg1_t*)&r));
//      break;
//    case 2:
//      tell_r2(*((reg2_t*)&r));
//      break;
//    case 3:
//      tell_r3(*((reg3_t*)&r));
//      break;
//    case 4:
//      tell_r4(*((reg4_t*)&r));
//      break;
//    case 5:
//      tell_r5(*((reg5_t*)&r));
//      break;
//    case 6:
//      tell_r6(*((reg6_t*)&r));
//      break;
//  }
//}

//void TRF3765::tell_r0(reg0_t r) {
//  Serial.print(F("Reg0:\n"));
//  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
//  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
//  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
//  Serial.print(F("\tchip_id: "));Serial.println(r.field.chip_id);
//  Serial.print(F("\tr_sat_err: "));Serial.println(r.field.r_sat_err);
//  Serial.print(F("\tvco_trim: "));Serial.println((uint16_t)r.field.vco_trim);
//  Serial.print(F("\tvco_sel: "));Serial.println((uint16_t)r.field.vco_sel);
//  Serial.print(F("\tcount_mode_mux_sel: "));Serial.println(r.field.count_mode_mux_sel);
//  Serial.print(F("\tcount (tst): "));Serial.println(r.tst_mode.count);
//  Serial.print(F("\tcount_mode_mux_sel (rb): "));Serial.println(r.rb_mode.count_mode_mux_sel);
//  Serial.print(F("\treg (rb): "));Serial.println((uint16_t)r.rb_mode.reg);
//  Serial.print(F("\trb_enable (rb): "));Serial.println(r.rb_mode.rb_enable);
//}
//
//
//void TRF3765::tell_r1(reg1_t r) {
//  Serial.print(F("Reg1:\n"));
//  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
//  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
//  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
//  Serial.print(F("\trdiv: "));Serial.println(r.field.rdiv);
//  Serial.print(F("\tinvert_ref_clock: "));Serial.println(r.field.invert_ref_clock);
//  Serial.print(F("\tneg_vco: "));Serial.println(r.field.neg_vco);
//  Serial.print(F("\ticp: "));Serial.println((uint16_t) r.field.icp);
//  Serial.print(F("\ticpdouble: "));Serial.println(r.field.icpdouble);
//  Serial.print(F("\tcal_clk_sel: "));Serial.println((uint16_t)r.field.cal_clk_sel);
//}
//
//void TRF3765::tell_r2(reg2_t r) {
//  Serial.print(F("Reg2:\n"));
//  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
//  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
//  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
//  Serial.print(F("\tnint: "));Serial.println((uint16_t) r.field.nint);
//  Serial.print(F("\tpll_div_sel: "));Serial.println((uint16_t) r.field.pll_div_sel);
//  Serial.print(F("\tprescale_sel: "));Serial.println(r.field.prescale_sel);
//  Serial.print(F("\tvco_sel: "));Serial.println((uint16_t) r.field.vco_sel);
//  Serial.print(F("\tvcosel_mode: "));Serial.println(r.field.vco_sel_mode);
//  Serial.print(F("\tcal_acc: "));Serial.println((uint16_t) r.field.cal_acc);
//  Serial.print(F("\ten_cal: "));Serial.println(r.field.en_cal);
//}
//
//void TRF3765::tell_r3(reg3_t r) {
//  Serial.print(F("Reg3:\n"));
//  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
//  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
//  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
//  Serial.print(F("\tnfrac: "));Serial.println(r.field.nfrac);
//}
//
//void TRF3765::tell_r4(reg4_t r) {
//  Serial.print(F("Reg4:\n"));
//  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
//  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
//  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
//  Serial.print(F("\tpwd_pll: "));Serial.println(r.field.pwd_pll);
//  Serial.print(F("\tpwd_cp: "));Serial.println(r.field.pwd_cp);
//  Serial.print(F("\tpwd_vco: "));Serial.println(r.field.pwd_vco);
//  Serial.print(F("\tpwd_vcomux: "));Serial.println(r.field.pwd_vcomux);
//  Serial.print(F("\tpwd_rfdiv: "));Serial.println(r.field.pwd_rfdiv);
//  Serial.print(F("\tpwd_presc: "));Serial.println(r.field.pwd_presc);
//  Serial.print(F("\tpwd_lodiv: "));Serial.println(r.field.pwd_lodiv);
//  Serial.print(F("\tpwd_buff1: "));Serial.println(r.field.pwd_buff1);
//  Serial.print(F("\tpwd_buff2: "));Serial.println(r.field.pwd_buff2);
//  Serial.print(F("\tpwd_buff3: "));Serial.println(r.field.pwd_buff3);
//  Serial.print(F("\tpwd_buff4: "));Serial.println(r.field.pwd_buff4);
//  Serial.print(F("\ten_extvco: "));Serial.println(r.field.en_extvco);
//  Serial.print(F("\text_vco_ctrl: "));Serial.println(r.field.ext_vco_ctrl);
//  Serial.print(F("\ten_isource: "));Serial.println(r.field.en_isource);
//  Serial.print(F("\tld_ana_prec: "));Serial.println((uint16_t)r.field.ld_ana_prec);
//  Serial.print(F("\tcp_tristate: "));Serial.println((uint16_t)r.field.cp_tristate);
//  Serial.print(F("\tspeedup: "));Serial.println(r.field.speedup);
//  Serial.print(F("\tld_dig_prec: "));Serial.println(r.field.ld_dig_prec);
//  Serial.print(F("\ten_dith: "));Serial.println(r.field.en_dith);
//  Serial.print(F("\tmod_ord: "));Serial.println((uint16_t)r.field.mod_ord);
//  Serial.print(F("\tdith_sel: "));Serial.println(r.field.dith_sel);
//  Serial.print(F("\tdel_sd_clk: "));Serial.println((uint16_t)r.field.del_sd_clk);
//  Serial.print(F("\ten_frac: "));Serial.println(r.field.en_frac);
//}
//
//void TRF3765::tell_r5(reg5_t r) {
//  Serial.print(F("Reg5:\n"));
//  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
//  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
//  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
//  Serial.print(F("\tvcobias_rtirm: "));Serial.println((uint16_t) r.field.vcobias_rtirm);
//  Serial.print(F("\tpllbias_rtrim: "));Serial.println((uint16_t) r.field.pllbias_rtrim);
//  Serial.print(F("\tvco_bias: "));Serial.println((uint16_t) r.field.vco_bias);
//  Serial.print(F("\tvcobuf_bias: "));Serial.println((uint16_t) r.field.vcobuf_bias);
//  Serial.print(F("\tvcomux_bias: "));Serial.println((uint16_t) r.field.vcomux_bias);
//  Serial.print(F("\tbufout_bias: "));Serial.println((uint16_t) r.field.bufout_bias);
//  Serial.print(F("\tvco_cal_ib: "));Serial.println(r.field.vco_cal_ib);
//  Serial.print(F("\tvco_cal_ref: "));Serial.println((uint16_t) r.field.vco_cal_ref);
//  Serial.print(F("\tvco_ampl_ctrl: "));Serial.println((uint16_t) r.field.vco_ampl_ctrl);
//  Serial.print(F("\tvco_vb_ctrl: "));Serial.println((uint16_t) r.field.vco_vb_ctrl);
//  Serial.print(F("\ten_ld_isource: "));Serial.println(r.field.en_ld_isource);
//}
//
//void TRF3765::tell_r6(reg6_t r) {
//  Serial.print(F("Reg6:\n"));
//  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
//  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
//  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
//  Serial.print(F("\tvco_trim: "));Serial.println((uint16_t) r.field.vco_trim);
//  Serial.print(F("\ten_lockdet: "));Serial.println(r.field.en_lockdet);
//  Serial.print(F("\tvco_test_mode: "));Serial.println(r.field.vco_test_mode);
//  Serial.print(F("\tcal_bypass: "));Serial.println(r.field.cal_bypass);
//  Serial.print(F("\tmux_ctrl: "));Serial.println((uint16_t) r.field.mux_ctrl);
//  Serial.print(F("\tisource_sink: "));Serial.println(r.field.isource_sink);
//  Serial.print(F("\tisource_trim: "));Serial.println((uint16_t) r.field.isource_trim);
//  Serial.print(F("\tlo_div_sel: "));Serial.println((uint16_t) r.field.lo_div_sel);
//  Serial.print(F("\tlo_div_ib: "));Serial.println((uint16_t) r.field.lo_div_ib);
//  Serial.print(F("\tdiv_mux_ref: "));Serial.println((uint16_t) r.field.div_mux_ref);
//  Serial.print(F("\tdiv_mux_out: "));Serial.println((uint16_t) r.field.div_mux_out);
//  Serial.print(F("\tdiv_mux_bias_ovrd: "));Serial.println(r.field.div_mux_bias_ovrd);
//}
