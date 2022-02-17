typedef struct reg_t {
    uint8_t addr:3;
    uint8_t _pad:2;
    uint32_t val:27;
} reg_t;

typedef struct r0fields_t {
    uint8_t :5;
    bool chip_id:1;
    uint8_t :6;
    bool r_sat_err:1;
    uint8_t :2;
    uint8_t vco_trim:6;
    bool :1;
    uint8_t vco_sel:2;
    uint8_t :7;
    bool count_mode_mux_sel:1;
} r0fields_t;

typedef struct r0count_t {
    uint16_t :13;
    uint32_t count:18;
    uint8_t :1;
} r0count_t;

typedef struct rbfields_t {
    uint8_t :5;
    uint32_t :22;
    bool count_mode_mux_sel:1;
    uint8_t reg:3;
    bool rb_enable:1;
} rbfields_t;

typedef struct r1fields_t {
    uint8_t :5;
    uint16_t rdiv:13;
    bool :1;
    bool invert_ref_clock:1;
    bool neg_vco:1;
    uint8_t icp:5; //p30
    bool icpdouble:1;
    uint8_t cal_clk_sel:4;  //p30
    bool :1;
} r1fields_t;

typedef struct r2fields_t {
    uint8_t :5;
    uint16_t nint:16;
    uint8_t pll_div_sel:2;  //div value = 2**pll_div valid values 0,1,2
    bool prescale_sel:1;
    uint8_t :2;
    uint8_t vco_sel:2;
    bool vco_sel_mode:1;
    uint8_t cal_acc:2;
    bool en_cal:1;
} r2fields_t;

typedef struct r3fields_t {
    uint8_t :5;
    uint32_t nfrac:25; //0 - 0.99999, 2^26-1 steps
    uint8_t :2;
} r3fields_t;

 typedef struct r4fields_t {
    uint8_t :5;
    bool pwd_pll:1;
    bool pwd_cp:1;
    bool pwd_vco:1;
    bool pwd_vcomux:1;
    bool pwd_rfdiv:1;
    bool pwd_presc:1;
    bool pwd_lodiv:1;
    bool pwd_buff1:1;
    bool pwd_buff2:1;
    bool pwd_buff3:1;
    bool pwd_buff4:1;
    bool en_extvco:1;
    bool ext_vco_ctrl:1;
    bool en_isource:1;
    uint8_t ld_ana_prec:2;
    uint8_t cp_tristate:2;
    bool speedup:1;
    bool ld_dig_prec:1;
    bool en_dith:1;
    uint8_t mod_ord:2;
    bool dith_sel:1;
    uint8_t del_sd_clk:1;
    bool en_frac:1;
} r4fields_t;

typedef struct r5fields_t {
    uint8_t :5;
    uint8_t vcobias_rtirm:3;
    uint8_t pllbias_rtrim:2;
    uint8_t vco_bias:4;
    uint8_t vcobuf_bias:2;
    uint8_t vcomux_bias:2;
    uint8_t bufout_bias:2;
    uint8_t :2;
    bool vco_cal_ib:1;
    uint8_t vco_cal_ref:3;
    uint8_t vco_ampl_ctrl:2;
    uint8_t vco_vb_ctrl:2;
    bool :1;
    bool en_ld_isource:1;
} r5fields_t;

typedef struct r6fields_t {
    uint8_t :5;
    uint8_t :2;
    uint8_t vco_trim:6;
    bool en_lockdet:1;
    bool vco_test_mode:1;
    bool cal_bypass:1;
    uint8_t mux_ctrl:3;
    bool isource_sink:1;
    uint8_t isource_trim:3;
    uint8_t lo_div_sel:2;
    uint8_t lo_div_ib:2;
    uint8_t div_mux_ref:2;
    uint8_t div_mux_out:2;
    bool div_mux_bias_ovrd:1;
} r6fields_t;

typedef union reg0_t {
  r0fields_t field;
  r0count_t tst_mode;
  rbfields_t rb_mode;
  reg_t data; 
  uint32_t raw;
} reg0_t;

typedef union reg1_t {
  r1fields_t field;
  reg_t data;
  uint32_t raw;
} reg1_t;

typedef union reg2_t {
  r2fields_t field;
  reg_t data;
  uint32_t raw;
} reg2_t;

typedef union reg3_t {
  r3fields_t field;
  reg_t data;
  uint32_t raw;
} reg3_t;

typedef union reg4_t {
  r4fields_t field;
  reg_t data;
  uint32_t raw;
} reg4_t;

typedef union reg5_t {
  r5fields_t field;
  reg_t data;
  uint32_t raw;
} reg5_t;
  
typedef union reg6_t {
  r6fields_t field;
  reg_t data;
  uint32_t raw;
} reg6_t;

typedef union genericreg_t {
  reg_t data;
  uint32_t raw;
} genericreg_t;

typedef union regmap_t {
  struct {
  reg0_t r0;
  reg1_t r1;
  reg2_t r2;
  reg3_t r3;
  reg4_t r4;
  reg5_t r5;
  reg6_t r6;
  } regs;
  uint32_t data[7];
} regmap_t;

inline reg_t REG_FROM_DATA(uint8_t n, uint32_t v) {
  reg_t r={.addr=n, ._pad=1, .val=v};
  return r;
}

#define NEW_REG(REG,N) (REG) {data:{addr:N, _pad:1}}
#define REG_FROM_RAW(N, RAW) {data:{addr:N, _pad:1, val:(RAW>>5)}}

inline reg0_t RB_REQUEST(uint8_t n) {
  reg0_t r;
  r.data.addr=0;
  r.data._pad=1;
  r.rb_mode.count_mode_mux_sel=false;
  r.rb_mode.reg=n;
  r.rb_mode.rb_enable=true;
  return r;
}


inline regmap_t REGMAP() {
  regmap_t rm={.regs={.r0={.data={.addr=0,._pad=1}},
                     .r1={.data={.addr=1,._pad=1}},
                     .r2={.data={.addr=2,._pad=1}},
                     .r3={.data={.addr=3,._pad=1}},
                     .r4={.data={.addr=4,._pad=1}},
                     .r5={.data={.addr=5,._pad=1}},
                     .r6={.data={.addr=6,._pad=1}}}};
  return rm;
}


#define REG_FROM_DATA(N, X) {data:{addr:N, _pad:1, val:X}}
