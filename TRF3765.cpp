#include "TRF3765.h"
#include <SPI.h> 
#include "pins.h"
//Gets contents of register address

uint32_t rise=0;
void _lockISR() {
  rise=(PORTB & 0b10000)>0 ? millis():0;
}

TRF3765::TRF3765() {
  fref=DEFAULT_FREF;
  attachInterrupt(4, _lockISR, CHANGE);
}

uint32_t TRF3765::get_register(uint8_t address) {
  if (address > 6)
    return 0;

  uint32_t ret;

  reg0_t reg=RB_REQUEST(address);
  tell_r0(reg);
  ret = _request(reg.raw);
  tell_rX(address, ret);
  return ret;
}

void TRF3765::tell_rX(uint8_t x, uint32_t val){
  genericreg_t r={.raw=val};
  switch (x) {
    case 0:
      tell_r0(*((reg0_t*)&r));
      break;
    case 1:
      tell_r1(*((reg1_t*)&r));
      break;
    case 2:
      tell_r2(*((reg2_t*)&r));
      break;
    case 3:
      tell_r3(*((reg3_t*)&r));
      break;
    case 4:
      tell_r4(*((reg4_t*)&r));
      break;
    case 5:
      tell_r5(*((reg5_t*)&r));
      break;
    case 6:
      tell_r6(*((reg6_t*)&r));
      break;
  }
}

regmap_t TRF3765::get_config() {
  return _rm;
}

//Sets contents of register address, no safety check performed, read datasheet 
// low 5 bits will be overwritten with appropriate address
void TRF3765::set_register(uint8_t address, uint32_t x) {
  if (address > 6) return;
  genericreg_t r=REG_FROM_RAW(address, x);
  _send(r.raw);
  _rm.data[address]=r.raw;
}

uint32_t TRF3765::_request(uint32_t x) {
  SPI.beginTransaction(SPISettings(TRF_SPI_FREQ, LSBFIRST, SPI_MODE0));
  SPI.transfer((uint8_t *)&x, 4);
  SPI.endTransaction();
  digitalWrite(PIN_SS_LO, HIGH);

  SPI.end();

  digitalWrite(PIN_CLK, HIGH);
  digitalWrite(PIN_CLK, LOW);
//delayMicroseconds(10);
//  DDRB|=0b10;
//  PORTB|= 0b10;
//  delayMicroseconds(1);
//  PORTB&=~0b10;
//    delayMicroseconds(1);
  digitalWrite(PIN_SS_LO, LOW);
  x=0;
  SPI.beginTransaction(SPISettings(TRF_SPI_FREQ, LSBFIRST, SPI_MODE0));
  SPI.transfer((uint8_t *)&x, 4);
  SPI.endTransaction();
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
}


bool TRF3765::locked() {
//  return (millis()-rise > LOCK_TIME_MS);

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

void TRF3765::self_test(double freq, bool fractional) {
  regmap_t rm;
  
  for (int i=0;i<4;i++ ) { //each vco
    Serial.print("VCO");Serial.println(i);
    rm.regs.r2.field.vco_sel=i;
    for (int j=0;j<2;j++) { //min=0  max=1
      rm.regs.r6.field.vco_test_mode=true;
      rm.regs.r2.field.en_cal=true;
      //write regs and wait
      for(int i=0;i<7;i++) _send(rm.data[i]);

      delay(5);  //calibration takes a max of 4.2ms
     
      //read reg0
      reg0_t rbreq=RB_REQUEST(0);
      rbreq.rb_mode.count_mode_mux_sel=j;
      rbreq.raw = _request(rbreq.raw);
      if (j) Serial.print(F(" Max:")); 
      else Serial.print(F(" Min:")); 
      Serial.print(F(" sat_err=")); 
      Serial.print(rbreq.field.r_sat_err);
      Serial.print(F(" count="));Serial.println(rbreq.tst_mode.count);
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
  
  Serial.print(F("Calibration\n sat_err=")); 
  Serial.print(rbreq.field.r_sat_err);
  Serial.print(F(" vco_trim="));Serial.print((uint16_t) rbreq.field.vco_trim);
  Serial.print(F("\n vco_sel="));Serial.println((uint16_t) rbreq.field.vco_sel);

  
}

inline double TRF3765::f_PFD(regmap_t rm) {
  return fref/rm.regs.r1.field.rdiv;
}

inline double TRF3765::cal_clock(regmap_t rm) {
  double f_pfd = f_PFD(rm);
  if (rm.regs.r1.field.cal_clk_sel & 0b1000)
    return f_pfd/(1<<(rm.regs.r1.field.cal_clk_sel&7));
  else
    return f_pfd*(1<<(7-rm.regs.r1.field.cal_clk_sel));
}

inline uint8_t factor_2_cal_clk_sel(float factor){
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

  Serial.print("Cal Clock factor:");
  if (lt1) Serial.print("1/");
  Serial.print(factor);Serial.print(" (");
  Serial.print(f);Serial.print(", ");Serial.print(n);Serial.print(")=0b");Serial.println(ret&0b1111, BIN);
  return ret;
}


inline double TRF3765::f_RFSTEP(regmap_t rm) {
  double step=f_PFD(rm)*(1<<rm.regs.r2.field.pll_div_sel)/(double) (1<<rm.regs.r6.field.lo_div_sel);
  if (rm.regs.r4.field.en_frac) 
    step/=((uint32_t)1)<<25;
  return step;
}

inline double TRF3765::f_NMAX(double freq, regmap_t rm) {
  return (1<<rm.regs.r6.field.lo_div_sel) * freq/(1<<rm.regs.r2.field.pll_div_sel)/ (rm.regs.r2.field.prescale_sel ? 8.0:4.0);
}

bool TRF3765::set_freq(double freq, bool fractional, bool calibrate, bool gen2) {
//freq is in MHz

//gen 2 tells lo to use vco 0 and device picks trim cap

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

  uint8_t loDivSel, pllDiv, pllDivSel, prscSel;
  uint16_t rDiv, nInt;
  uint32_t nFrac;
  bool fail=false;
  regmap_t rm=REGMAP(), rm_g3;


  // Calculate parameter and register values
  rDiv = 1; //pdf suggests this should be Fref/FPDF FPFD is the rf step size for us this maybe should be the dac resolution of ~7.62kHz
  freq /= 2; // Output frequency is doubled on the PCB)
  
  if (freq<300 || freq >4800) {
    Serial.println(F("Frequency out of range."));
    return false;
  }

  //lower lo dividers improve phase noise performance
  //smaller pll dividers improve performace
  // See plots f_PFD=f_RFSTEP *lo_div/pll_div = f_VCOSTEP/pll_div
  // integer mode want max f_PFD possible to a target RF step
  // fractional mode rf step comes via fractional mode divider "A large 
  // fPFD should be used to minimize the effects of fractional controller 
  // noise in the output spectrum. In this case, fPFD may vary according
  // to the reference clock and fractional spur requirements" (p26)
  
  if(freq>=2400){
    loDivSel = 0;
  } else if(freq>=1200) {
    loDivSel = 1;
  } else if(freq>=600) {
    loDivSel = 2;
  } else if(freq>=300) {
    loDivSel = 3;
  } 
  
  pllDiv = (uint8_t) ceil(((double)(1<<loDivSel))*freq/3000.0);  //also known as rf divider in pdf
  // For our cases plldiv will be either LO < 6GHz ? 1:2

  //NB fVCO = (1<<LO_DIV_SEL) * freq; 
  double frac_tmp=freq*(double)rDiv*(double)(1<<loDivSel)/(fref*(double)pllDiv);
  nInt = (uint16_t) frac_tmp;
  nFrac = (frac_tmp-nInt)*(double)(1<<25);

  //prscSel==0 ? 4/5ths : 8/9ths
  prscSel = nInt>=(fractional ? 75:72);   // 72 in integer mode, 75 in frac

  pllDivSel = pllDiv>>1;

  if (nInt<(fractional ? 23:20)) {
    Serial.println(F("nint too low, see pg26! (g2)"));
    fail=gen2;
  }
  
  if (pllDivSel>2) {
    Serial.println(F("Invalid PLL_DIV!"));
    fail=true;
  }


  rm.regs.r1=(reg1_t)REG_FROM_RAW(1, 0b01101001010100000000000000001001);
  rm.regs.r1.field.rdiv=rDiv;
  
  rm.regs.r6=(reg6_t)REG_FROM_RAW(6, XSP_REG6_WRITE);
  rm.regs.r6.field.lo_div_sel=loDivSel;

  if (fractional) {
    rm.regs.r2=(reg2_t)REG_FROM_RAW(2, 0x8800000A);
    rm.regs.r3.field.nfrac=nFrac;
  }
  else 
    rm.regs.r2=(reg2_t)REG_FROM_RAW(2, 0x0800000A);
  rm.regs.r2.field.nint=nInt;
  rm.regs.r2.field.prescale_sel=prscSel;
  rm.regs.r2.field.pll_div_sel=pllDivSel;

  if (f_NMAX(freq, rm) > 375){
    Serial.println(F("Digital divider freq. (fN) to high! (g2)"));
    fail=gen2;
  }

//  Serial.println("GEN2REGS");
//  tell_status(rm);

  // GEN3 -------
  rm_g3=rm;
//  f_RFSTEP = ?;
//  double f_pfd = f_RFSTEP * (1<<loDivSel)/ (double)(1<<pllDivSel);
//  rm_g3.regs.r1.field.rdiv=fref/f_pfd;
  rm_g3.regs.r1.field.rdiv=1;

  frac_tmp=freq*(double)rm_g3.regs.r1.field.rdiv*(double)(1<<loDivSel)/(fref*(double)pllDiv);
  //prscSel==0 ? 4/5ths : 8/9ths
  prscSel = ((uint16_t) frac_tmp)>=(fractional ? 75:72);   // 72 in integer mode, 75 in frac

  if (((uint16_t)frac_tmp)<(fractional ? 23:20)) {
    Serial.println(F("nint too low, see pg26 (g3)!"));
    fail=!gen2;
  }


  rm_g3.regs.r2.field.nint=frac_tmp;
  rm_g3.regs.r3.field.nfrac=(frac_tmp-rm.regs.r2.field.nint)*(double)(1<<25);
  rm_g3.regs.r2.field.prescale_sel=prscSel;

  if (f_NMAX(freq, rm_g3) > 375){
    Serial.println(F("Digital divider freq. (fN) to high (g3)!"));
    fail=!gen2;
  }
  /*
  for fREF=10MHz this constraint implies scale >=/32 at rdiv=1 and max rdiv=25600
  //but max rdiv is 8191 as 13bits (and rdiv probably doesn't need to exceed 9 anyway)
  //per p31
  */
  //must write reg2 last as it kicks off the calibration when en_cal is true
  rm_g3.regs.r1.field.cal_clk_sel=factor_2_cal_clk_sel(.3/f_PFD(rm_g3)); //default=0b1000, no scaling=FREF/rdiv
  rm_g3.regs.r2.field.en_cal=true;
  rm_g3.regs.r2.field.cal_acc=0;
  rm_g3.regs.r6.field.cal_bypass=false;
  rm_g3.regs.r2.field.vco_sel_mode=!calibrate;
  rm_g3.regs.r2.field.vco_sel=3; //highest freq vco 
  
  if(.05 > cal_clock(rm) || cal_clock(rm) > .6) {
    Serial.println(F("Slow cal_clock by increasing rdiv, CAL_CLK_SEL, or lowering fref (g2)"));
    fail = gen2;
  }

  if(.05 > cal_clock(rm_g3) || cal_clock(rm_g3) > .6) {
    Serial.println(("Slow cal clock by increasing rdiv, CAL_CLK_SEL, or lowering fref (g3)"));
    fail = !gen2;
  }
  
  if (fractional) {
    //per pg 22
    rm_g3.regs.r4.field.ld_dig_prec=false;
    rm_g3.regs.r4.field.ld_ana_prec=3;
    //per pg 28
    //Optimal performance may require tuning the 
    // MOD_ORD, ISOURCE_SINK, and ISOURCE_TRIM 
    //values according to the chosen frequency band.
    rm_g3.regs.r4.field.en_isource=true;
    rm_g3.regs.r4.field.en_dith=true;
    rm_g3.regs.r4.field.mod_ord=2;
    rm_g3.regs.r4.field.dith_sel=false;
    rm_g3.regs.r4.field.del_sd_clk=2;
    rm_g3.regs.r4.field.en_frac=true;
    rm_g3.regs.r5.field.en_ld_isource=false;
    rm_g3.regs.r6.field.isource_sink=false;
    rm_g3.regs.r6.field.isource_trim=4;  //or 7, isouce bias current (as en_isource=true)
    rm_g3.regs.r1.field.icpdouble=false;
  } else {
    rm_g3.regs.r4.field.en_frac=false;
    //per pg 22
    rm_g3.regs.r4.field.ld_dig_prec=false;
    rm_g3.regs.r4.field.ld_ana_prec=0;
  }

  rm_g3.regs.r4.field.en_extvco=false;
  rm_g3.regs.r4.field.speedup=false;

  // GEN3 -------


  if (gen2) {
    Serial.print(F("Gen2"));
    if (fractional) Serial.println(F(" fractional mode")); 
//    reset(fractional);
    if (fractional) {
      rm.regs.r4=(reg4_t)REG_FROM_RAW(4,XSP_REG4_WRITE_FRAC);
      rm.regs.r5=(reg5_t)REG_FROM_RAW(5,XSP_REG5_WRITE_FRAC);
    } else {
      //Note that the microblaze gen2 code NEVER wrote the value it computed for r2 in integer mode!
      rm.regs.r4=(reg4_t)REG_FROM_RAW(4,XSP_REG4_WRITE);
      rm.regs.r5=(reg5_t)REG_FROM_RAW(5,XSP_REG5_WRITE);
      rm.regs.r2=(reg2_t)REG_FROM_RAW(2,XSP_REG2_WRITE_ENC);
    }
  } else {
    Serial.print(F("Gen3"));
    if (calibrate) Serial.print(F(" calibrate"));
    if (fractional) Serial.println(F(" fractional mode")); 
    rm=rm_g3;
  }
  
  
  tell_status(rm);
  //Note that gen2 wrote XSP_REG0_WRITE before these writes
  if (!fail) {
    _send(rm.regs.r1.raw);
    _send(rm.regs.r3.raw);
    _send(rm.regs.r4.raw);
    _send(rm.regs.r5.raw);
    _send(rm.regs.r6.raw);
    _send(rm.regs.r2.raw);
    _rm=rm;
  }
  return true;
}

//Gets LO value
double TRF3765::get_freq() {
  //must have current regmap at _rm
  return get_freq(_rm);
}

double TRF3765::get_freq(regmap_t rm) { 

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
  init_data.regs.r2=(reg2_t) REG_FROM_RAW(2,(fractional ? XSP_REG2_WRITE_ENC:XSP_REG2_WRITE));
  init_data.regs.r4=(reg4_t) REG_FROM_RAW(4,XSP_REG4_WRITE);
  init_data.regs.r5=(reg5_t) REG_FROM_RAW(5,(fractional ? XSP_REG5_WRITE_FRAC:XSP_REG5_WRITE));
  init_data.regs.r6=(reg6_t) REG_FROM_RAW(6,(fractional ? XSP_REG6_WRITE_FRAC:XSP_REG6_WRITE));

  Serial.println(F("#Init registers"));
  tell_status(init_data);
  
  for (int i=1;i<7;i++) _send(init_data.data[i]);
}

void TRF3765::tell_status() {
  for (int i=0;i<7;i++)
    _rm.data[i]=_request(RB_REQUEST(i).raw);
  tell_status(_rm);
}


void TRF3765::tell_status(regmap_t rm) {
  tell_regmap(rm);

  Serial.print("f_REF=");Serial.print(fref);Serial.println(" MHz");
  Serial.print("f_PFD=");Serial.print(f_PFD(rm));Serial.println(" MHz");
  Serial.print("f_calclock=");Serial.print(cal_clock(rm));Serial.println(" MHz");
  Serial.print("df_RF=");Serial.print(f_RFSTEP(rm)*1e6);Serial.println(" Hz");

  Serial.print("LO freq: ");Serial.print(get_freq(rm), 3);Serial.println(" MHz");
  if (locked()) Serial.print(F("LO locked"));
  else Serial.print(F("LO unlocked"));
  Serial.print("(pin=");Serial.print(digitalRead(PIN_LOCKED));
  Serial.print(" rise=");Serial.print(rise);Serial.println(")");

//  Serial.print("Reg7: 0x");Serial.println(_request(RB_REQUEST(7).raw),HEX);
}


void TRF3765::tell_regmap(regmap_t rm) {
  tell_r0(rm.regs.r0);
  tell_r1(rm.regs.r1);
  tell_r2(rm.regs.r2);
  tell_r3(rm.regs.r3);
  tell_r4(rm.regs.r4);
  tell_r5(rm.regs.r5);
  tell_r6(rm.regs.r6);
}

void TRF3765::tell_reg(genericreg_t r){
  Serial.print(F("Reg:\n"));
  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
  Serial.print(F("\tdata: "));Serial.println(r.data.val);
}

void TRF3765::tell_r0(reg0_t r) {
  Serial.print(F("Reg0:\n"));
  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
  Serial.print(F("\tchip_id: "));Serial.println(r.field.chip_id);
  Serial.print(F("\tr_sat_err: "));Serial.println(r.field.r_sat_err);
  Serial.print(F("\tvco_trim: "));Serial.println((uint16_t)r.field.vco_trim);
  Serial.print(F("\tvco_sel: "));Serial.println((uint16_t)r.field.vco_sel);
  Serial.print(F("\tcount_mode_mux_sel: "));Serial.println(r.field.count_mode_mux_sel);
  Serial.print(F("\tcount (tst): "));Serial.println(r.tst_mode.count);
  Serial.print(F("\tcount_mode_mux_sel (rb): "));Serial.println(r.rb_mode.count_mode_mux_sel);
  Serial.print(F("\treg (rb): "));Serial.println((uint16_t)r.rb_mode.reg);
  Serial.print(F("\trb_enable (rb): "));Serial.println(r.rb_mode.rb_enable);
}

void TRF3765::tell_r1(reg1_t r) {
  Serial.print(F("Reg1:\n"));
  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
  Serial.print(F("\trdiv: "));Serial.println(r.field.rdiv);
  Serial.print(F("\tinvert_ref_clock: "));Serial.println(r.field.invert_ref_clock);
  Serial.print(F("\tneg_vco: "));Serial.println(r.field.neg_vco);
  Serial.print(F("\ticp: "));Serial.println((uint16_t) r.field.icp);
  Serial.print(F("\ticpdouble: "));Serial.println(r.field.icpdouble);
  Serial.print(F("\tcal_clk_sel: "));Serial.println((uint16_t)r.field.cal_clk_sel);
}

void TRF3765::tell_r2(reg2_t r) {
  Serial.print(F("Reg2:\n"));
  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
  Serial.print(F("\tnint: "));Serial.println((uint16_t) r.field.nint);
  Serial.print(F("\tpll_div_sel: "));Serial.println((uint16_t) r.field.pll_div_sel);
  Serial.print(F("\tprescale_sel: "));Serial.println(r.field.prescale_sel);
  Serial.print(F("\tvco_sel: "));Serial.println((uint16_t) r.field.vco_sel);
  Serial.print(F("\tvcosel_mode: "));Serial.println(r.field.vco_sel_mode);
  Serial.print(F("\tcal_acc: "));Serial.println((uint16_t) r.field.cal_acc);
  Serial.print(F("\ten_cal: "));Serial.println(r.field.en_cal);
}

void TRF3765::tell_r3(reg3_t r) {
  Serial.print(F("Reg3:\n"));
  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
  Serial.print(F("\tnfrac: "));Serial.println(r.field.nfrac);
}

void TRF3765::tell_r4(reg4_t r) {
  Serial.print(F("Reg4:\n"));
  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
  Serial.print(F("\tpwd_pll: "));Serial.println(r.field.pwd_pll);
  Serial.print(F("\tpwd_cp: "));Serial.println(r.field.pwd_cp);
  Serial.print(F("\tpwd_vco: "));Serial.println(r.field.pwd_vco);
  Serial.print(F("\tpwd_vcomux: "));Serial.println(r.field.pwd_vcomux);
  Serial.print(F("\tpwd_rfdiv: "));Serial.println(r.field.pwd_rfdiv);
  Serial.print(F("\tpwd_presc: "));Serial.println(r.field.pwd_presc);
  Serial.print(F("\tpwd_lodiv: "));Serial.println(r.field.pwd_lodiv);
  Serial.print(F("\tpwd_buff1: "));Serial.println(r.field.pwd_buff1);
  Serial.print(F("\tpwd_buff2: "));Serial.println(r.field.pwd_buff2);
  Serial.print(F("\tpwd_buff3: "));Serial.println(r.field.pwd_buff3);
  Serial.print(F("\tpwd_buff4: "));Serial.println(r.field.pwd_buff4);
  Serial.print(F("\ten_extvco: "));Serial.println(r.field.en_extvco);
  Serial.print(F("\text_vco_ctrl: "));Serial.println(r.field.ext_vco_ctrl);
  Serial.print(F("\ten_isource: "));Serial.println(r.field.en_isource);
  Serial.print(F("\tld_ana_prec: "));Serial.println((uint16_t)r.field.ld_ana_prec);
  Serial.print(F("\tcp_tristate: "));Serial.println((uint16_t)r.field.cp_tristate);
  Serial.print(F("\tspeedup: "));Serial.println(r.field.speedup);
  Serial.print(F("\tld_dig_prec: "));Serial.println(r.field.ld_dig_prec);
  Serial.print(F("\ten_dith: "));Serial.println(r.field.en_dith);
  Serial.print(F("\tmod_ord: "));Serial.println((uint16_t)r.field.mod_ord);
  Serial.print(F("\tdith_sel: "));Serial.println(r.field.dith_sel);
  Serial.print(F("\tdel_sd_clk: "));Serial.println((uint16_t)r.field.del_sd_clk);
  Serial.print(F("\ten_frac: "));Serial.println(r.field.en_frac);
}

void TRF3765::tell_r5(reg5_t r) {
  Serial.print(F("Reg5:\n"));
  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
  Serial.print(F("\tvcobias_rtirm: "));Serial.println((uint16_t) r.field.vcobias_rtirm);
  Serial.print(F("\tpllbias_rtrim: "));Serial.println((uint16_t) r.field.pllbias_rtrim);
  Serial.print(F("\tvco_bias: "));Serial.println((uint16_t) r.field.vco_bias);
  Serial.print(F("\tvcobuf_bias: "));Serial.println((uint16_t) r.field.vcobuf_bias);
  Serial.print(F("\tvcomux_bias: "));Serial.println((uint16_t) r.field.vcomux_bias);
  Serial.print(F("\tbufout_bias: "));Serial.println((uint16_t) r.field.bufout_bias);
  Serial.print(F("\tvco_cal_ib: "));Serial.println(r.field.vco_cal_ib);
  Serial.print(F("\tvco_cal_ref: "));Serial.println((uint16_t) r.field.vco_cal_ref);
  Serial.print(F("\tvco_ampl_ctrl: "));Serial.println((uint16_t) r.field.vco_ampl_ctrl);
  Serial.print(F("\tvco_vb_ctrl: "));Serial.println((uint16_t) r.field.vco_vb_ctrl);
  Serial.print(F("\ten_ld_isource: "));Serial.println(r.field.en_ld_isource);
}

void TRF3765::tell_r6(reg6_t r) {
  Serial.print(F("Reg6:\n"));
  Serial.print(F("\traw: 0x"));Serial.println(r.raw,HEX);
  Serial.print(F("\taddr: "));Serial.println((uint16_t) r.data.addr);
  Serial.print(F("\t_pad: "));Serial.println((uint16_t) r.data._pad);
  Serial.print(F("\tvco_trim: "));Serial.println((uint16_t) r.field.vco_trim);
  Serial.print(F("\ten_lockdet: "));Serial.println(r.field.en_lockdet);
  Serial.print(F("\tvco_test_mode: "));Serial.println(r.field.vco_test_mode);
  Serial.print(F("\tcal_bypass: "));Serial.println(r.field.cal_bypass);
  Serial.print(F("\tmux_ctrl: "));Serial.println((uint16_t) r.field.mux_ctrl);
  Serial.print(F("\tisource_sink: "));Serial.println(r.field.isource_sink);
  Serial.print(F("\tisource_trim: "));Serial.println((uint16_t) r.field.isource_trim);
  Serial.print(F("\tlo_div_sel: "));Serial.println((uint16_t) r.field.lo_div_sel);
  Serial.print(F("\tlo_div_ib: "));Serial.println((uint16_t) r.field.lo_div_ib);
  Serial.print(F("\tdiv_mux_ref: "));Serial.println((uint16_t) r.field.div_mux_ref);
  Serial.print(F("\tdiv_mux_out: "));Serial.println((uint16_t) r.field.div_mux_out);
  Serial.print(F("\tdiv_mux_bias_ovrd: "));Serial.println(r.field.div_mux_bias_ovrd);
}
