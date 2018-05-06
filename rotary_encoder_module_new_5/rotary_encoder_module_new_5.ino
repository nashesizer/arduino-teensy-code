// Working code for the 4 x rotary encoder module of the Nashesizer - includes commented out workings - to tidy up
// Uses EC12PLRGBSDVBF-D-25K-24-24C-6108-6H rotary encoders with in-built RGB LED from Pimoroni - https://shop.pimoroni.com/products/rotary-encoder-illuminated-rgb...
// + Arduino Nano + passive components
// Requires the Wire, Encoder and RGBLED libraries.
// Coded in Arduino IDE 1.8.5 and Teensyduino 1.4.0
// I2C two-way comms tested and working (might yet be refined) via the module's slave Arduino Nano to/from the master Teensy 3.6 to/from the Processing...
// Nashesizer-Hub OSC-to-Serial bridge to/from Live (upstream)
// Sends encoder position into Live and also updates encoder positions and their RGB LED colour when a change is made to a LiveGrabber assigned GUI control in Live
// Variable names are somewhat long but explicit to help readability of the code
// Schematic still being finalised - current version in root folder


///////////////////////////////////////
/// LIBRARIES + GLOBALS
///////////////////////////////////////

#include <Wire.h> // I2C
#include <Encoder.h>
/*
  RGBLedExample
  Example for the RGBLED Library
  Created by Bret Stateham, November 13, 2014
  You can get the latest version from http://github.com/BretStateham/RGBLED
*/
#include <RGBLED.h>

/// Wire configuration
int clockFrequency = 400000;

// data sent from Teensy 3.6
//char buf[20];
boolean newFromTeensyMessage = false;
//String fromTeensyMessage = "";
//int encNo, encValue = 0;
//float encValueF = 0.0;
byte paramNoB, encValueInB = B00000000;
int paramNoI, encNoInI, encValueInI = 0;


/// rotary encoders
// pointer to pointer - enables less repetion in the code via for loops using multiple pointers to
// Encoder objects of the form rotEnc[i] - these declared in setup();
Encoder** rotEnc;
int encRead[4], encReadLast[4] = {0, 0, 0, 0};
int encI[4] = {0, 0, 0, 0};
byte encB[4] = {B00000000, B00000000, B00000000, B00000000};
int lastEncUsed, lastEncUsedTemp = 0;
int noEncsAttached = 4;
boolean physicalUpdate = false;

// coarse + fine resolution modes
int resMode = 0;

/// RGB LEDs
RGBLED** rgbLedRE; // pointer to pointer - as per Encoder

// simple timer for printing debugging info to Serial Monitor
unsigned long currentTime;
unsigned long lastTime;
unsigned long delayTime = 200;


///////////////////////////////////////
/// SET UP
///////////////////////////////////////

void setup() {

  Serial.begin(250000);

  Wire.begin(2); // join i2c bus with address #2
  Wire.setClock(clockFrequency);
  Wire.onRequest(requestEvent); // register event - function below
  Wire.onReceive(receiveEvent); // register event - function below

  // allocate a buffer in memory for manipulating Strings
  // TO DO - Mike Cook recommends that Arduino String should be avoided because of lack of garbage collection - but String manipulation is far more straightforward to use
  // and using reserve minimises fragmentation of the heap according to this Adafruit guide on Optimising SRAM - https://learn.adafruit.com/memories-of-an-arduino/optimizing-sram
  // now replaced this method with a simpler reading of two bytes
  //fromTeensyMessage.reserve(12);

  // Change these two numbers to the pins connected to your encoder.
  //   Best Performance: both pins have interrupt capability
  //   Good Performance: only the first pin has interrupt capability
  //   Low Performance:  neither pin has interrupt capability
  //   avoid using pins with LEDs attached
  rotEnc = new Encoder*[noEncsAttached];
  rotEnc[0] = new Encoder(7, 8);
  rotEnc[1] = new Encoder(12, 24);
  rotEnc[2] = new Encoder(27, 28);
  rotEnc[3] = new Encoder(29, 30);

  // Declare an RGBLED instanced named rgbLed
  // Red, Green and Blue LED legs are connected to PWM pins 11,9 & 6 respectively
  // In this example, we have a COMMON_ANODE LED, use COMMON_CATHODE otherwise
  rgbLedRE = new RGBLED*[noEncsAttached];
  rgbLedRE[0] = new RGBLED(3, 4, 5, COMMON_ANODE);
  rgbLedRE[1] = new RGBLED(6, 9, 10, COMMON_ANODE);
  rgbLedRE[2] = new RGBLED(25, 32, 20, COMMON_ANODE);
  rgbLedRE[3] = new RGBLED(21, 22, 23, COMMON_ANODE);

  // flash the encoder LEDs green at power up
  for (int i = 0; i < noEncsAttached; i++) {
    rgbLedRE[i]->writeRGB(0, 255, 0);
  }
  delay(500);
  for (int i = 0; i < noEncsAttached; i++) {
    rgbLedRE[i]->writeRGB(0, 0, 0);
  }
  delay(500);

  currentTime = millis();
  lastTime = currentTime;

}


///////////////////////////////////////
/// MAIN LOOP
///////////////////////////////////////

void loop() {

  // simple timer
  currentTime = millis();

  // recive message from Teensy I2C master
  if (newFromTeensyMessage) handleTeensyMessage(); // do something with message from the Teensy

  // read the encoder values
  for (int i = 0; i < noEncsAttached; i++) {
    encRead[i] = rotEnc[i]->read();
  }

  // check which encoder was used last
  for (int i = 0; i < noEncsAttached; i++) {
    if (encRead[i] != encReadLast[i]) {
      physicalUpdate = true;
      lastEncUsedTemp = i;
    }
    encReadLast[i] = encRead[i];
  }

  // limit the encoder values to between 0 & 255
  for (int i = 0; i < noEncsAttached; i++) {
    if (encRead[i] > 255) {
      encI[i] = 255;
    } else if (encRead[i] < 0) {
      encI[i] = 0;
    } else {
      encI[i] = encRead[i];
    }
    encB[i] = byte(encI[i]);
  }

//      // debugging
//      if (currentTime - lastTime >= delayTime) {
//  //      for (int i = 0; i < noEncsAttached; i++) {
//  //        Serial.print("encRead[");
//  //        Serial.print(i);
//  //        Serial.print("]:" );
//  //        Serial.println(encRead[i]);
//  //        Serial.print("encI[");
//  //        Serial.print(i);
//  //        Serial.print("]:" );
//  //        Serial.println(encI[i]);
//  //        Serial.print("encB[");
//  //        Serial.print(i);
//  //        Serial.print("]:" );
//  //        Serial.println(encB[i]);
//  //      }
//        Serial.print("lastEncUsed: ");
//        Serial.println(lastEncUsed);
//        lastTime = currentTime;  // Updates lastTime
//      }

  // sets the last encoder used - either physically or via OSC
  if (physicalUpdate == true) {
    lastEncUsed = lastEncUsedTemp;
  } else if (physicalUpdate == false) {
    lastEncUsed = encValueInI;
  }

  if (resMode == 0) { // default coarse mode

    // adjust the colour of the built-in RGB LED
    // red for below 0 and above 255, blue (0) to green (255)
    for (int i = 0; i < noEncsAttached; i++) {
      if (encRead[i] > 255) {
        rgbLedRE[i]->writeRGB(30, 0, 0);
      } else if (encRead[i] < 0) {
        rgbLedRE[i]->writeRGB(30, 0, 0);
      } else {
        rgbLedRE[i]->writeRGB(0, encI[i], 255 - encI[i]);
      }
    }

  } else if (resMode == 1) { // fine mode

    // TO DO - this likely activated from the buttons module and/or 7" touchscreen display
    // - yet to implement

  }

}

///////////////////////////////////////
/// Nashesizer module functions
///////////////////////////////////////

// function that executes whenever data is requested by Teensy I2C master
// this function is registered as an event, see setup()
void requestEvent() {

  Wire.write(byte(lastEncUsed)); // respond with message of 1 byte as expected by master
  Wire.write(encB[lastEncUsed]); // respond with message of 1 byte as expected by master

}

// function that executes whenever data is received from Teensy I2C master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {

  // worked - but simplifying to just reading 2 incoming bytes
  //  for ( int i = 0; i < sizeof(buf);  ++i ) buf[i] = (char)0; // clear the buffer
  //  fromTeensyMessage = ""; // reset the message
  //  static int i = 0;
  //  while (Wire.available() > 0 && newFromTeensyMessage == false) { // loop through all but the last byte
  //    //// debugging
  //    //char c = Wire.read(); // receive byte as a character
  //    //Serial.print(c); // print the character
  //    buf[i] = Wire.read();
  //    //Serial.print(buf[i]); // debugging
  //    if (buf[i] == ':') { // added ':' delimiter to end of I2C writes from Teensy
  //      newFromTeensyMessage = true;
  //      i = 0;
  //    } else {
  //      // compile the message from the buffer
  //      fromTeensyMessage += buf[i];
  //      //Serial2.println(fromTeensyMessage); // debugging
  //      i += 1;
  //    }
  //  }

  while (Wire.available()) { // slave may send less than requested
    paramNoB = Wire.read(); // receive first byte as character
    paramNoI = int(paramNoB);
    // the Teensy sends the current parameter # - this has to be mapped to a specific encoder
    if (paramNoI == 1) { // Panning
      encNoInI = 3;
    } else if (paramNoI == 5) { // SendD
      encNoInI = 2;
    } else if (paramNoI == 6) { // SendE
      encNoInI = 1;
    } else if (paramNoI == 7) { // SendF
      encNoInI = 0;
    }

    encValueInB = Wire.read(); // receive second byte as character
    encValueInI = encValueInB;
    lastEncUsed = encValueInI;
    physicalUpdate = false;
    newFromTeensyMessage = true;
  }

}


void handleTeensyMessage() {

  // do something with the message

//  // debugging
//  Serial.print("paramNoI: ");
//  Serial.println(paramNoI);

  //  // worked - but simplifying to just reading 2 incoming bytes
  //  //Serial.println(fromTeensyMessage);
  //  // extracts values from fromTeensyMessage and updates the relevant encoder position and RGB LED colour
  //  // messages from the Teensy are always of the form 2,0.620000
  //  int ind1 = fromTeensyMessage.indexOf(',');  // finds location of the ,
  //  // extracts the encoder #
  //  encNo = fromTeensyMessage.substring(0, ind1).toInt();
  //  // extracts the encoder value
  //  encValueF = fromTeensyMessage.substring(ind1 + 1, fromTeensyMessage.length()).toFloat();
  //  // convert this float to an integer
  //  encValue = mapfi(encValueF, 0.0, 1.0, 0, 255);
  //  //  // debugging
  //  //  Serial.print("encNo: ");
  //  //  Serial.println(encNo);
  //  //  Serial.print("encValueF: ");
  //  //  Serial.println(encValueF);
  //  //  Serial.print("encValue: ");
  //  //  Serial.println(encValue);
  //  // write the value to the encoder
  //  rotEnc[encNo]->write(encValue);
  //  // update it's RGB LED
  //  rgbLedRE[encNo]->writeRGB(0, encValue, 255 - encValue);

  // write the value to the encoder
  rotEnc[encNoInI]->write(encValueInI);
  // update it's RGB LED
  rgbLedRE[encNoInI]->writeRGB(0, encValueInI, 255 - encValueInI);

  newFromTeensyMessage = false; // ready for next messaage

}

///////////////////////////////////////
/// Misc functions
///////////////////////////////////////

// no longer used
//// takes float inputs and outputs integers
//float mapfi(float x, float in_min, float in_max, int out_min, int out_max) {
//  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
//}
