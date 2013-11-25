/**
 * Arduino ECD
 *
 * Version 0.0.2  May, 2013
 * Copyright (c) 2013 Toshimitsu Yamaguchi. All rights reserved.
 * 
 */

#include <MsTimer2.h>
#include <IRremote.h>
#include <memory.h>

#define ST_WAITING    0
#define ST_SCANNING   1
#define ST_SENDING    2
#define ST_RECEIVING  3
#define ST_LEARNING   4
#define ST_LSCANNING  5
#define ST_COUNT      6

#define IR_IN   12
#define IR_OUT  11
#define ST_LED  10

#define MD_SW    7
#define CTRL_SW  8

#define POS1     2
#define POS2     3
#define POS3     4
#define POS4     5
#define POS5     6


// Storage for the recorded code
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
unsigned int rawCodes[RAWBUF]; // The durations if raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

struct IRcode {
  int codeType; // The type of code
  unsigned long codeValue; // The code value if not raw
  unsigned int rawCodes[RAWBUF]; // The durations if raw
  int codeLen; // The length of the code
  int toggle; // The RC5/6 toggle state
};

// global variable
int STATE = ST_WAITING;   // initial state
int cPOS = 0;             // position of scanning LEDs
int md_val = 0;           // value of mode button
int md_val_old = 0;       // previous value of mode button
int ctrl_val = 0;         // value of scanning control button
int ctrl_val_old = 0;     // previous value of scanning control button
IRrecv irrecv(IR_IN);     // IR receiver
IRsend irsend;            // IR sender
decode_results results;   // IR decode result

IRcode myIRcodes[5];

void setup(){
  pinMode(IR_IN, INPUT);
  pinMode(IR_OUT, OUTPUT);
  pinMode(ST_LED, OUTPUT);

  pinMode(MD_SW, INPUT);
  pinMode(CTRL_SW, INPUT);

  pinMode(POS1, OUTPUT);
  pinMode(POS2, OUTPUT);
  pinMode(POS3, OUTPUT);
  pinMode(POS4, OUTPUT); 
  pinMode(POS5, OUTPUT);

  MsTimer2::set(50, btnCheck);
  MsTimer2::start();
  
  // for dev.
  Serial.begin(9600);
}


void loop(){
  // scanning
  if( STATE == ST_SCANNING || STATE == ST_LSCANNING ){
    for( cPOS = POS1; cPOS <= POS5; cPOS++ ){
      digitalWrite(cPOS, HIGH);
      if( cPOS > POS1 ){
        digitalWrite(cPOS-1, LOW);
      } else {
        digitalWrite(POS5, LOW);      
      }
      delay(800);
      if( STATE == ST_SENDING || STATE == ST_COUNT ){
        break;
      }
    }
  }

  // send IR signal
  if( STATE == ST_SENDING ){
    // flushing 3 times
    for( int i = 0; i < 5; i++ ){
      digitalWrite(cPOS, !digitalRead(cPOS));
      delay(100);
    }
    sendCode(1);
    STATE = ST_WAITING; // back to WAITING state
  }

  // indicate changing to RECIEVE mode 
  if( STATE == ST_COUNT ){
    for( int i = 0; i < 5; i++ ){
      digitalWrite(cPOS, !digitalRead(cPOS));
      delay(100);
    }
    for( int i = POS5; i >= POS1; i-- ){
      digitalWrite(i, HIGH);
      delay(300);
    }
    for( int i = POS1; i <= POS5; i++ ){
      digitalWrite(i, LOW);
      delay(1000);
    }
    irrecv.enableIRIn();    // IR receiver on
    STATE = ST_RECEIVING;   // change state 
  }
    
  // receiving IR signal
  if( STATE == ST_RECEIVING ){
    if( irrecv.decode(&results) ){
      myIRcodes[cPOS-2].toggle = 0;
      myIRcodes[cPOS-2].codeType = -1;
      storeCode(&results);
    }
  }  
}


void btnCheck(){
  //Serial.println(STATE);
  md_val = digitalRead(MD_SW);
  ctrl_val = digitalRead(CTRL_SW);
  // mode change SW  
  if( (md_val == HIGH) && (md_val_old == LOW) ){    
    switch( STATE ){
      case ST_WAITING:
        digitalWrite(ST_LED, HIGH);
        STATE = ST_LEARNING;
        break;
      case ST_LEARNING:
        digitalWrite(ST_LED, LOW);
        STATE = ST_WAITING;
        break;
      case ST_LSCANNING:
        digitalWrite(ST_LED, LOW);
        STATE = ST_WAITING;
        break;
    }
    delay(10); // for chattering
  }
  
  // control SW  
  if( (ctrl_val == HIGH) && (ctrl_val_old == LOW) ){
    switch( STATE ){
      case ST_WAITING:
        STATE = ST_SCANNING;
        break;
      case ST_LEARNING:
        STATE = ST_LSCANNING;
        break;
      case ST_SCANNING:
        STATE = ST_SENDING;
        break;
      case ST_LSCANNING:
        STATE = ST_COUNT;
        break;
    }
    delay(10); // for chattering
  }
  
  md_val_old = md_val;
  ctrl_val_old = ctrl_val;
}


void storeCode(decode_results *results) {
  myIRcodes[cPOS-2].codeType = results->decode_type;
  int count = results->rawlen;
  if (myIRcodes[cPOS-2].codeType == UNKNOWN) {
    Serial.println("Received unknown code, saving as raw");
    myIRcodes[cPOS-2].codeLen = results->rawlen - 1;
    // To store raw codes:
    // Drop first value (gap)
    // Convert from ticks to microseconds
    // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
    for (int i = 1; i <= myIRcodes[cPOS-2].codeLen; i++) {
      if (i % 2) {
        // Mark
        myIRcodes[cPOS-2].rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK - MARK_EXCESS;
        Serial.print(" m");
      } else {
        // Space
        myIRcodes[cPOS-2].rawCodes[i - 1] = results->rawbuf[i]*USECPERTICK + MARK_EXCESS;
        Serial.print(" s");
      }
      Serial.print(myIRcodes[cPOS-2].rawCodes[i - 1], DEC);
    }
    Serial.println("");
  } else {
    if (myIRcodes[cPOS-2].codeType == NEC) {
      Serial.print("Received NEC: ");
      if (results->value == REPEAT) {
        // Don't record a NEC repeat value as that's useless.
        Serial.println("repeat; ignoring.");
        return;
      }
    } else if (myIRcodes[cPOS-2].codeType == SONY) {
      Serial.print("Received SONY: ");
    } else if (myIRcodes[cPOS-2].codeType == RC5) {
      Serial.print("Received RC5: ");
    } else if (myIRcodes[cPOS-2].codeType == RC6) {
      Serial.print("Received RC6: ");
    } else {
      Serial.print("Unexpected codeType ");
      Serial.print(myIRcodes[cPOS-2].codeType, DEC);
      Serial.println("");
    }
    Serial.println(results->value, HEX);
    myIRcodes[cPOS-2].codeValue = results->value;
    myIRcodes[cPOS-2].codeLen = results->bits;
  }
  
  STATE = ST_WAITING; // 受信終了後はWAITINGへ
  digitalWrite(ST_LED, LOW);

  // ボタンのチェック用タイマの再開
  MsTimer2::set(50, btnCheck);
  MsTimer2::start();
}


void sendCode(int repeat) {
  if (myIRcodes[cPOS-2].codeType == NEC) {
    if (repeat) {
      irsend.sendNEC(REPEAT, myIRcodes[cPOS-2].codeLen);
      Serial.println("Sent NEC repeat");
    } else {
      irsend.sendNEC(myIRcodes[cPOS-2].codeValue, myIRcodes[cPOS-2].codeLen);
      Serial.print("Sent NEC ");
      Serial.println(myIRcodes[cPOS-2].codeValue, HEX);
    }
  } else if (myIRcodes[cPOS-2].codeType == SONY) {
    irsend.sendSony(myIRcodes[cPOS-2].codeValue, myIRcodes[cPOS-2].codeLen);
    Serial.print("Sent Sony ");
    Serial.println(myIRcodes[cPOS-2].codeValue, HEX);
  } else if (myIRcodes[cPOS-2].codeType == RC5 || myIRcodes[cPOS-2].codeType == RC6) {
    if (!repeat) {
      // Flip the toggle bit for a new button press
      myIRcodes[cPOS-2].toggle = 1 - myIRcodes[cPOS-2].toggle;
    }
    // Put the toggle bit into the code to send
    myIRcodes[cPOS-2].codeValue = myIRcodes[cPOS-2].codeValue & ~(1 << (myIRcodes[cPOS-2].codeLen - 1));
    myIRcodes[cPOS-2].codeValue = myIRcodes[cPOS-2].codeValue | (myIRcodes[cPOS-2].toggle << (myIRcodes[cPOS-2].codeLen - 1));
    if (myIRcodes[cPOS-2].codeType == RC5) {
      Serial.print("Sent RC5 ");
      Serial.println(myIRcodes[cPOS-2].codeValue, HEX);
      irsend.sendRC5(myIRcodes[cPOS-2].codeValue, myIRcodes[cPOS-2].codeLen);
    } else {
      irsend.sendRC6(myIRcodes[cPOS-2].codeValue, myIRcodes[cPOS-2].codeLen);
      Serial.print("Sent RC6 ");
      Serial.println(myIRcodes[cPOS-2].codeValue, HEX);
    }
  } else if (myIRcodes[cPOS-2].codeType == UNKNOWN /* i.e. raw */) {
    // Assume 38 KHz
    irsend.sendRaw(myIRcodes[cPOS-2].rawCodes, myIRcodes[cPOS-2].codeLen, 38);
    Serial.println("Sent raw");
  }
  
  // ボタンのチェック用タイマの再開
  MsTimer2::set(50, btnCheck);
  MsTimer2::start();
}








void dump(decode_results *results) {

  int count = results->rawlen;

  //memory[cPOS-2].maker = (unsigned int)results->decode_type;
  //memory[cPOS-2].hex = (unsigned long)results->value;
  //memory[cPOS-2].bits = (int)results->bits;
  //for(int i = 0; i < count; i++){
  //  memory[cPOS-2].raw[i] = (int)results->rawbuf[i];
  //}

  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  } 
  else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
  } 
  else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  } 
  else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  } 
  else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  }
  else if (results->decode_type == PANASONIC) {	
    Serial.print("Decoded PANASONIC - Address: ");
    Serial.print(results->panasonicAddress,HEX);
    Serial.print(" Value: ");
  }
  else if (results->decode_type == JVC) {
     Serial.print("Decoded JVC: ");
  }
  Serial.print(results->value, HEX);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (int i = 0; i < count; i++) {
    if ((i % 2) == 1) {
      Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
    } 
    else {
      Serial.print(-(int)results->rawbuf[i]*USECPERTICK, DEC);
    }
    Serial.print(" ");
  }
  Serial.println("");

}
