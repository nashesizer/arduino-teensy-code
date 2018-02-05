// Code for the 4 x rotary encoder module of the Nashesizer
// Uses EC12PLRGBSDVBF-D-25K-24-24C-6108-6H rotary encoders with in-built RGB LED from Pimoroni - https://shop.pimoroni.com/products/rotary-encoder-illuminated-rgb...
// + Teensy 3.2 + passive components
// I2C two-way comms tested and working (though not yet fully implemented) via the module's slave Arduino Nano to/from the master Teensy 3.6 to/from the Processing...
// Nashesizer-Hub OSC-to-Serial bridge to/from Live (upstream)
// Sends encoder position into Live and also updates encoder positions and their RGB LED colour when a change is made to a LiveGrabber assigned GUI control in Live
// Schematic still being finalised
// Requires the Wire, Encoder and RGBLED libraries.
// Coded in Arduino IDE 1.8.5 and Teensyduino 1.4.0

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
char buf[20];
boolean newMessage = false;
String fromTeensyMessage = "";
int encNo, encValue = 0;
float encValueF = 0.0;

/// rotary encoders
// pointer to pointer - enables less repetion in the code via for loops using multiple pointers to
// Encoder objects of the form rotEnc[i] - these declared in setup();
Encoder** rotEnc;
byte encoderByte[4] = {B00000000, B00000000, B00000000, B00000000};
int encoderInt[4] = {0, 0, 0, 0};
int encoderRead[4], encoderReadLast[4] = {0, 0, 0, 0};
int lastEncoderUsed = 0;
int noAttachedEncoders = 2;

// coarse + fine resolution modes
int resMode = 0;

/// RGB LEDs
RGBLED** rgbLedRE; // pointer to pointer - as per Encoder

// simple timer for printing to Serial Monitor
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

  // Change these two numbers to the pins connected to your encoder.
  //   Best Performance: both pins have interrupt capability
  //   Good Performance: only the first pin has interrupt capability
  //   Low Performance:  neither pin has interrupt capability
  //   avoid using pins with LEDs attached
  rotEnc = new Encoder*[noAttachedEncoders];
  rotEnc[0] = new Encoder(7, 8);
  rotEnc[1] = new Encoder(11, 12);

  // Declare an RGBLED instanced named rgbLed
  // Red, Green and Blue LED legs are connected to PWM pins 11,9 & 6 respectively
  // In this example, we have a COMMON_ANODE LED, use COMMON_CATHODE otherwise
  rgbLedRE = new RGBLED*[noAttachedEncoders];
  rgbLedRE[0] = new RGBLED(3, 4, 5, COMMON_ANODE);
  rgbLedRE[1] = new RGBLED(21, 22, 23, COMMON_ANODE);

  // flash the encoder LEDs green at power up
  for (int i = 0; i < noAttachedEncoders; i++) {
    rgbLedRE[i]->writeRGB(0, 255, 0);
    delay(500);
    rgbLedRE[i]->writeRGB(0, 0, 0);
    delay(500);
  }

  currentTime = millis();
  lastTime = currentTime;

}


///////////////////////////////////////
/// MAIN LOOP
///////////////////////////////////////

void loop() {

  if (newMessage) handleMessage(); // do something with message from the Teensy

  currentTime = millis();

  // read the encoder values
  for (int i = 0; i < noAttachedEncoders; i++) {
    encoderRead[i] = rotEnc[i]->read();
  }

  // check which encoder was used last
  for (int i = 0; i < noAttachedEncoders; i++) {
    if (encoderRead[i] != encoderReadLast[i]) {
      lastEncoderUsed = i;
    }
    encoderReadLast[i] = encoderRead[i];
  }

  if (resMode == 0) { // default coarse mode

    // limit the encoder values to between 0 & 255
    for (int i = 0; i < noAttachedEncoders; i++) {
      if (encoderRead[i] > 255) {
        encoderInt[i] = 255;
      } else if (encoderRead[i] < 0) {
        encoderInt[i] = 0;
      } else {
        encoderInt[i] = encoderRead[i];
      }
      encoderByte[i] = byte(encoderInt[i]);
    }

    //    // debugging
    //    if (currentTime - lastTime >= delayTime) {
    //      for (int i = 0; i < noAttachedEncoders; i++) {
    //        Serial.print("encoderRead[");
    //        Serial.print(i);
    //        Serial.print("]:" );
    //        Serial.println(encoderRead[i]);
    //        Serial.print("encoderInt[");
    //        Serial.print(i);
    //        Serial.print("]:" );
    //        Serial.println(encoderInt[i]);
    //        Serial.print("encoderByte[");
    //        Serial.print(i);
    //        Serial.print("]:" );
    //        Serial.println(encoderByte[i]);
    //      }
    //      Serial.print("lastEncoderUsed: ");
    //      Serial.println(lastEncoderUsed);
    //      lastTime = currentTime;  // Updates lastTime
    //    }

    // adjust the colour of the built-in RGB LED
    // red for below 0 and above 255, blue (0) to green (255)
    for (int i = 0; i < noAttachedEncoders; i++) {
      if (encoderRead[i] > 255) {
        rgbLedRE[i]->writeRGB(30, 0, 0);
      } else if (encoderRead[i] < 0) {
        rgbLedRE[i]->writeRGB(30, 0, 0);
      } else {
        rgbLedRE[i]->writeRGB(0, encoderInt[i], 255 - encoderInt[i]);
      }
    }

  } else if (resMode == 1) { // fine mode

  }

}

///////////////////////////////////////
/// Nashesizer module functions
///////////////////////////////////////

// function that executes whenever data is requested by I2C master
// this function is registered as an event, see setup()
void requestEvent() {
  
  Wire.write(byte(lastEncoderUsed)); // respond with message of 1 byte as expected by master
  Wire.write(encoderByte[lastEncoderUsed]); // respond with message of 1 byte as expected by master
  
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {

  for ( int i = 0; i < sizeof(buf);  ++i ) buf[i] = (char)0; // clear the buffer
  fromTeensyMessage = ""; // reset the message
  static int i = 0;
  while (Wire.available() > 0 && newMessage == false) { // loop through all but the last byte
    //// debugging
    //char c = Wire.read(); // receive byte as a character
    //Serial.print(c); // print the character
    buf[i] = Wire.read();
    //Serial.print(buf[i]); // debugging
    if (buf[i] == ':') { // added ':' delimiter to end of I2C writes from Teensy
      newMessage = true;
      i = 0;
    } else {
      // compile the message from the buffer
      fromTeensyMessage += buf[i];
      //Serial2.println(fromTeensyMessage); // debugging
      i += 1;
    }
  }
  
}


void handleMessage() {
  
  // do something with the message
  //Serial.println(fromTeensyMessage);
  // extracts values from fromTeensyMessage and updates the relevant encoder position and RGB LED colour
  // messages from the Teensy are always of the form 2,0.620000
  int ind1 = fromTeensyMessage.indexOf(',');  // finds location of the ,
  // extracts the encoder #
  encNo = fromTeensyMessage.substring(0, ind1).toInt();
  // extracts the encoder value
  encValueF = fromTeensyMessage.substring(ind1 + 1, fromTeensyMessage.length()).toFloat();
  // convert this float to an integer
  encValue = mapfi(encValueF, 0.0, 1.0, 0, 255);
  //  // debugging
  //  Serial.print("encNo: ");
  //  Serial.println(encNo);
  //  Serial.print("encValueF: ");
  //  Serial.println(encValueF);
  //  Serial.print("encValue: ");
  //  Serial.println(encValue);
  // write the value to the encoder
  rotEnc[encNo]->write(encValue);
  // update it's RGB LED
  rgbLedRE[encNo]->writeRGB(0, encValue, 255 - encValue);

  newMessage = false; // ready for next messaage
  
}

///////////////////////////////////////
/// Misc functions
///////////////////////////////////////

// inbuilt map function is integer only - this takes integer inputs and outputs floats
float mapif(int x, int in_min, int in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// takes float inputs and outputs integers
float mapfi(float x, float in_min, float in_max, int out_min, int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
