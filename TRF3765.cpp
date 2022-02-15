#include "TRF3765.h"
#include <SPI.h> 
#include "pins.h"
//Gets contents of register address
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
//  reg4_t r4={.raw=val};
//tell_r4(r4);
//
//  tell_r4(*((reg4_t*)&r));
//
//  tell_r4(r4);
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
//  pinMode(PIN_CLK,      OUTPUT);
//  digitalWrite(PIN_CLK, HIGH);
//  delayMicroseconds(10);
//  digitalWrite(PIN_CLK, LOW);
  SPI.end();

//    pinMode(PIN_CLK,      OUTPUT);
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
  return digitalRead(PIN_LOCKED);
}

bool TRF3765::set_freq(double freq, bool fractional) {

  uint8_t loDivSel, loDiv;
  uint8_t pllDiv;
  uint8_t pllDivSel;
  uint8_t prscSel;
  uint16_t rDiv;
  uint16_t nInt;
  uint32_t nFrac;

  // Calculate parameter and register values
  freq /= 2; // Output frequency is doubled
  if(freq>2400){
    loDiv = 1;
    loDivSel = 0;
  } else if(freq>1200) {
    loDiv = 2;
    loDivSel = 1;
  } else if(freq>600) {
    loDiv = 4;
    loDivSel = 2;
  } else if(freq>300) {
    loDiv = 8;
    loDivSel = 3;
  }
  pllDiv = (uint8_t)ceil(((double)loDiv)*freq/3000.0);
  rDiv = 1;
  double frac_tmp=freq*(double)rDiv*(double)loDiv/(10.0*(double)pllDiv);
  nInt = (uint16_t)frac_tmp;
  nFrac = (frac_tmp-nInt)*(1<<25);
  prscSel = nInt>=(fractional ? 75:72); // 72 in integer mode, 75 in frac
    
  if(pllDiv==1)
      pllDivSel = 0;
  else if(pllDiv==2)
      pllDivSel = 1;
  else if(pllDiv==4)
      pllDivSel = 2;    
  else {
      Serial.println(F("Invalid PLL_DIV!\n"));
      return false;
  }

  regmap_t rm=REGMAP();
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
  rm.regs.r2.field.ndiv=nInt;
  rm.regs.r2.field.prescale_sel=prscSel;
  rm.regs.r2.field.pll_div=pllDivSel;
    

  //Note that the microblaze gen2 code NEVER wrote the value it computed for r2 in integer mode!
  uint32_t itmp[] = {rm.regs.r1.raw, rm.regs.r3.raw, XSP_REG4_WRITE, XSP_REG5_WRITE, 
                     rm.regs.r6.raw, XSP_REG2_WRITE_ENC};
    
  //fractional mode regs
  uint32_t ftmp[] = {XSP_REG0_WRITE, rm.regs.r1.raw, rm.regs.r3.raw, XSP_REG4_WRITE_FRAC,
                     XSP_REG5_WRITE_FRAC, rm.regs.r6.raw, rm.regs.r2.raw};

  reset(fractional);

  
  if (fractional) {
    Serial.println(F("Programming for fractional"));
//    tell_r0((reg0_t) REG_FROM_RAW(0,XSP_REG0_WRITE));
    tell_r1(rm.regs.r1);
    tell_r3(rm.regs.r3);
    tell_r4((reg4_t) REG_FROM_RAW(4,XSP_REG4_WRITE_FRAC));
    tell_r5((reg5_t) REG_FROM_RAW(5,XSP_REG5_WRITE_FRAC));
    tell_r6(rm.regs.r6);
    tell_r2(rm.regs.r2);
    for(int i=0;i<7;i++) _send(ftmp[i]);
  } else {
    Serial.println(F("Programming for integer"));
    tell_r1(rm.regs.r1);
    tell_r3(rm.regs.r3);
    tell_r4((reg4_t) REG_FROM_RAW(4,XSP_REG4_WRITE));
    tell_r5((reg5_t) REG_FROM_RAW(5,XSP_REG5_WRITE));
    tell_r6(rm.regs.r6);
    tell_r2((reg2_t) REG_FROM_RAW(5,XSP_REG2_WRITE_ENC));
    for(int i=0;i<6;i++) _send(itmp[i]);
  }
  return true;
}

//Gets LO value
double TRF3765::get_freq() {
  return 0.0;
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
  tell_regmap(init_data);
  
  for (int i=0;i<7;i++) _send(init_data.data[i]);

}


void TRF3765::tell_status() {
  reg0_t r;
  regmap_t rm;
  if (locked()) Serial.println(F("LO Locked"));
  else Serial.println(F("LO Not Locked"));
  for (int i=0;i<7;i++)
    rm.data[i]=_request(RB_REQUEST(i).raw);
    
  tell_regmap(rm);
  Serial.print("Reg7: 0x");Serial.println(_request(RB_REQUEST(7).raw),HEX);
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
  Serial.print(F("\trdev: "));Serial.println(r.field.rdiv);
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
  Serial.print(F("\tpll_div: "));Serial.println((uint16_t) r.field.pll_div);
  Serial.print(F("\tprescale_sel: "));Serial.println(r.field.prescale_sel);
  Serial.print(F("\tvco_sel: "));Serial.println((uint16_t) r.field.vco_sel);
  Serial.print(F("\tvco_sel_mode: "));Serial.println(r.field.vco_sel_mode);
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
