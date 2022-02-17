#include <SdFat.h>
#include <OneWire.h>  //for crc
#include <EEPROM.h>
#include "PE43705.h"
#include "TRF3765.h"
#include "pins.h"
#include "MemoryFree.h"

//#define DEBUG_EEPROM
//#define DEBUG_RUN_TIME
#define DEBUG_COMMAND

#define POWERUP_TIME_MS 100
#define VERSION_STRING "IFShield v0.5"
#define VERSION 0x00

//EEPROM Addresses (Uno has 1024 so valid adresses are 0x0000 - 0x03FF
#define EEPROM_VERSION_ADDR     0x000  // 3  bytes (version int repeated thrice)
#define EEPROM_BOOT_COUNT_ADDR  0x003  // 1  byte
#define EEPROM_MODE_ADDR        0x004  // 1  byte  0=unconfigured 1=debug 2=normal operation  anything else -> 0
#define EEPROM_DATA_CRC16_ADDR  0x0006 // 2  bytes
#define EEPROM_DATA_ADDR        0x0008 // 


#pragma mark Globals
bool booted=false;
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
#define N_COMMANDS 14
bool PCcommand();
bool PVcommand();
bool ATcommand();
bool LMcommand();
bool LOcommand();
bool TScommand();
bool ZBcommand();
bool IOcommand();
bool WRcommand();
bool RRcommand();
bool FRcommand();
bool G2command();
bool CAcommand();
bool STcommand();

typedef struct {
    String name;
    bool (*callback)();
    const bool allowOffline;
} Command;

Command commands[N_COMMANDS]={
    //name callback offlineok
    //Print Commands
    {"PC", PCcommand, true},
    //Print version String
    {"PV", PVcommand, true},
    //Attenuation  channel, attenuation
    {"AT", ATcommand, true},
    //LO Mode (integer/fractional)
    {"LM", LMcommand, true},
    //Set LO Frequency
    {"LO", LOcommand, true},
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
    //toggle between gen2/3 setup mode
    {"G2", G2command, true},
    //toggle calibration in gen3 setup mode
    {"CA", CAcommand, true},
    //run self-test
    {"ST", STcommand, true}
};


char command_buffer[81];
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
    String name;
    uint8_t consumed=0;
    instruction.ndx=-1;
    if(command_length >= 2) {
        name+=command_buffer[0];
        name+=command_buffer[1];
        for (uint8_t i=0; i<N_COMMANDS;i++) {
          if (commands[i].name==name) {
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
    if (command_buffer_ndx>79) //Something out of whack, reset buffer so new messages can be received
      command_buffer_ndx=0;
    if (n_bytes_to_read > 80-command_buffer_ndx)
      n_bytes_to_read=80-command_buffer_ndx;
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


void setup() {

    boottime=millis();

    // Start serial connection
    Serial.begin(115200);
    //Pin Modes & pullups 
//    pinMode(PIN_MISO,   INPUT);
//    pinMode(PIN_MOSI,   OUTPUT);
//    pinMode(PIN_CLK,    OUTPUT);
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
//    digitalWrite(PIN_CLK,    LOW);
    digitalWrite(PIN_SS,     LOW);
    digitalWrite(PIN_SS_LO,  LOW);
//    digitalWrite(PIN_MISO,   LOW);  //?
//    digitalWrite(PIN_MOSI,   LOW);  
    digitalWrite(PIN_LOCKED, LOW); //?
    digitalWrite(PIN_ONOFF,  LOW);

    config.lo=6000.0;
    config.fractional=true;
    config.gen2=true;
    loadEEPROM();

    //Boot info
    boottime=millis()-boottime;
    bootcount=bootCount(true);
    cout<<F("#Boot ")<<(uint16_t) bootcount<<F(" took ")<<boottime<<F(" ms.\n");
    booted=true;

}


//Main loop, runs forever at full steam
void loop() {

  if (Serial.available()) serialEvent();
    //If the command received flag is set
    if (have_command_to_parse) {

        bool commandGood=false;
        
        #ifdef DEBUG_COMMAND
            printCommandBufNfo();
        #endif

        //If not a command respond error
        if (parseCommand()) {
            Command *cmd=&commands[instruction.ndx];

            #ifdef DEBUG_COMMAND
                cout<<"Command is ";Serial.println(cmd->name);
            #endif
            
            if (!cmd->allowOffline && !powered()) {
                cout<<F("#Powered down\n");
                commandGood=false;
            } else {
                commandGood=cmd->callback();  //Execute the command 
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
    if (set==true && count < 200) {
        //increment & save boot count
        EEPROM.write(EEPROM_BOOT_COUNT_ADDR, ++count);
    }
    return count;
}

bool powered() {
  return digitalRead(PIN_ONOFF);
}

eeprom_data_t getState() { 
  return config;
}

bool restoreState(eeprom_data_t x) {
  config=x;
  return true;
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


//Load the nominal slits positions for all the slits from EEPROM
bool loadEEPROM() {
    cout<<F("#Restoring EEPROM info")<<endl;
    uint16_t crc, saved_crc;
    eeprom_data_t data;
    uint8_t mode;
    
    bool ret=false;
    
    #ifdef DEBUG_EEPROM
        uint32_t t=millis();
    #endif
    
    //Fetch the stored slit positions & CRC16
    EEPROM.get(EEPROM_DATA_ADDR, data);
    EEPROM.get(EEPROM_DATA_CRC16_ADDR, saved_crc);
    EEPROM.get(EEPROM_MODE_ADDR, mode);
    crc=OneWire::crc16((uint8_t*) &data, sizeof(eeprom_data_t));

    //If the CRC matches, restore the positions
    if (crc == saved_crc) {
        restoreState(data);
        ret=true;
    } else {
        cout<<F("#ERROR EEPROM CRC")<<endl;
        return false;
    }

    #ifdef DEBUG_EEPROM
        cout<<F("#Restoring config from EEPROM took ")<<millis()-t<<" ms.\n";
    #endif

    return ret;
}

//Store the nominal slits positions for all the slits to EEPROM
void saveEEPROM() {
    uint16_t crc;
    eeprom_data_t data;

    #ifdef DEBUG_EEPROM
        uint32_t t=millis();
    #endif

    data = getState();

    //Store with CRC16
    EEPROM.put(EEPROM_DATA_ADDR, data);
    crc=OneWire::crc16((uint8_t*) &data, sizeof(eeprom_data_t));
    EEPROM.put(EEPROM_DATA_CRC16_ADDR, crc);
    #ifdef DEBUG_EEPROM
        cout<<"Saving config to EEPROM took "<<millis()-t<<" ms.\n";
    #endif
}

void powerDown() {
  saveEEPROM();
  digitalWrite(PIN_ONOFF, LOW);
  attenuator.power_off();
  
}


void powerUp() {
  digitalWrite(PIN_ONOFF, HIGH);
  delay(POWERUP_TIME_MS);
  attenuator.set_atten(DAC1, config.attens.dac1);
  attenuator.set_atten(DAC2, config.attens.dac2);
  attenuator.set_atten(ADC1, config.attens.adc1);
  attenuator.set_atten(ADC2, config.attens.adc2);
  lo.set_freq(config.lo, config.fractional, config.calibrate, config.gen2);
  config.lo = lo.get_freq();
}


#pragma mark Command Handlers
//Report the version string
bool PVcommand() {
  cout<<VERSION_STRING<<endl;
  return true;
}


//Print the commands
bool PCcommand() {
    cout<<F("#PC - Print list of commands\n"\
            "#TS - Tell Status\n"\
            "#PV - Print Version\n"\
            "#ZB{1} - Zero boot count, reset settings if 1 \n"
  
            "#AT[?|I|Q|*]{#} - Get/Set the attenuation\n"\
            "#LO[#|?] - Get/Set the LO\n"\

            "#LM - Toggle between fractional and integer mode for setting LO\n"
            "#IO[1|0] - On/Off. Last set settings are loaded\n"\
            "#WR[0-6][hex#] - Write hex value to lo register. CAUTION\n"
            "#RR[0-6] - Read hex value of LO register\n");
    return true;
}


//Zero the boot count and set to uninited
bool ZBcommand(){
  if (instruction.arg_len>0) EEPROM.write(EEPROM_DATA_CRC16_ADDR, 0);
  EEPROM.write(EEPROM_BOOT_COUNT_ADDR, 0);
  EEPROM.write(EEPROM_MODE_ADDR, 0);
  return true;
}


//Get/Set attenuation: [I|Q|*]#
bool ATcommand() {

  if (instruction.arg_buffer[0]=='?') {
      cout<<"{adc1:"<<attenuator.get_atten(ADC1)<<", adc2:"<<attenuator.get_atten(ADC1);
      cout<<"{dac1:"<<attenuator.get_atten(DAC1)<<", dac2:"<<attenuator.get_atten(DAC2)<<"}"<<endl;
      return true;
  }

  if (instruction.arg_len<2) return false;

  if ((instruction.arg_buffer[0] != 'I' && instruction.arg_buffer[0]!='Q' && instruction.arg_buffer[0]!='*') ||
      instruction.arg_buffer[1] <'0' || instruction.arg_buffer[1]>'9') 
      return false;
  float atten=atof(instruction.arg_buffer+1);

  if (instruction.arg_buffer[0]=='1'||instruction.arg_buffer[0]=='*') {
    attenuator.set_atten(ADC1, atten);
    config.attens.adc1=attenuator.get_atten(ADC1);
  }
  if (instruction.arg_buffer[0]=='2'||instruction.arg_buffer[0]=='*') {
    attenuator.set_atten(ADC2, atten);
    config.attens.adc2=attenuator.get_atten(ADC2);
  }
  if (instruction.arg_buffer[0]=='3'||instruction.arg_buffer[0]=='*') {
    attenuator.set_atten(DAC1, atten);
    config.attens.dac1=attenuator.get_atten(DAC1);
  }
  if (instruction.arg_buffer[0]=='4'||instruction.arg_buffer[0]=='*') {
    attenuator.set_atten(DAC2, atten);
    config.attens.dac2=attenuator.get_atten(DAC2);
  }
  return true;
}


//Report the status
bool TScommand() {
  cout<<endl<<endl;
  cout<<F("================\nPower:");
  if (digitalRead(PIN_ONOFF)) cout<<" on";
  else cout<<" off";
  cout<<F("\nAttenuators: ");
  attenuator.tell_status();
  cout<<"LO: ";
  lo.tell_status();
  cout<<F("================\n");
  return true;
}

bool LOcommand() {
  if (instruction.arg_len<1) return false;
  if (instruction.arg_buffer[0] == '?') {
    cout<<lo.get_freq()<<endl;
    return true;
  }

  double freq;
  freq = atof(instruction.arg_buffer);
  if (!lo.set_freq(freq, config.fractional, config.calibrate, config.gen2)) 
    return false;
  config.lo = lo.get_freq();
  config.lo_config = lo.get_config();
  return true;
}

bool LMcommand() {
  config.fractional=!config.fractional;
  return true;
}

bool IOcommand() {
  if (instruction.arg_len<1) return false;
  if (instruction.arg_buffer[0] < '0' || instruction.arg_buffer[0] > '1') return false;
  if (instruction.arg_buffer[0] == '0') powerDown();
  else powerUp();
}

bool WRcommand() {
  uint8_t reg;
  uint32_t val;
  if (instruction.arg_len<2) return false;
  reg=atol(instruction.arg_buffer);
  if (reg>6) return false;
  val=strtol(instruction.arg_buffer+1, (char**)0, 16);
  lo.set_register(reg, val);
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
//  if (instruction.arg_buffer[0] == '?') {
//    cout<<lo.get_freq()<<endl;
//    return true;
//  }

  double freq;
  freq = atof(instruction.arg_buffer);
  if (!lo.set_fref(freq)) 
    return false;
  config.lo = lo.get_freq();
  config.lo_config = lo.get_config();
  return true;
}

bool G2command() {
  config.gen2=!config.gen2;
  return true;
}

bool CAcommand() {
  config.calibrate=!config.calibrate;
  return true;
}


bool STcommand() {
  lo.self_test(config.lo, config.fractional);
  return true;
}
