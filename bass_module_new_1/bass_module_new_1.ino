// Code for the base module of the Nashesizer
// Uses Teensy 3.6 with USB type 'All of the Above'
// This version only integrates the joystick module
// Schematic still being finalised
// Requires the Wire, Keyboard and StopWatch libraries.
// Coded in Arduino IDE 1.8.5 and Teensyduino 1.4.0


///////////////////////////////////////
/// LIBRARIES + GLOBALS
///////////////////////////////////////

#include <Wire.h> // I2C
#include "Keyboard.h" // Teensy as an emulated keyboard
#include <StopWatch.h>

// single byte requested from joystick module
byte bJs, bJsLast;
byte jsUp = B1000;
byte jsDown = B0100;
byte jsLeft = B0010;
byte jsRight = B0001;

// for rolling polling of I2C bus
int nextTime1  = 10; // do this every 10 milliseconds
long int goTime1;

// keyboard emulation
#define OSX 0
// change this to match your platform:
int platform = OSX;

/// StopWatch
StopWatch jsHoldSW;


///////////////////////////////////////
/// SET UP
///////////////////////////////////////

void setup() {

  Wire.begin(0); // join i2c bus (address optional for master)
  Serial.begin(230400); // start serial for Serial Monitor output
  Keyboard.begin();

}


///////////////////////////////////////
/// MAIN LOOP
///////////////////////////////////////

void loop() {

  // rolling polling of I2C slave modules
  if (millis() >= goTime1) functionReadJoystick();

}


///////////////////////////////////////
/// Nashesizer module functions
///////////////////////////////////////
void functionReadJoystick() {

  Wire.requestFrom(1, 1); // request 1 byte from slave device #1
  while (Wire.available()) { // slave may send less than requested
    bJs = Wire.read(); // receive a byte as character
  }
  ////debugging
  //Serial.print(bJs, BIN); // print the byte
  //Serial.print(",");

  if ((bJs == jsUp) && (bJsLast != bJs)) { // if a new joystick UP byte is received and the previous byte received was not UP
    Keyboard.press(KEY_UP_ARROW); // send a key press
    Keyboard.releaseAll();
    // debugging
    //Serial.print("bJs: ");
    //Serial.print(bJs);
    //Serial.print(", bJsLast: ");
    //Serial.println(bJsLast);
  } else if ((bJs == jsDown) && (bJsLast != bJs)) { // else if a new joystick DOWN byte is received and the previous byte received was not DOWN
    Keyboard.press(KEY_DOWN_ARROW);
    Keyboard.releaseAll();
    // debugging
    //Serial.print("bJs: ");
    //Serial.print(bJs);
    //Serial.print(", bJsLast: ");
    //Serial.println(bJsLast);
  } else if ((bJs == jsRight) && (bJsLast != bJs)) { // else if a new joystick RIGHT byte is received and the previous byte received was not RIGHT
    Keyboard.press(KEY_RIGHT_ARROW);
    Keyboard.releaseAll();
    // debugging
    //Serial.print("bJs: ");
    //Serial.print(bbJs);
    //Serial.print(", bJsLast: ");
    //Serial.println(bJsLast);
  } else if ((bJs == jsLeft) && (bJsLast != bJs)) { // else if a new joystick LEFT byte is received and the previous byte received was not LEFT
    Keyboard.press(KEY_LEFT_ARROW);
    Keyboard.releaseAll();
    // debugging
    //Serial.print("bJs: ");
    //Serial.print(bbJs);
    //Serial.print(", bJsLast: ");
    //Serial.println(bJsLast);
  } else {
    // debugging
    //Serial.print("bJs: ");
    //Serial.print(bbJs);
    //Serial.print(", bJsLast: ");
    //Serial.println(bJsLast);
  }

  if (bJsLast == bJs) { // if the new byte is the same as the last byte - joystick is being held - start the stopwatch
    jsHoldSW.start();
  } else { // otherwise stop and reset the stopwatch
    jsHoldSW.stop();
    jsHoldSW.reset();
  }
  // debugging
  //    Serial.print("jsHoldSW.elapsed(): ");
  //    Serial.println(jsHoldSW.elapsed());

  // use the stopwatch to reset the last byte to CENTRE after a set time so that the code above retriggers creating an auto repeat key press
  if ((jsHoldSW.elapsed() > 250) && ((bJs == jsUp) || (bJs == jsDown) || (bJs == jsRight) || (bJs == jsLeft))) {
    bJsLast = B00000000;
  } else {
    bJsLast = bJs;
  }

  goTime1 = millis() + nextTime1;

}
