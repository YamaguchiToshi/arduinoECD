#include <MsTimer2.h>
#include <IRremote.h>

#define ST_WAITING    0
#define ST_SCANNING   1
#define ST_SENDING    2
#define ST_RECIEVING  3
#define ST_LEARNING   4
#define ST_LSCANNING  5
#define ST_COUNT      6

#define IR_IN   12
#define IR_OUT  11
#define ST_LED  10

#define ST_SW    7
#define CTRL_SW  8

#define POS1     2
#define POS2     3
#define POS3     4
#define POS4     5
#define POS5     6


// グローバル変数
int STATUS = ST_WAITING;  // initial status
int cPOS = 0;             // initial position of scanning LEDs
IRrecv irrecv(IR_IN);     // IR reciever
decode_results results;   // IR decode result
int st_val = 0;           // value of status button
int st_val_old = 0;       // previous value of status button
int ctrl_val = 0;         // value of scanning control button
int ctrl_val_old = 0;     // previous value of scanning control button

void setup(){
  //pinMode(IR_IN, INPUT);
  pinMode(IR_OUT, OUTPUT);
  pinMode(ST_LED, OUTPUT);

  pinMode(ST_SW, INPUT);
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
  if( STATUS == ST_SCANNING || STATUS == ST_LSCANNING ){
    for( cPOS = POS1; cPOS <= POS5; cPOS++ ){
      digitalWrite(cPOS, HIGH);
      if( cPOS > POS1 ){
        digitalWrite(cPOS-1, LOW);
      } else {
        digitalWrite(POS5, LOW);      
      }
      delay(800);
      if( STATUS == ST_SENDING || STATUS == ST_COUNT ){
        break;
      }
    }
  }

  // send IR signal
  if( STATUS == ST_SENDING ){
    for( int i = 0; i < 5; i++ ){
      digitalWrite(cPOS, !digitalRead(cPOS));
      delay(100);
    }
    //SENDSENDSENDSENDSEND
    STATUS = ST_WAITING; // back to WAITING mode
  }

  // indicate chaning to RECIEVE mode 
  if( STATUS == ST_COUNT ){
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
    irrecv.enableIRIn();    // IR reciever on
    STATUS = ST_RECIEVING;  // status change
  }
    
  // recieving IR signal
  if( STATUS == ST_RECIEVING ){
    if( irrecv.decode(&results) ){
      dump(&results);
      irrecv.resume(); // Receive the next value
    }
      //irrecv.decode(&results);
      //Serial.println("*****");
      //Serial.println(results.value, HEX);
      //STATUS = ST_WAITING; // 受信終了後はWAITINGへ
      //digitalWrite(ST_LED, LOW);
    //irrecv.resume();
  }  
}


void btnCheck(){
  st_val = digitalRead(ST_SW);
  ctrl_val = digitalRead(CTRL_SW);
  
  // モード切り替えスイッチ  
  if( (st_val == HIGH) && (st_val_old == LOW) ){    
    switch( STATUS ){
      case ST_WAITING:
        digitalWrite(ST_LED, HIGH);
        STATUS = ST_LEARNING;
        break;
      case ST_LEARNING:
        digitalWrite(ST_LED, LOW);
        STATUS = ST_WAITING;
        break;
      case ST_LSCANNING:
        digitalWrite(ST_LED, LOW);
        STATUS = ST_WAITING;
        break;
    }
    delay(10);
  }
  
  // コントロールスイッチ  
  if( (ctrl_val == HIGH) && (ctrl_val_old == LOW) ){
    switch( STATUS ){
      case ST_WAITING:
        STATUS = ST_SCANNING;
        break;
      case ST_LEARNING:
        STATUS = ST_LSCANNING;
        break;
      case ST_SCANNING:
        STATUS = ST_SENDING;
        break;
      case ST_LSCANNING:
        STATUS = ST_COUNT;
        break;
    }
    delay(10);
  }
  
  st_val_old = st_val;
  ctrl_val_old = ctrl_val;
}






void dump(decode_results *results) {
  int count = results->rawlen;
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
