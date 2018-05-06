// Code for the joystick module of the Nashesizer - an APEM 1000 Series Part No. 100113 joystick, 4 x NeoPixel minis and an Arduino Nano + passive components
// Wired up according to the orginal schematic worked up by Mike Cook for the prototype version of the Nashesizer using the 'Alternate Switch Joystick' wiring
// Requires the Wire, Adafruit_NeoPixel and StopWatch libraries.
// Coded in Arduino IDE 1.8.5 and Teensyduino 1.4.0


///////////////////////////////////////
/// LIBRARIES + GLOBALS
///////////////////////////////////////

#include <Wire.h> // I2C
#include <Adafruit_NeoPixel.h>
#include <StopWatch.h>

/// Wire configuration
int clockFrequency = 400000;

/// Neopixels
#ifdef __AVR__
#include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 4
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 4

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

/// joystick
#define LRPin A0
#define UDPin A1
int sensorValue[2] = {0, 0}; // variable array to store the values read from the joystick's analogue inputs
byte joystick; // joystick position as least significant bits within a single byte - requested via I2C by Teensy 3.6
/*
   B1000 - UP
   B0100 - DOWN
   B0010 - LEFT
   B0001 - RIGHT
   B00000000 - CENTRE
*/

/// StopWatch - for LED colour change on joystick hold
StopWatch jsHoldSW, jsHoldDebugSW;


///////////////////////////////////////
/// SET UP
///////////////////////////////////////

void setup() {

  // define A0 & A1 as inputs
  pinMode(LRPin, INPUT);
  pinMode(UDPin, INPUT);

  Serial.begin(9600);
  Wire.begin(1); // join i2c bus with address #1
  Wire.setClock(clockFrequency);
  Wire.onRequest(requestEvent); // register event - function below

  pixels.begin(); // This initializes the NeoPixel library.
  pixels.show(); // Initialize all pixels to 'off'

  /// flash all LEDs on start up
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 32, 0)); // green
    pixels.show(); // sends the updated pixel color to the hardware.
  }
  delay(500);
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // off
    pixels.show();
  }
  delay(500);

}


///////////////////////////////////////
/// MAIN LOOP
///////////////////////////////////////

void loop() {

  // read the value from the joystick:
  sensorValue[0] = analogRead(LRPin);
  sensorValue[1] = analogRead(UDPin);

//  // debugging - to read sensorValue[0] and sensorValue[1] in Serial Monitor
//    jsHoldDebugSW.start();
//    if (jsHoldDebugSW.elapsed() > 500) {
//      Serial.print("sensorValue[0]: ");
//      Serial.println(sensorValue[0]);
//      Serial.print("sensorValue[1]: ");
//      Serial.println(sensorValue[1]);
//      jsHoldDebugSW.stop();
//      jsHoldDebugSW.reset();
//    }

  /*
    centre - sensorValue[0] = 622; sensorValue[1] = 622;
    up - sensorValue[0] = 594; sensorValue[1] = 1023;
    down - sensorValue[0] = 608; sensorValue[1] = 0;
    left - sensorValue[0] = 1023; sensorValue[1] = 593;
    right - sensorValue[0] = 0; sensorValue[1] = 608;
  */
  /*
    NeoPixels - 0 - down, 1 - right, 2 - up, 3 - left
  */

  if (sensorValue[1] > 970) { // UP
    //Serial.println("UP");
    joystick = B1000;
    // use to StopWatch to turn LED from green to blue when held
    jsHoldSW.start();
    if (jsHoldSW.elapsed() < 250) {
      pixels.setPixelColor(2, pixels.Color(0, 32, 0));
      pixels.show();
    } else {
      pixels.setPixelColor(2, pixels.Color(0, 0, 32));
      pixels.show();
    }
  } else if (sensorValue[1] < 50) { // DOWN
    //Serial.println("DOWN");
    joystick = B0100;
    jsHoldSW.start();
    if (jsHoldSW.elapsed() < 250) {
      pixels.setPixelColor(0, pixels.Color(0, 32, 0));
      pixels.show();
    } else {
      pixels.setPixelColor(0, pixels.Color(0, 0, 32));
      pixels.show();
    }
    //digitalWrite(ledPin, HIGH);
  } else if (sensorValue[0] > 970) { // LEFT
    //Serial.println("LEFT");
    joystick = B0010;
    jsHoldSW.start();
    if (jsHoldSW.elapsed() < 250) {
      pixels.setPixelColor(3, pixels.Color(0, 32, 0));
      pixels.show();
    } else {
      pixels.setPixelColor(3, pixels.Color(0, 0, 32));
      pixels.show();
    }
  } else if (sensorValue[0] < 50) { // RIGHT
    //Serial.println("RIGHT");
    joystick = B0001;
    jsHoldSW.start();
    if (jsHoldSW.elapsed() < 250) {
      pixels.setPixelColor(1, pixels.Color(0, 32, 0));
      pixels.show();
    } else {
      pixels.setPixelColor(1, pixels.Color(0, 0, 32));
      pixels.show();
    }
  } else { // CENTRE
    joystick = B00000000;
    jsHoldSW.stop();
    jsHoldSW.reset();
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      pixels.show();
    }
  }
}

// function that executes whenever data is requested by I2C master
// this function is registered as an event, see setup()
void requestEvent() {
  Wire.write(joystick); // respond with message of 1 byte as expected by master
}
