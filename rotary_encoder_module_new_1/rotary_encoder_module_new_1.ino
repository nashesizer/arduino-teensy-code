// Code for the 4 x rotary encoder module of the Nashesizer
// Uses EC12PLRGBSDVBF-D-25K-24-24C-6108-6H rotary encoders with in- built RGB LED from Pimoroni - https://shop.pimoroni.com/products/rotary-encoder-illuminated-rgb + Teensy 3.2 + passive components
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

  Serial.begin(9600);

  Wire.begin(2); // join i2c bus with address #2
  Wire.onRequest(requestEvent); // register event - function below

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
        rgbLedRE[i]->writeRGB(0, map(encoderInt[i], 0, 255, 10, 255), 255 - map(encoderInt[i], 0, 255, 10, 255));
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
