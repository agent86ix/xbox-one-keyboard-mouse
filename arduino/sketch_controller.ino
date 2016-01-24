#include <SPI.h>

#define NUM_BUTTONS 16

typedef enum {
  PIN_A = 0,
  PIN_B = 1,
  PIN_X = 2,
  PIN_Y = 3,
  PIN_LB = 4,
  PIN_RB = 5,
  PIN_L3 = 6,
  PIN_R3 = 7,
  PIN_BACK = 8,
  PIN_START = 9,
  /* 11, 12, 13 are for SPI */
  PIN_LS = 14,
  PIN_RS = 15,
  PIN_T = 16  
} pin_t;

typedef enum {
  BUTTON_TYPE_DIGITAL,
  BUTTON_TYPE_POT_XY,
  BUTTON_TYPE_POT_X,
  BUTTON_TYPE_POT_Y,  
} button_type_t;

typedef struct {
  pin_t pin;
  button_type_t type;
  unsigned char address;
  unsigned char init;
} button_map_t;

button_map_t buttonMap[NUM_BUTTONS] = {
  {PIN_A, BUTTON_TYPE_DIGITAL, 0, 1},
  {PIN_B, BUTTON_TYPE_DIGITAL, 1, 1},
  {PIN_X, BUTTON_TYPE_DIGITAL, 2, 1},
  {PIN_Y, BUTTON_TYPE_DIGITAL, 3, 1},
  {PIN_LB, BUTTON_TYPE_DIGITAL, 4, 1},
  {PIN_RB, BUTTON_TYPE_DIGITAL, 5, 1},
  {PIN_L3, BUTTON_TYPE_DIGITAL, 6, 1},
  {PIN_R3, BUTTON_TYPE_DIGITAL, 7, 1},
  {PIN_BACK, BUTTON_TYPE_DIGITAL, 8, 1},
  {PIN_START, BUTTON_TYPE_DIGITAL, 9, 1},
  {PIN_LS, BUTTON_TYPE_POT_X, 10, 127},
  {PIN_LS, BUTTON_TYPE_POT_Y, 11, 127},
  {PIN_RS, BUTTON_TYPE_POT_X, 12, 127},
  {PIN_RS, BUTTON_TYPE_POT_Y, 13, 127},
  {PIN_T, BUTTON_TYPE_POT_X, 14, 0},
  {PIN_T, BUTTON_TYPE_POT_Y, 15, 0},
};

/* Constants for the digial pots */
const int wiper1Reg = (0b0001<<12);
const int wiper0Reg = (0b0000<<12);
const int tconReg = (0b0100<<12);
const int statusReg = (0b0101<<12);
const int invalidReg = (0b0011<<12);
const int readCmd = (0b11<<10);
const int writeCmd = (0b00<<10);

/* Storage location for address, in case there is a time delta between addr. and data */
int cmdByte0;

void serialPrint16(unsigned short input) {
  for(int i=0; i<16; i++) {
    if(input&(1<<(15-i))) Serial.print("1");
    else Serial.print("0");
  }
}

void resetController() {
  for(int i=0; i<NUM_BUTTONS; i++) {
    if(buttonMap[i].type == BUTTON_TYPE_DIGITAL) {
      pinMode(buttonMap[i].pin, INPUT);
      digitalWrite(buttonMap[i].pin, LOW);    
    } else {
      pinMode(buttonMap[i].pin, OUTPUT);
      pinMode(buttonMap[i].pin, HIGH);
      if(buttonMap[i].type == BUTTON_TYPE_POT_X) {
        spiWrite16(buttonMap[i].pin, wiper0Reg|writeCmd|buttonMap[i].init);
      } else if (buttonMap[i].type == BUTTON_TYPE_POT_Y) {
        spiWrite16(buttonMap[i].pin, wiper1Reg|writeCmd|buttonMap[i].init);
      }
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  cmdByte0 = -1;
  Serial.begin(9600);
  delay(500);

  Serial.println("startup");
  SPI.begin();
  
  resetController();
  

  /*
  pinMode(PIN_LS, OUTPUT);
  digitalWrite(PIN_LS, HIGH);
  pinMode(PIN_RS, OUTPUT);
  digitalWrite(PIN_RS, HIGH);
  pinMode(PIN_T, OUTPUT);
  digitalWrite(PIN_T, HIGH);
  SPI.begin();

  spiWrite16(PIN_LS, wiper0Reg|writeCmd|127);
  spiWrite16(PIN_LS, wiper1Reg|writeCmd|127);
  spiWrite16(PIN_RS, wiper0Reg|writeCmd|127);
  spiWrite16(PIN_RS, wiper1Reg|writeCmd|127);
  spiWrite16(PIN_T, wiper0Reg|writeCmd|127);
  spiWrite16(PIN_T, wiper1Reg|writeCmd|127);
  */
}

void spiWrite16(int cs, unsigned short value) {
  //unsigned short valueSwap = ((value&0xff)<<8) | ((value>>8)&0xff);
  SPI.beginTransaction(SPISettings(10*1000, MSBFIRST, SPI_MODE0));

  digitalWrite(cs, LOW);
  
  //byte readData0 = SPI.transfer((value&0xff00)>>8);
  //byte readData1 = SPI.transfer((value&0xff));
  //int readData = readData0<<8 | readData1;
  int readData = SPI.transfer16(value);

  digitalWrite(cs, HIGH);
  SPI.endTransaction();
  
  Serial.print("In: ");
  serialPrint16(value);
  Serial.print(" Out: ");
  serialPrint16(readData);
  Serial.print("\n");
}

void serialTest(int byteRead) {
  
  switch(byteRead) {
      case 'w':
        spiWrite16(PIN_LS, wiper0Reg|writeCmd|0);
        spiWrite16(PIN_LS, wiper1Reg|writeCmd|127);
        break;
      case 'a':
        spiWrite16(PIN_LS, wiper0Reg|writeCmd|127);
        spiWrite16(PIN_LS, wiper1Reg|writeCmd|0);
        break;
      case 's':
        spiWrite16(PIN_LS, wiper0Reg|writeCmd|255);
        spiWrite16(PIN_LS, wiper1Reg|writeCmd|127);
        break;
      case 'd':
        spiWrite16(PIN_LS, wiper0Reg|writeCmd|127);
        spiWrite16(PIN_LS, wiper1Reg|writeCmd|255);
        spiWrite16(PIN_RS, wiper0Reg|writeCmd|127);
        spiWrite16(PIN_RS, wiper1Reg|writeCmd|255);
        spiWrite16(PIN_T, wiper0Reg|writeCmd|127);
        spiWrite16(PIN_T, wiper1Reg|writeCmd|255);
        break;
      case ' ':
        spiWrite16(PIN_LS, wiper0Reg|writeCmd|127);
        spiWrite16(PIN_LS, wiper1Reg|writeCmd|127);
        spiWrite16(PIN_RS, wiper0Reg|writeCmd|127);
        spiWrite16(PIN_RS, wiper1Reg|writeCmd|127);
        spiWrite16(PIN_T, wiper0Reg|writeCmd|127);
        spiWrite16(PIN_T, wiper1Reg|writeCmd|127);
        break;
      case 't':
        Serial.println("read TCON reg");
        spiWrite16(PIN_LS, tconReg|readCmd);
        break;
      case 'g':
        Serial.println("read STATUS reg");
        spiWrite16(PIN_LS, statusReg|readCmd);
        break;
      case 'i':
        Serial.println("invalid command");
        spiWrite16(PIN_LS, invalidReg|readCmd);
        break;
      case '1':
        Serial.println("sweep #1");
        for(int i=0; i<255; i++) {
          spiWrite16(PIN_LS, wiper0Reg|writeCmd|i);
          spiWrite16(PIN_LS, wiper1Reg|writeCmd|i);    
          delay(50);      
        }
        break;
      case '2':
        Serial.println("sweep #2");
        for(int j=0; j<6; j++) {
          if(j%1) spiWrite16(PIN_T, wiper1Reg|writeCmd|255);    
          else    spiWrite16(PIN_T, wiper0Reg|writeCmd|255);    
          for(int i=0; i<255; i+=32) {
            spiWrite16(PIN_LS, wiper0Reg|writeCmd|i);
            spiWrite16(PIN_LS, wiper1Reg|writeCmd|i);    
            spiWrite16(PIN_RS, wiper0Reg|writeCmd|i);
            spiWrite16(PIN_RS, wiper1Reg|writeCmd|i);    
            if(j%1) spiWrite16(PIN_T, wiper0Reg|writeCmd|i);
            else    spiWrite16(PIN_T, wiper1Reg|writeCmd|i);    
            delay(500);      
          }
        }
        break;
    }
}


void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()) {
    byte byteRead = Serial.read();
    Serial.print("new byte: 0x");
    Serial.println(byteRead, HEX);
    if(cmdByte0 == -1){
      if(byteRead >= NUM_BUTTONS<<3) {
        resetController(); 
        return;
      }
      cmdByte0 = byteRead;
      return;
    } 

    int address = cmdByte0>>3;
    
    Serial.print("testing addr: 0x");
    Serial.println(address, HEX);
    for(int i=0; i<NUM_BUTTONS; i++) {
      if(buttonMap[i].address == address) {
        Serial.print("addr maps to pin ");
        Serial.println(buttonMap[i].pin);
        if((buttonMap[i].type == BUTTON_TYPE_DIGITAL)) {
          if(byteRead != 0) {
            pinMode(buttonMap[i].pin, OUTPUT);
          } else {
            pinMode(buttonMap[i].pin, INPUT);
          }
        } else if(buttonMap[i].type == BUTTON_TYPE_POT_X) {
          spiWrite16(buttonMap[i].pin, wiper0Reg|writeCmd|byteRead);
        } else if(buttonMap[i].type == BUTTON_TYPE_POT_Y) {
          spiWrite16(buttonMap[i].pin, wiper1Reg|writeCmd|byteRead);
        }
      }
    }
    cmdByte0 = -1;
  }
  
}
