// Working code for the base module of the Nashesizer
// Uses Teensy 3.6 with USB type 'All of the Above'
// Requires the Wire, Keyboard and StopWatch libraries.
// Coded in Arduino IDE 1.8.5 and Teensyduino 1.4.0
// This version integrates the joystick, motorised sliders, rotary encoders and buttons modules and I2C 2-way comms
// Forwards joystick positions as emulated keypresses
// Forwards slider positions into Live and also receives and forwards on changes to LiveGrabber assigned GUI controls in Live to the motorised sliders module...
// to update slider positions and their RGB LED strip colours
// Forwards encoder positions into Live and also receives and forwards on changes to LiveGrabber assigned GUI controls in Live to the rotary encoders module...
// to update encoder positions and their RGB LED colour
// Forwards button presses as either emulated keypresses or OSC message depending on mode and also receives and forwards on changes to LiveGrabber assigned GUI controls...
// in Live to the buttons module to update button and LED states
// I2C two-way comms tested and working (might yet be refined) from the master Teensy 3.6 from the joystick module and to/from the motorised sliders, rotary encoders and buttons modules'...
// slave Arduino Nano or Teensy 3.2 (downstream) and to/from the Processing Nashesizer-Hub OSC-to-Serial bridge to/from Live (upstream)
// Debugging Serial2.print() code commented out but left in for now
// Variable names are somewhat long but explicit to help readability of the code
// Latest schematic on GitHub


///////////////////////////////////////
/// LIBRARIES + GLOBALS
///////////////////////////////////////

#include <Wire.h> // I2C
#include "Keyboard.h" // Teensy as an emulated keyboard
#include <StopWatch.h>

/// Wire configuration
int clockFrequency = 400000;

// single byte requested from joystick module
byte jsB, jsBLast;
byte jsUp = B1000;
byte jsDown = B0100;
byte jsLeft = B0010;
byte jsRight = B0001;
boolean jsTrkChange = false;

// keyboard emulation
#define OSX 0
// change this to match your platform:
int platform = OSX;

/// StopWatch - used for joystick 'repeat' functionality
StopWatch jsHoldSW, msDelaySW;

// bytes requested from the motorised sliders module and variables
int sliderReadings [] = {0, 0, 0, 0};
int sliderReadingsLast [] = {0, 0, 0, 0};
// colours to use first 4 green for each slider, then red, then blue
byte slidePost[] = {20, 0, 0, 20,    0, 20, 0, 10,    0, 0, 20, 0 };
byte slide[] =     { 20, 20, 20, 20,   20, 20, 20, 20,   20, 20, 20, 20};
int jitter = 5; // there's a bit of 'jitter' on the slider readings
String toHubMSMessage, toHubMSMessageLast = "";
boolean newMSMessage = false;
boolean msMoving = false;
float sldValueF = 0.0;
int sldValueI, sliderMIDI = 0;

// bytes requested from the rotary encoders module and variables
byte encNoB, encValueB;
int currentTrkNo = 1;
// to compile OSC message from the received rotary encoders module bytes, check if the new message is the same as the last and only write this via Serial to the Nashesizer-Hub if it is
String toHubREMessage, toHubREMessageLast = "";
boolean newREMessage = false;

// bytes requested from the buttons module and variables
byte butNoB, butValueB, butEventTypeB;
int butNoI, butNoILast, butValueI, butValueILast, butEventTypeI, butEventTypeILast, buttonMode, buttonModeLast = 0;
int noButtonsAttached = 4;
boolean buttonToggle[4] = {0, 0, 0, 0}; // button state - on or off
// to compile OSC message from the received buttons module bytes, check if the new message is the same as the last and only write this via Serial to the Nashesizer-Hub if it is
String toHubBMessage, toHubBMessageLast = "";
boolean newBMessage, newBMode1Message = false;
boolean newTrkMode = false;

/// Serial comms from Processing Nashesizer-Hub OSC-to-Serial bridge
char buf[30];
boolean newHubIncoming = false;
// incoming OSC message from the Nashesizer Hub
String fromHubMessage = "";

// 2D array to store track parameter - Volume, Panning, SendA-F, Solo abd Mute updates from Live tracks
// the Nashesizer default Live project will have a total of 24 tracks - 16 audio + 8 MIDI - with LiveGrabber devices preloaded
// TO DO - this doesn't currently accomodate the return and master tracks - an additonal 7 columns
int trkSettings [24][10];
int trkNo, paramNo = 0;
String paramType = "";
float paramValueF = 0.0;
int paramValueI = 0;
int ind1, ind2, ind3, ind4; // Hub message delimiter locations

// for rolling polling of I2C bus
int nextTime1  = 20; // do this every 20 milliseconds - joystick
int nextTime2  = 10; // do this every 10 milliseconds - encoder
int nextTime3  = 10; // do this every 20 milliseconds - buttons
int nextTime4  = 10; // do this every 10 milliseconds - sliders
long int goTime1, goTime2, goTime3, goTime4;


///////////////////////////////////////
/// SET UP
///////////////////////////////////////

void setup() {

  Wire.begin(0); // join i2c bus (address optional for master)
  Wire.setClock(clockFrequency);
  Keyboard.begin();
  Serial.begin(250000); // start Serial for USB output
  // enabling a second hardware Serial port to print debugging info from the Teensy via an Adafruit FTDI Friend and CoolTerm Serial Console
  // while its also connected to the Nashesizer Hub via USB
  Serial2.begin(230400);
  Serial2.println("Serial2 enabled...");

  // allocate a buffer in memory for manipulating Strings
  // TO DO - Mike Cook recommends that Arduino String should be avoided because of lack of garbage collection - but String manipulation is far more straightforward to use
  // and using reserve minimises fragmentation of the heap according to this Adafruit guide on Optimising SRAM - https://learn.adafruit.com/memories-of-an-arduino/optimizing-sram
  toHubREMessage.reserve(25);
  toHubREMessageLast.reserve(25);
  toHubBMessage.reserve(25);
  toHubBMessageLast.reserve(25);
  toHubMSMessage.reserve(25);
  toHubMSMessageLast.reserve(25);
  fromHubMessage.reserve(25);
  paramType.reserve(8);

  // initialise trkSettings[][] array
  initTrkSettings();

}


///////////////////////////////////////
/// MAIN LOOP
///////////////////////////////////////

void loop() {

  if (Serial.available() > 0) readHubIncoming();
  if (newHubIncoming) handleHubIncoming();

  // rolling polling of I2C slave modules
  if (millis() >= goTime1) readJoystick();

  if (jsTrkChange == true) {
    updateREM();
    updateBM();
    updateMSM();
    msDelaySW.stop();
    msDelaySW.reset();
    msDelaySW.start();
    //updateMSM();
    //readSliders();
    //delay(400);
    //readSliders();
    jsTrkChange = false;
  }

  if (msDelaySW.elapsed() <= 500) {
    msMoving = true;
  } else {
    msMoving = false;

  }
  //  Serial2.print("msMoving: ");
  //  Serial2.println(msMoving);

  if (millis() >= goTime4) {
    if (msMoving == false) readSliders();
  }
  
  //readSliders();

  if (millis() >= goTime2) readEncoders();

  if (millis() >= goTime3) readButtons();

  if (newBMessage == true) {
    handleButtonsMessage();
    newBMessage = false;
  }

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

