// Latest code for the base module of the Nashesizer - now includes code from the 'MIDI_Serial_teensy_test1' sketch by Mike Cook to test 2-way comms
// Uses Teensy 3.6 with USB type 'All of the Above'
// This version integrates the joystick and rotary encoder modules
// Forwards joystick positions as emulated keypresses
// I2C two-way comms tested and working (though not yet fully implemented) from this master Teensy 3.6 to/from the rotary encoder module's slave Arduino Nano (downstream)...
// and to/from the Processing Nashesizer-Hub OSC-to-Serial bridge to/from Live (upstream)
// While not yet fully implemented (though tested and working) it sends encoder positions into Live and also receives and forwards on changes to LiveGrabber assigned GUI controls in Live..
// and forwards these on to the rotary encoder module to update encoder positions and their RGB LED colour
// Schematic still being finalised
// Requires the Wire, Keyboard and StopWatch libraries.
// Coded in Arduino IDE 1.8.5 and Teensyduino 1.4.0


///////////////////////////////////////
/// LIBRARIES + GLOBALS
///////////////////////////////////////

#include <Wire.h> // I2C
#include "Keyboard.h" // Teensy as an emulated keyboard
#include <StopWatch.h>

/// Wire configuration
int clockFrequency = 400000;

// single byte requested from joystick module
byte bJs, bJsLast;
byte jsUp = B1000;
byte jsDown = B0100;
byte jsLeft = B0010;
byte jsRight = B0001;

// variables for and bytes requested from the rotary encoders module
byte bEnc, bEncLast, bEncNo, bEncNoLast;
int activeTrkNo = 2; // currently hardcoded - will be variable depending on joystick position

// for rolling polling of I2C bus
int nextTime1  = 50; // do this every 50 milliseconds - joystick
int nextTime2  = 20; // do this every 20 milliseconds - encoder
long int goTime1, goTime2;

// keyboard emulation
#define OSX 0
// change this to match your platform:
int platform = OSX;

/// StopWatch
StopWatch jsHoldSW;

/// Serial comms from Processing Nashesizer-Hub OSC-to-Serial bridge
char buf[50];
boolean newMessage = false;

// to check if the new message from the rotary encoders module is the same as the last and to only write this via Serial if it is
String messageLast = "";

// incoming OSC message from the Nashesizer Hub
String fromLiveOSCMessage = "";

// 2D array to store parameter updates from Live tracks
// the Nashesizer default Live project will have a total of 24 tracks - 16 audio + 8 MIDI - with LiveGrabber devices preloaded
// TO DO - this doesn't currently accomodate the return and master tracks
float trkSettingsLive [24][6];
int liveTrkNo = 0;
String liveTrkNoC;
String paramTypeL, paramType;
int arrayColIndex = 0;
float paramValue = 0.0;
int ind1, ind2, ind3, ind4; // delimiter locations

///////////////////////////////////////
/// SET UP
///////////////////////////////////////

void setup() {

  Wire.begin(0); // join i2c bus (address optional for master)
  Wire.setClock(clockFrequency);
  Keyboard.begin();
  Serial.begin(250000); // start Serial for USB output
  // enabling hardware Serial port to read Serial messages to and from the Teensy
  // via an SLAB_USBtoUART module and CoolTerm whilst the Teensy is connected to the Nashesizer Hub via USB
  Serial2.begin(230400);
  //Serial.flush();
  Serial2.println("Serial2 enabled...");

  // initialise trkSettingsLive array
  for (int i = 0; i < 24; i++) {
    for (int j = 0; j < 6; j++) {
      trkSettingsLive[i][j] = 0.0;
    }
  }

}


///////////////////////////////////////
/// MAIN LOOP
///////////////////////////////////////

void loop() {

  if (Serial.available() > 0) readIncomming(); 
  if (newMessage) handleMessage(); // do something with message

  // rolling polling of I2C slave modules
  if (millis() >= goTime1) functionReadJoystick();
  if (millis() >= goTime2) functionReadEncoders();

}


///////////////////////////////////////
/// Serial functions
///////////////////////////////////////

void readIncomming() {
  for ( int i = 0; i < sizeof(buf);  ++i ) buf[i] = (char)0; // clear the buffer
  fromLiveOSCMessage = ""; // reset the message
  static int i = 0;
  while (Serial.available() > 0 && newMessage == false) {
    buf[i] = Serial.read();
    //Serial2.print(buf[i]);
    if (buf[i] == ':') { // added ':' delimiter to end of liveOSCMessage in Nashesizer Hub
      //Serial2.println();
      newMessage = true;
      i = 0;
    } else {
      // compile the message from the buffer
      fromLiveOSCMessage += buf[i];
      //Serial2.println(fromLiveOSCMessage);
      i += 1;
    }
  }
}

void handleMessage() {
  // do something with the message - here just flash LED
  //  for (int i = 0; i < 10; i++) { // make this an odd number to leave the LED toggled
  //    digitalWrite(ledPin,!(digitalRead(ledPin))); // toggle LED
  //    delay(100);
  //  }
  //echoMessage();

  //// debugging
  //Serial2.println(fromLiveOSCMessage);

  // extracts values from fromLiveOSCMessage and updates the trkSettings array
  // messages from the Live parameters - Volume, Panning, SendA-SendF - are always of the form /T2/Panning,f,0.2777778
  // TO DO - clips names etc. won't use this format - need to check for type
  // indexes for the various delimiters
  ind1 = fromLiveOSCMessage.indexOf('T');  // finds location of the T
  ind2 = fromLiveOSCMessage.indexOf('/', ind1 + 1 ); // finds location of second /
  ind3 = fromLiveOSCMessage.indexOf(',', ind2 + 1 ); // finds location of first ,
  ind4 = fromLiveOSCMessage.indexOf(',', ind3 + 1 ); // finds location of second ,
  // extrcta the the track # to set the column position in the array
  liveTrkNo = fromLiveOSCMessage.substring(ind1 + 1, ind2).toInt();
  //// debugging
  //    Serial2.print("liveTrkNoC: ");
  //    Serial2.println(liveTrkNoC);
  // the parameter type
  paramType = fromLiveOSCMessage.substring(ind2 + 1, ind3);
  //// debugging
  //  Serial2.print("paramType: ");
  //  Serial2.println(paramType);
  // sets the parameter type row position in the array
  if (paramType == "Volume") {
    arrayColIndex = 0;
  } else if (paramType == "Panning") {
    arrayColIndex = 1;
  } else if (paramType == "SendA") {
    arrayColIndex = 2;
  } else if (paramType == "SendB") {
    arrayColIndex = 3;
  }
//  // debugging
//    Serial2.print("arrayColIndex: ");
//    Serial2.println(arrayColIndex);
  // extracts the parameter value
  paramValue = fromLiveOSCMessage.substring(ind4 + 1, fromLiveOSCMessage.length()).toFloat();
  //// debugging
  //    Serial2.print("paramValue: ");
  //    Serial2.println(paramValue);
  // writes the value into the array
  trkSettingsLive[liveTrkNo - 1][arrayColIndex] = paramValue;

  /*
     TO DO
     - use this array to send updated settings to rotary encoder module
     - later to motorised fader module - and differentiate between them
  */

  // if the track # of the changed parameter in Live's interface is the same as the currently active track # in the Nashesizer send data
  if (liveTrkNo == activeTrkNo) {
    Wire.beginTransmission(2);  // transmit to device #2
    char buf1[2];
    for (int i = 0; i < 2; i++) {
      dtostrf(arrayColIndex, 1, 0, buf1);
    }
    Wire.write(buf1);
    Wire.write(',');
    char buf2[6];
    for (int i = 0; i < 6; i++) {
      dtostrf(paramValue, 4, 6, buf2);  //4 is mininum width, 6 is precision
    }
    Wire.write(buf2);
    Wire.write(':');
    Wire.endTransmission();     // stop transmitting
  } else {
    // currently do nothing - but need to update if the active track # is changed and a chage has been made in Live
  }

  newMessage = false; // ready for next messaage
}

//void echoMessage() {
//  Serial.print(" echoing:- ");
//  //  int i = 0;
//  //  while (buf[i] != 10) {
//  //    Serial.write(buf[i]);
//  //    i += 1;
//  //  }
//  Serial.write(10);
//  //Serial.flush();
//}


///////////////////////////////////////
/// Nashesizer module functions
///////////////////////////////////////

void functionReadJoystick() {

  Wire.requestFrom(1, 1); // slave device #1 request 1 byte
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

void functionReadEncoders() {

  Wire.requestFrom(2, 2); // slave device #2 request 2 bytes
  while (Wire.available()) { // slave may send less than requested
    bEncNo = Wire.read(); // receive first byte as character
    bEnc = Wire.read(); // receive second byte as character
  }

  int bEncI = int(bEnc);
  float bEncF = mapif(bEncI, 0, 255, 0.0, 1.0);

  //  //debugging
  //  Serial.print(bEncNo, DEC); // print the byte
  //  Serial.print(":");
  //  Serial.print(bEnc, DEC); // print the byte
  //  Serial.println();
  //  Serial.print("bEncI: ");
  //  Serial.println(bEncI);
  //  Serial.print("bEncF: ");
  //  Serial.println(bEncF);

  /*
     TO DO - the two wired up rotary encoders for testing and their associated bEncNo currently coded to Volume and Panning
     - this will change when the other two encoders and  motorised faders are added
  */

  if (bEncNo == 0) { // Volume
    // compile the message
    String message = "/T";
    message += activeTrkNo;
    message += "/Volume,f,";
    message += bEncF;
    // print only if it's different to the last message
    if (message != messageLast) {
      Serial.println(message);
    }
    messageLast = message;
  } else if (bEncNo == 1) { // Panning
    // compile the message
    String message = "/T";
    message += activeTrkNo;
    message += "/Panning,f,";
    message += bEncF;
    // print only if it's different to the last message
    if (message != messageLast) {
      Serial.println(message);
    }
    messageLast = message;
  }

  goTime2 = millis() + nextTime2;

}

///////////////////////////////////////
/// Misc functions
///////////////////////////////////////

// inbuilt map function is integer only - this takes integer inputs and outputs floats
float mapif(int x, int in_min, int in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// takes float inputs and outputs integers
float mapfi(float x, float in_min, float in_max, int out_min, int out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

