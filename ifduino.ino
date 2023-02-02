//#include <SdFat.h>
#include <sdios.h>
#include <OneWire.h>  //for crc
#include <EEPROM.h>
#include "pe43705.h"
#include "trf3765.h"
#include "pins.h"
#include "MemoryFree.h"
#include <ArduinoJson.h>

//#define DEBUG_EEPROM
//#define DEBUG_RUN_TIME
//#define DEBUG_COMMAND

#define POWERUP_TIME_MS 20
#define VERSION_STRING "IFShield v0.5"
#define VERSION 0x00

//EEPROM Addresses (Uno has 1024 so valid adresses are 0x0000 - 0x03FF
#define EEPROM_VERSION_ADDR     0x000  // 3  bytes (version int repeated thrice)
#define EEPROM_BOOT_COUNT_ADDR  0x003  // 1  byte
#define EEPROM_DATA_CRC16_ADDR  0x0006 // 2  bytes
#define EEPROM_DATA_ADDR        0x0008 //


#pragma mark Globals
bool booted=false;
bool told_hello=false;
uint32_t boottime;
uint8_t bootcount;
ArduinoOutStream cout(Serial);


TRF3765 lo;   //the local oscillator
PE43705 attenuator;  //the digital attenuators (will take a channel argument)


typedef struct eeprom_version_t {
  uint8_t v[3];
} eeprom_version_t;

typedef struct eeprom_data_t {
  double lo;
  bool fractional;
  bool gen2;
  bool calibrate;
  regmap_t lo_config;
  attens_t attens;
} eeprom_data_t;

eeprom_data_t config;


#pragma mark Commands
#define N_COMMANDS 16
bool PCcommand();
bool PVcommand();
bool ATcommand();
bool FMcommand();
bool LOcommand();
bool NFcommand();
bool TScommand();
bool ZBcommand();
bool IOcommand();
bool WRcommand();
bool RRcommand();
bool FRcommand();
bool G2command();
bool CAcommand();
bool SVcommand();
bool STcommand();

typedef struct command_t {
    //String name;
    char name[3];
    bool (*callback)();
    const bool allowOffline;
} command_t;

command_t commands[N_COMMANDS]={
    //name, callback, offlineok
    //Print Commands
    {"PC", PCcommand, true},
    //Print version String
    {"PV", PVcommand, true},
    //Attenuation  channel, attenuation
    {"AT", ATcommand, true},
    //LO Mode (integer/fractional)
    {"FM", FMcommand, true},
    //Set LO Frequency
    {"LO", LOcommand, true},
    {"NF",NFcommand, false}, //nudge it
    //Tell Status
    {"TS", TScommand, true},
    //Zero the boot and reset
    {"ZB", ZBcommand, true},
    //On/Off
    {"IO", IOcommand, true},
    //Write an LO Reg CAUTION!
    {"WR", WRcommand, false},
    //Read LO Regs
    {"RR", RRcommand, false},
    //set fref MHz
    {"FR", FRcommand, true},
    //save settings
    {"SV", SVcommand, true},
    //toggle between gen2/3 setup mode
    {"G2", G2command, true},
    //toggle calibration in gen3 setup mode
    {"CA", CAcommand, true},
    //run self-test
    {"ST", STcommand, true}
};

#define COMMAND_BUFFER_LEN 41
char command_buffer[COMMAND_BUFFER_LEN];
unsigned char command_buffer_ndx=0;
unsigned char command_length=0;
bool have_command_to_parse=false;

typedef struct {
    int8_t ndx;
    uint8_t arg_len;
    char *arg_buffer;
} Instruction;

Instruction instruction;


#ifdef DEBUG_COMMAND
void printCommandBufNfo(){
  cout<<F("Command Buffer Info\n");
  cout<<"Buf ndx: "<<(unsigned int)command_buffer_ndx<<" Cmd len: "<<(unsigned int)command_length<<endl;
  cout<<"Contents: ";Serial.write((const uint8_t*)command_buffer,command_buffer_ndx);
}
#endif

//Search through command names for a name that matches the first two
// characters received. Attempt to compute argument
// lengths and buffer offsets. Load everything into the instruction. Return false
// if no command was matched or fewer than two characters received.
bool parseCommand() {
    //Extract the command from the command_buffer
    uint8_t consumed=0;
    instruction.ndx=-1;
    if(command_length >= 2) {
        for (uint8_t i=0; i<N_COMMANDS;i++) {
//          if (strncmp(commands[i].name, command_buffer, strlen(commands[i].name))==0){
          if (commands[i].name[0]==command_buffer[0] && 
              commands[i].name[1]==command_buffer[1]) {
            instruction.ndx=i;
            consumed=2;
            break;
          }
        }
    }
    instruction.arg_len=command_length-consumed-1; //exclude /n
    instruction.arg_buffer=&command_buffer[consumed];
    return instruction.ndx!=-1;
}

#pragma mark Serial Event Handler

void serialEvent() {
  char i, n_bytes_to_read;
  if(!have_command_to_parse) {
    n_bytes_to_read=Serial.available();
    if (command_buffer_ndx>COMMAND_BUFFER_LEN-2) //Something out of whack, reset buffer so new messages can be received
      command_buffer_ndx=0;
    if (n_bytes_to_read > COMMAND_BUFFER_LEN-1-command_buffer_ndx)
      n_bytes_to_read=COMMAND_BUFFER_LEN-1-command_buffer_ndx;
    Serial.readBytes(command_buffer+command_buffer_ndx, n_bytes_to_read);
    i=command_buffer_ndx;
    command_buffer_ndx+=n_bytes_to_read;
    while (!have_command_to_parse && i<command_buffer_ndx) {
      have_command_to_parse=command_buffer[i]=='\n';
      i++;
    }
    if (have_command_to_parse) {
      command_length=i; //Length inclusive of null terminator
      command_buffer[command_length]=0;
    }
  }
}



#pragma mark Setup & Loop

void _lockISR() {
  lo.set_rise((PORTB & 0b10000)>0 ? millis():0);
}

void setup() {

    boottime=millis();

    // Start serial connection
    Serial.begin(115200);
    //Pin Modes & pullups
    pinMode(PIN_SS,     OUTPUT);
    pinMode(PIN_SS_LO,  OUTPUT);
    pinMode(PIN_ONOFF,  OUTPUT);
    pinMode(PIN_LOCKED, INPUT);
    /*
     * 32u4 SPI
     * SS PB0
     * SCLK PB1
     * MOSI PB2
     * MISO PB3
      */
    DDRB|=0b0111;
    DDRB&=~0b1000;
    digitalWrite(PIN_SS,     LOW);
    digitalWrite(PIN_SS_LO,  LOW);
    digitalWrite(PIN_LOCKED, LOW);
    digitalWrite(PIN_ONOFF,  LOW);

    attachInterrupt(4, _lockISR, CHANGE);
    
    config.lo=6000.0;
    config.fractional=true;
    config.gen2=true;
    config.calibrate=true;
    loadEEPROM();  


    //Boot info
    boottime=millis()-boottime;
    bootcount=bootCount(true);
    cout<<F("#Boot ")<<(uint16_t) bootcount<<F(" took ")<<boottime<<F(" ms.\n");
    booted=true;
}


//Main loop, runs forever at full steam
void loop() {

    if (Serial.available())
        serialEvent();
    if (!told_hello && (millis()%1000)==0) cout<<"#Enter PC for cmd list\n";
    if (have_command_to_parse) {
        bool commandGood=false;

        #ifdef DEBUG_COMMAND
            printCommandBufNfo();
        #endif

        //If not a command respond error
        if (parseCommand()) {
            command_t *cmd=&commands[instruction.ndx];

            #ifdef DEBUG_COMMAND
                cout<<"Command is ";Serial.println(cmd->name);
            #endif

            if (!cmd->allowOffline && !powered()) {
                cout<<F("#Powered down\n");
                commandGood=false;
            } else {
                commandGood=cmd->callback();  //Execute the command
                told_hello=true;
            }
        }

        Serial.println(commandGood ? ":" : "?");
        //Reset the command buffer and the command received flag now that we are done with it
        have_command_to_parse=false;
        command_buffer_ndx=0;
    }
}



#pragma mark Helper Functions
uint8_t bootCount(bool set) {
    uint8_t count;
    count=EEPROM.read(EEPROM_BOOT_COUNT_ADDR);
    if (set==true && count < 255) {
        //increment & save boot count
        EEPROM.write(EEPROM_BOOT_COUNT_ADDR, ++count);
    }
    return count;
}

inline bool powered() {
  return digitalRead(PIN_ONOFF);
}

//Return true if data in eeprom is consistent with current software version
// update version
bool versionMatch() {
  bool updateVersion=false;
  eeprom_version_t ver_info;

  EEPROM.get(EEPROM_VERSION_ADDR, ver_info);
  if (ver_info.v[0]!=ver_info.v[1] || ver_info.v[1] != ver_info.v[2]) {
    updateVersion=true;     //version corrupt or didn't exist
  } else if (ver_info.v[0]!=VERSION) {
    updateVersion=true;     //Version changed
  }

  if (updateVersion) {
    ver_info.v[0]=VERSION;
    ver_info.v[1]=VERSION;
    ver_info.v[2]=VERSION;
    EEPROM.put(EEPROM_VERSION_ADDR, ver_info);
    return false;
  }

  return true;
}


bool loadEEPROM() {
    #ifdef DEBUG_EEPROM
    cout<<F("#Restoring EEPROM info")<<endl;
    #endif
    uint16_t crc, saved_crc;
    eeprom_data_t data;
    bool ret=false;

    #ifdef DEBUG_EEPROM
        uint32_t t=millis();
    #endif

    //Fetch the stored config & CRC16
    EEPROM.get(EEPROM_DATA_ADDR, data);
    EEPROM.get(EEPROM_DATA_CRC16_ADDR, saved_crc);
    crc=OneWire::crc16((uint8_t*) &data, sizeof(eeprom_data_t));

    //If the CRC matches, restore the config
    if (crc == saved_crc) {
        config=data;
        ret=true;
    } else {
        cout<<F("#ERROR EEPROM CRC")<<endl;
        ret=false;
    }

    #ifdef DEBUG_EEPROM
        cout<<F("#Restoring config from EEPROM took ")<<millis()-t<<" ms.\n";
    #endif

    return ret;
}

//Store the current config to EEPROM
void saveEEPROM() {
    uint16_t crc;

    #ifdef DEBUG_EEPROM
        uint32_t t=millis();
    #endif

    //Store with CRC16
    EEPROM.put(EEPROM_DATA_ADDR, config);
    crc=OneWire::crc16((uint8_t*) &config, sizeof(eeprom_data_t));
    EEPROM.put(EEPROM_DATA_CRC16_ADDR, crc);
    #ifdef DEBUG_EEPROM
        cout<<"Saving config to EEPROM took "<<millis()-t<<" ms.\n";
    #endif
}

void powerDown(bool save) {
//Saving settings made while unpowered is not supported due to potential LO register consistency issues.
    if (!powered()) return;
    if (save) saveEEPROM();
    digitalWrite(PIN_ONOFF, LOW);
}


void powerUp() {
  digitalWrite(PIN_ONOFF, HIGH);
  delay(POWERUP_TIME_MS);
  attenuator.set_attens(config.attens);
  lo.set_freq(config.lo, config.fractional, config.calibrate, config.gen2, false);
  config.lo = lo.get_freq();
  config.lo_config = lo.get_config();
}

void print_attens(attens_t attens) {
    cout<<"{\"adc1\":"<<attens.adc1<<", \"adc2\":"<<attens.adc2;
    cout<<" , \"dac1\":"<<attens.dac1<<", \"dac2\":"<<attens.dac2<<"}";
}

#pragma mark Command Handlers
//Report the version string
bool PVcommand() {
  cout<<VERSION_STRING<<endl;
  return true;
}

//Print the commands
bool PCcommand() {
    cout<<F("#PC - Print this list\n"
            "#TS - Tell Status\n"
            "#PV - Print Version\n"
            "#ZB{1} - Zero boot count, reset settings if 1 \n"
            "#AT[?|D1#,D2#,A1#,A2#] - Get/Set attenuations.\n"
            "#LO[#|?] - Get/Set LO\n"
            "#NF[#] - Nugde LO\n"
            "#FM[?|T|F] - Use fractional (not integer) mode for LO\n"
            "#G2[?|T|F] - Use gen2 mode for LO\n"
            "#CA[?|T|F] - Do full calibration when setting LO\n"
            "#ST - Do self-test and report the results, FM matters\n"
            "#FR# - Set Fref (MHz) for LO calcs\n"
            "#IO[?|1|0]{S} - On/Off. Last set settings are loaded, S to save settings at poweroff\n"\
            "#WR[0-6][hex#] - Write 32bit hex reg value to lo. Read source & PDF.\n"
            "#RR[0-6] - Read hex value of LO register\n"
            "{}=opt. []=req.\n");
    return true;
}


//Zero the boot count and set to uninited
bool ZBcommand(){
  if (instruction.arg_len>0) EEPROM.write(EEPROM_DATA_CRC16_ADDR, 0);
  EEPROM.write(EEPROM_BOOT_COUNT_ADDR, 0);
  return true;
}


//Get/Set attenuation: [?|#,#,#,#|#,#]
bool ATcommand() {

    if (instruction.arg_buffer[0]=='?') {
        print_attens(config.attens);
        return true;
    }

    attens_t attens;
    uint8_t natten=0;

    char* token = strtok(instruction.arg_buffer, ",");
    for (uint8_t i=0;i<4;i++) {
        if(token==NULL) break;
        natten++;
        float x=atof(token);
        switch (i) {
          case 0:
            attens.dac1=x;
            break;
          case 1:
            attens.dac2=x;
            break;
          case 2:
            attens.adc1=x;
            break;
          case 3:
            attens.adc2=x;
            break;
        }
        token = strtok(NULL, ",");
    }
    switch (natten) {
        case 4: //4 attenuations   dac1, dac2 adc1, adc2
            attenuator.set_attens(attens);
            break;
//        case 2:  //attenuator number, attenuation
//            if (!attenuator.set_atten(((uint8_t)attens.adc1), attens.adc2))
//              return false;
//            break;
        default:
            return false;
    }
    config.attens=attenuator.get_attens();
    return true;
}


//Report the status
bool TScommand() {
  cout<<F("#================\n");

    StaticJsonDocument<128> doc;
    doc[F("coms")]=told_hello;
    doc[F("boot")]=(uint16_t)bootcount;
    doc[F("ver")]=VERSION_STRING;
    doc[F("gen2")]=config.gen2;
    doc[F("fract")]=config.fractional;
    doc[F("g3fcal")]=config.calibrate;
    doc[F("lo")]=config.lo;
    doc[F("power")]=powered();
    Serial.print(F("{\n\"global\":"));
    serializeJson(doc, Serial);Serial.println(",");
    Serial.print(F("\"attens\":"));print_attens(config.attens);Serial.println(",");
    Serial.print(F("\"trf\":"));lo.tell_status();Serial.println("}");


//  
//  cout<<F("boot: ")<<(uint16_t)bootcount<<endl;
//  cout<<F("version: ")<<VERSION_STRING<<endl;
//  cout<<F("gen2: ")<<(config.gen2 ? "true":"false")<<endl;
//  cout<<F("fractional: ")<<(config.fractional ? "true":"false")<<endl;
//  cout<<F("full_calib: ")<<(config.calibrate ? "true":"false")<<endl;
//  cout<<F("lo: ")<<config.lo<<endl;
//  cout<<F("powered: ")<<(powered() ? "true":"false")<<endl;
//  cout<<F("attenuators:"); print_attens();
  cout<<F("#IF -- dac1 -- dac2 -- RF -- adc1 -- adc2 -- IF (dB)")<<endl;
//  cout<<"trf:\n";
//  lo.tell_status();
  cout<<F("#================\n");
  return true;
}

bool NFcommand() {
    if (instruction.arg_len<1)
        return false;
    double freq = atof(instruction.arg_buffer);
    if (!lo.set_freq(freq, config.fractional, config.calibrate, config.gen2, true))
        return false;

    //NB while presently accurate if not powered this may not always be so
    config.lo = lo.get_freq();
    config.lo_config = lo.get_config();
    return true;
}


bool LOcommand() {
    double freq;

    if (instruction.arg_len<1)
        return false;

    if (instruction.arg_buffer[0] == '?') {
        cout<<config.lo<<endl;
        return true;
    }

    freq = atof(instruction.arg_buffer);
    if (!lo.set_freq(freq, config.fractional, config.calibrate, config.gen2, false))
        return false;

    //NB while presently accurate if not powered this may not always be so
    config.lo = lo.get_freq();
    config.lo_config = lo.get_config();
    return true;
}

bool IOcommand() {
    if (instruction.arg_len<1)
        return false;
    if (instruction.arg_buffer[0]=='?') {
        cout<<digitalRead(PIN_ONOFF);
        return true;
    }
    if (instruction.arg_buffer[0] == '0') {
        powerDown(instruction.arg_buffer[1]=='S');
        return true;
    } else if (instruction.arg_buffer[0] =='1') {
        powerUp();
        return true;
    }
    return false;
}

bool WRcommand() {
  uint8_t reg;
  uint32_t val;
  if (instruction.arg_len<2)
    return false;
  reg=instruction.arg_buffer[0]-'0';
  if (reg>6)
    return false;
  val=strtol(instruction.arg_buffer+1, NULL, 16);
  lo.set_register(reg, val);
  config.lo = lo.get_freq();
  config.lo = lo.get_freq();
}

bool RRcommand() {
  if (instruction.arg_len<1) return false;
  uint8_t reg;
  uint32_t val;
  reg=atol(instruction.arg_buffer);
  if (reg>6) return false;

  val=lo.get_register(reg);
  Serial.print("0x");Serial.println(val, HEX);
}


bool FRcommand() {
  if (instruction.arg_len<1) return false;

  double freq;
  freq = atof(instruction.arg_buffer);
  if (!lo.set_fref(freq))
    return false;
  config.lo = lo.get_freq();
  config.lo_config = lo.get_config();
  return true;
}

bool bool_setting_command(bool &setting) {
    if (instruction.arg_len<1)
        return false;
    if (instruction.arg_buffer[0] == '?')
        cout<<setting<<endl;
    else if (instruction.arg_buffer[0] == 'T')
        setting=true;
    else if (instruction.arg_buffer[0] == 'F')
        setting=false;
    else
        return false;
  return true;
}

//Fractional mode [?|T|F]
bool FMcommand() {
    return bool_setting_command(config.fractional);
}

bool G2command() {
    return bool_setting_command(config.gen2);
}

bool CAcommand() {
    return bool_setting_command(config.calibrate);
}

bool SVcommand() {
    saveEEPROM();
    return true;
}


bool STcommand() {
  lo.self_test(config.lo, config.fractional);
  return true;
}
