// Software Nashisizer
// I2C commands to move the fader

#include <Wire.h>
#include "MCP28S17_constants.h"

#define mpcReset A1
#define SS_PIN 5
#define SCKPIN 13
#define SDIPIN 6
#define LED_DATA 2
#define LED_CLOCK 4
#define LED_READY 7
#define NUMBYTES 108 // 36 LEDs @ 3bytes per LED

byte ledBuffer[NUMBYTES];
byte motorEnable[] = {11, 9, 3, 10};
byte MOTOR_1A[] = {4, 6, 0, 2}; // bits on expander port B
byte MOTOR_2A[] = {5, 7, 1, 3}; // bits on expander port B
byte motorControl = 0;
byte motorLast = 0;
// colours to use first 4 green for each slider, then red, then blue
byte slide[] =    {20, 0, 0, 20,   0, 20, 0, 10,   0, 0, 20, 0};
byte slidePost[] = {0, 20, 20, 0,  20, 0, 20, 10,  20, 20, 0, 20};

int current[] = {0, 0, 0, 0}; // current slider position
int last[] = {2000, 2000, 2000, 2000}; // last slider position
int target[] = {0, 0, 0, 0}; // place to move the slider to
boolean moved[] = {false, false, false, false}; // has the motor been moved?
boolean moveing[] = {false, false, false, false}; // is slider currently moveing
boolean changed[] = {false, false, false, false};
boolean toMove = false; // a move command is requested
byte analogueChannel[] = {3, 7, 2, 6}; // A/D channel to read slider pot

void setup() {
  pinMode(mpcReset, OUTPUT); // use pin 2 to reset the chip
  digitalWrite(mpcReset, LOW);
  pinMode(SS_PIN, OUTPUT);
  pinMode(SCKPIN, OUTPUT);
  pinMode(SDIPIN, OUTPUT);
  pinMode(LED_DATA , OUTPUT);
  pinMode(LED_CLOCK , OUTPUT);
  digitalWrite(LED_CLOCK, HIGH);
  pinMode(LED_READY, INPUT);
  Serial.begin(230400);
//  Serial.println("Start");

  Wire.begin(0x1C);                // join i2c bus with address 0x1C
  Wire.onReceive(receiveRequest); // register event
  Wire.onRequest(requestData); // register event

  digitalWrite(SS_PIN, HIGH);  // Disable the SPI device
  digitalWrite(mpcReset, HIGH);  // take chip out of reset
  // Set up ICON register
  expanderW(0x0A, BANK | SEQOP | HAEN); //  ICON defaults to address 0xA on reset
  // initilise SPI expander data direction register
  expanderW(IODIRB, 0x00);      // Data direction register all outputs
  expanderW(GPIOB, motorControl);  // all motors off
  for (int i = 0; i < 4; i++) {
    current[i] = analogRead(analogueChannel[i]);
  }
}

void loop() {
  
  if (toMove) {
    moveSliders();
    toMove = false;
  }
  checkSliders();
  for (int i = 0; i < 4; i++) {
    if (changed[i]) trackSliders(i); // update LEDs
  }
  
}

void checkSliders() {
  
  int s;
  for (int i = 0; i < 4; i++) {
    s = analogRead(analogueChannel[i]);
    if (abs(last[i] - s) > 4) {
      changed[i] = true;
      last[i] = current[i];
//      Serial.print(i);
//      Serial.print(": ");
//      Serial.println(s);
    }
    else {
      changed[i] = false;
    }
    current[i] = s;
  }
  
}

void moveSliders() {
  boolean needingToMove = true;
  //Serial.println("moving motors");
  for (int i = 0; i < 4; i++) {
    // if(i==0) Serial.print("motors 0 ");
    current[i] = analogRead(analogueChannel[i]);
    //if(i==0) Serial.println(current[i]);
    if (current[i] == target[i]) {
      changed[0] = false ;
      moved[i] = true;
    }
    moveing[i] = true; // to prevent a read while slider is moveing
    setDirection(i);
    analogWrite(motorEnable[i], 190); // turn on motor
  }

  while (needingToMove) {
    for (int i = 0; i < 4; i++) {
      if ( abs(current[i] - target[i]) > 8 ) { // are we where we need to be
        current[i] = analogRead(analogueChannel[i]);
        //if(i==0) Serial.println(current[i]);
        if (abs(current[i] - target[i]) < 40 ) { // slow down a bit when approaching target
          analogWrite(motorEnable[i], 160);
        } else {
          analogWrite(motorEnable[i], 180); // incase new command arrived before old one finished
        }
        //if(i==0) {Serial.print(" direction ");Serial.println(i);}
        setDirection(i); // incase of overshoot
      }
      else {
        analogWrite(motorEnable[i], 0); // disable motor
        updateMotors(0, i); // flywheel breaking
        moved[i] = true; // say the motor has been moved
        moveing[i] = false; // say the motor is now not moveing
        changed[i] = false; // no chnage
        target[i] = current[i]; // we are where we are going
        needingToMove = false;
        for (int j = 0; j < 4; j++) {
          if (moveing[j]) needingToMove = true;
        }
      }
    }
  } // end of while needing to move
}

void setDirection(byte i) {
  if (current[i] == target[i]) return;
  if (current[i] < target[i]) { // set motor direction
    updateMotors(1, i); // motor towards the high end of the slider
  }
  else {
    updateMotors(2, i); // motor towards the low end of the slider
  }
}

void updateMotors(byte dir, byte slider) {
  if (slider > 3) {
    dir = 0;
    slider = 0;
  }
  // dir = 0 is for motor off
  motorControl = motorControl & ~((1 << MOTOR_1A[slider]) | (1 << MOTOR_2A[slider]));
  if (dir == 1) { // forwards
    motorControl = motorControl | (1 << MOTOR_1A[slider]);
  }
  if (dir == 2) { // reverse
    motorControl = motorControl | (1 << MOTOR_2A[slider]);
  }
  if (dir == 3) { // alternat stop
    motorControl = motorControl | (1 << MOTOR_2A[slider]) | (1 << MOTOR_1A[slider]);
  }

  if (motorLast != motorControl) { // only write if there has been a change
    expanderW(GPIOB, motorControl);
    motorLast = motorControl;
  }
}

void requestData() { // send the position of all 4 pots
  int data;
  for (int i = 0; i < 4; i++) {
    //data = analogRead(analogueChannel[i]);
    data = current[i];
    if (moveing[i]) {
      Wire.write(0xff); // return -1 if the slider is moving
      Wire.write(0xff);
    }
    else {
      Wire.write(data >> 8); // respond with message of 2 bytes MSB first
      Wire.write(data & 0xff);
    }
  }
}

void receiveRequest(int howMany) { // set pot position
  int goTo = 0;
  goTo = Wire.read(); // get first byte 0 for pot position
  if (goTo == 0) {
    for (int i = 0; i < 4; i++) {
      goTo = Wire.read() << 8; // receive byte
      goTo |= Wire.read();
      //Serial.print("Target" ); Serial.println(goTo);   // print the requested position
      target[i] = goTo;
      moved[i] = false;
      changed[i] = true;
    }
    toMove = true;
  }
  else if (goTo == 1) { // get colour to use
    for (int i = 0; i < 12; i++) {
      goTo = Wire.read();
      slide[i] = goTo;
    }
    for (int i = 0; i < 12; i++) {
      goTo = Wire.read();
      slidePost[i] = goTo;
    }
    for (int i = 0; i < 4; i++) {
      trackSliders(i);  // update LEDs
    }
  }
}

void expanderW(byte add, byte dat) // expander read  Device external address select all zero
{
  digitalWrite(SS_PIN, LOW);
  SPIbang(0x40);  // address write  Device external address select all zero
  SPIbang(add);   //  register address
  SPIbang(dat);   //  register data
  digitalWrite(SS_PIN, HIGH);
}

void SPIbang(int val) { // bit bang SPI so it is interruptable
  for ( int i = 0; i < 8; i++) {
    if ( (val & 0x80) == 0) {
      digitalWrite(SDIPIN, LOW);
    }
    else {
      digitalWrite(SDIPIN, HIGH);
    }
    digitalWrite(SCKPIN, HIGH); // toggle clock
    digitalWrite(SCKPIN, LOW);
    val = val << 1;
  }
}

void trackSliders(byte slider) { // change LEDs with silder movement
  byte track = 0;
  byte pointer;
  //Serial.print("Updating LEDs for slider "); Serial.println(slider);
  for (int led = slider * 9; led < 9 + (slider * 9) ; led++) {
    track += 1;
    pointer = led * 3;
    //Serial.println(pointer);
    if (1023 - current[slider] > (track * 113)) {
      ledBuffer[pointer]   = slide[slider];
      ledBuffer[pointer + 1] = slide[slider + 4];
      ledBuffer[pointer + 2] = slide[slider + 8];
    }
    else {
      ledBuffer[pointer]   = slidePost[slider];
      ledBuffer[pointer + 1] = slidePost[slider + 4];;
      ledBuffer[pointer + 2] = slidePost[slider + 8];
    }
  }
  ledSend(); // send new update
}
void ledSend() {
  //Serial.println("ready to send");
  digitalWrite(LED_CLOCK, HIGH);
  while (digitalRead(LED_READY) == LOW) { } // hold until ready to send
  //Serial.println("starting to send");
  for (int i = 0; i < NUMBYTES; i++) {
    // Serial.println("sending byte");Serial.println(i);
    for (int j = 0; j < 8; j++) { // send a byte
      digitalWrite(LED_DATA, ((ledBuffer[i] >> (7 - j)) & 0x01) );
      digitalWrite(LED_CLOCK, LOW);
      delayMicroseconds(40);
      digitalWrite(LED_CLOCK, HIGH);
      delayMicroseconds(20);
    }
  }
  // dummy clock to end sequence
  digitalWrite(LED_CLOCK, LOW);
  delayMicroseconds(200);
  digitalWrite(LED_CLOCK, HIGH);
  delayMicroseconds(200);
}

