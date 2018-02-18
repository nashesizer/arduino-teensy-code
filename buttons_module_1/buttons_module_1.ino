// Working code for the 4 x buttons module of the Nashesizer - includes some commented out workings and debugging - to tidy up
// Uses Ultralux RGB illuminated arcade button with in-built RGB LED and D4XX Cherry micro-switch from Arcade World UK - https://www.arcadeworlduk.com/products/ultralux-rgb-illuminated-arcade-button.html...
// + Arduino Nano + passive components
// Requires the Wire, AceButton, SoftPWM and RGBLED libraries
// Coded in Arduino IDE 1.8.5 and Teensyduino 1.4.0
// Button functionality and RGB LEDs using SoftPWM on non-PWM Nano pins tested and working (might yet be refined). 
// Only two test modes currently implemented - the default Live transport mode (buttons 2-4 to activate play, stop and record) and track mode (buttons 3-4 to activate solo and mute in a currently selected track)
// I2C two-way comms not yet implemented via the module's slave Arduino Nano to/from the master Teensy 3.6 to/from the Processing...
// Nashesizer-Hub OSC-to-Serial bridge to/from Live (upstream)
// Will send button presses into Live and also update RGB LED status colours when associated events are triggered in Live or a different mode is selected via the 7" touchscreen display and it's Processing controlP5 GUI
// Variable names are somewhat long but explicit to help readability of the code
// Schematic still being finalised - current version in 'schematics' folder


///////////////////////////////////////
/// LIBRARIES + GLOBALS
///////////////////////////////////////

// list all libraries first
#include <Wire.h> // I2C

/// AceButton library
#include <AceButton.h>
#include <AdjustableButtonConfig.h>

/* SoftPWM by bhagman - https://github.com/bhagman/SoftPWM */
#include <SoftPWM.h>

/*
  RGBLedExample
  Example for the RGBLED Library
  Created by Bret Stateham, November 13, 2014
  You can get the latest version from http://github.com/BretStateham/RGBLED
*/
#include <RGBLED.h>

/// Wire configuration
int clockFrequency = 400000;

/// AceButton
// pointer to pointer - enables less repetion in the code via for loops using multiple pointers to
// button objects of the form button[i] - these declared in setup();
using namespace ace_button;
AceButton** button;
ButtonConfig buttonConfig;
AdjustableButtonConfig adjustableButtonConfig;

/// RGB LEDs
RGBLED** rgbLedRE; // pointer to pointer - as per button
//int nPins[6] = {3,5,6,7,8,9,10,11,12,13,14,15}; // TO DO - test to assign pins to RGLED objects in for() loop?
// TO DO - switch from using pin 13 to pin A2 because of the in-built LED se on start up

// buttons
int noButtonsAttached = 4;
int buttonMode = 0;
int noModes = 5;
boolean buttonToggle[4] = {0, 0, 0, 0}; // button state - on or off
boolean buttonAction = true; // has a button press been detected - only update RGB LEDs and SoftPWM if it has

// data sent from Teensy 3.6
//char buf[20];
boolean newFromTeensyMessage = false;

// simple timer for printing debugging info to Serial Monitor
unsigned long currentTime;
unsigned long lastTime;
unsigned long delayTime = 20;


///////////////////////////////////////
/// SET UP
///////////////////////////////////////

void setup() {

  Serial.begin(250000);

  Wire.begin(3); // join i2c bus with address #3
  Wire.setClock(clockFrequency);
  Wire.onRequest(requestEvent); // register event - function below
  Wire.onReceive(receiveEvent); // register event - function below

  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  pinMode(4, INPUT);

  /// AceButton
  // Configure the ButtonConfig with the event handler and enable LongPress an RepeatPress.
  // SupressAfterLongPress is optional since we don't do anything if we get it.
  adjustableButtonConfig.setEventHandler(handleEvent);
  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  adjustableButtonConfig.setLongPressDelay(1000);
  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterRepeatPress);
  adjustableButtonConfig.setRepeatPressInterval(1000);

  // just using the above for now - but could include the following features from demo sketch
  //  // Configure the ButtonConfig with the event handler, and enable all higher level events.
  //  buttonConfig.setEventHandler(handleEvent);
  //  buttonConfig.setFeature(ButtonConfig::kFeatureClick);
  //  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  //  buttonConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
  //  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);

  button = new AceButton*[noButtonsAttached];
  for (int i=0; i<noButtonsAttached; i++) {
    button[i] = new AceButton(i, HIGH, i /* id */);
    button[i]->setButtonConfig(&adjustableButtonConfig);
  }

  // SoftPWM Library
  SoftPWMBegin(SOFTPWM_INVERTED); // this works empirically - suspect because RGB LEDs are common anode type

  // Declare an RGBLED instanced named rgbLed
  // Red, Green and Blue LED legs are connected to PWM pins 11,9 & 6 respectively
  // In this example, we have a COMMON_ANODE LED, use COMMON_CATHODE otherwise
  rgbLedRE = new RGBLED*[noButtonsAttached];
  
  rgbLedRE[0] = new RGBLED(3, 5, 6, COMMON_ANODE);
  rgbLedRE[1] = new RGBLED(7, 8, 9, COMMON_ANODE);
  rgbLedRE[2] = new RGBLED(10, 11, 12, COMMON_ANODE);
  rgbLedRE[3] = new RGBLED(13, A0, A1, COMMON_ANODE);

  // replace above with this - to test
//  for (int i=0; i<noButtonsAttached; i++) {
//    rgbLedRE[i] = new RGBLED(nPins[4*i], nPins[4*i+1], nPins[4*i+2], COMMON_ANODE);
//  }

  // flash the button LEDs green at power up
  // some LEDs are are non-PWM pins so use SoftPWM library - though values of 0 and 255 via RGBLED library are intepreted as LOW and HIGH respectively
  for (int i = 0; i < noButtonsAttached; i++) {
    rgbLedRE[i]->writeRGB(0, 255, 0);
  }
  delay(500);
  for (int i = 0; i < noButtonsAttached; i++) {
    rgbLedRE[i]->writeRGB(0, 0, 0);
  }
  delay(500);

  // iniatilses the button LEDs to default 'Live transport' mode
  setLEDs();

  // simple timer
  currentTime = millis();
  lastTime = currentTime;

}


///////////////////////////////////////
/// MAIN LOOP
///////////////////////////////////////

void loop() {

  // simple timer
  currentTime = millis();

  // receive message from Teensy I2C master
  if (newFromTeensyMessage) handleTeensyMessage(); // do something with message from the Teensy

  // poll the buttons for presses
  if (currentTime - lastTime >= delayTime) {
    for (int i = 0; i < noButtonsAttached; i++) {
      button[i]->check();
    }
    lastTime = currentTime;  // Updates lastTime
  }

  // debugging
  //// test SoftPWM controlled LED
  //  SoftPWMSet(13, 255);
  //  SoftPWMSet(A0, 255);
  //  SoftPWMSet(A1, 255);
  //  delay(1000);
  //  SoftPWMSet(13, 128);
  //  SoftPWMSet(A0, 128);
  //  SoftPWMSet(A1, 128);
  //  delay(1000);
  //  SoftPWMSet(13, 0);
  //  SoftPWMSet(A0, 0);
  //  SoftPWMSet(A1, 0);
  //  delay(1000);

  // only need to set LED colours on button press - otherwise it's updating every loop and causes flicker
  if (buttonAction == true) {
    setLEDs();
    buttonAction = false;
  } else {
  }

}



///////////////////////////////////////
/// button LEDs
///////////////////////////////////////

void setLEDs() {

  if (buttonMode == 0) { // default mode - transport

    // the mode change button
    rgbLedRE[0]->writeRGB(255, 255, 255); // white
    // sets the colours of the other buttons when pressed - dependent on mode
    // Live transport - play
    if (buttonToggle[1]) rgbLedRE[1]->writeRGB(0, 255, 0); // green
    else rgbLedRE[1]->writeRGB(0, 0, 0); // off
    // Live transport - stop
    if (buttonToggle[2]) rgbLedRE[2]->writeRGB(255, 255, 255); // white
    else rgbLedRE[2]->writeRGB(0, 0, 0); // off
    // Live transport - record
    if (buttonToggle[3]) rgbLedRE[3]->writeRGB(255, 0, 0); // red
    else rgbLedRE[3]->writeRGB(0, 0, 0); //turnOff(); // off

  } else if (buttonMode == 1) { // track mode

    // the mode change button
    rgbLedRE[0]->writeRGB(255, 255, 0); // yellow
    // sets the colours of the other buttons when pressed - dependent on mode
    // Track parameters - solo
    if (buttonToggle[2]) rgbLedRE[2]->writeRGB(0, 200, 255); // light blue
    else rgbLedRE[2]->writeRGB(0, 0, 0); //turnOff(); // record - off
    // Track parameters - mute
    if (buttonToggle[3]) rgbLedRE[3]->writeRGB(0, 0, 0); // off - track is on i.e. mute is off by default in Live
    else rgbLedRE[3]->writeRGB(255, 120, 0); // orange

  } else if (buttonMode == 2) { // ? mode

    // the mode change button
    rgbLedRE[0]->writeRGB(255, 128, 0); // orange

  } else if (buttonMode == 3) { // ? mode

    // the mode change button
    rgbLedRE[0]->writeRGB(255, 0, 255); // magenta

  } else if (buttonMode == 4) { // ? mode

    // the mode change button
    rgbLedRE[0]->writeRGB(128, 0, 255); // purple

  } else if (buttonMode == 5) { // ? mode

    // the mode change button
    rgbLedRE[0]->writeRGB(0, 255, 255); // cyan

  }

  // set SoftPWM based on LED colours
  //// debugging
  //  Serial.print("rgbLedRE[1]->redValue: ");
  //  Serial.println(rgbLedRE[1]->redValue);
  //  Serial.print("rgbLedRE[1]->greenValue: ");
  //  Serial.println(rgbLedRE[1]->greenValue);
  //  Serial.print("rgbLedRE[2]->blueValue: ");
  //  Serial.println(rgbLedRE[2]->blueValue);
  //  Serial.print("rgbLedRE[3]->redValue: ");
  //  Serial.println(rgbLedRE[3]->redValue);
  //  Serial.print("rgbLedRE[3]->greenValue: ");
  //  Serial.println(rgbLedRE[3]->greenValue);
  //  Serial.print("rgbLedRE[3]->blueValue: ");
  //  Serial.println(rgbLedRE[3]->blueValue);

  SoftPWMSet(7, rgbLedRE[1]->redValue);
  SoftPWMSet(8, rgbLedRE[1]->greenValue);
  SoftPWMSet(12, rgbLedRE[2]->blueValue);
  SoftPWMSet(13, rgbLedRE[3]->redValue);
  SoftPWMSet(A0, rgbLedRE[3]->greenValue);
  SoftPWMSet(A1, rgbLedRE[3]->blueValue);

}


///////////////////////////////////////
/// buttons
///////////////////////////////////////

// The event handler for the buttons.
void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {

  switch (eventType) {
    case AceButton::kEventPressed:

      if (button->getId() == 1) {
        buttonToggle[1] = !buttonToggle[1];
      }
      if (button->getId() == 2) {
        buttonToggle[2] = !buttonToggle[2];
      }
      if (button->getId() == 3) {
        buttonToggle[3] = !buttonToggle[3];
      }

      //      // debugging
      //      Serial.print("kEventPressed:- eventType: ");
      //      Serial.print(eventType);
      //      Serial.print(", buttonState: ");
      //      Serial.print(buttonState);
      //      Serial.print(", button->getId(): ");
      //      Serial.println(button->getId());
      //      Serial.print("buttonToggle[1]: ");
      //      Serial.println(buttonToggle[1]);
      //      Serial.print("buttonToggle[2]: ");
      //      Serial.println(buttonToggle[2]);
      //      Serial.print("buttonToggle[3]: ");
      //      Serial.println(buttonToggle[3]);

      buttonAction = true;

      break;

    case AceButton::kEventReleased:
      // We trigger on the Released event not the Pressed event to distinguish
      // this event from the LongPressed event.

      if ((button->getId() == 2) && (buttonMode == 0)) { // for stop button in transport mode
        buttonToggle[2] = !buttonToggle[2];
      } else {
      }

      //      // debugging
      //      Serial.print("kEventReleased:- eventType: ");
      //      Serial.print(eventType);
      //      Serial.print(", buttonState: ");
      //      Serial.print(buttonState);
      //      Serial.print(", button->getId(): ");
      //      Serial.println(button->getId());

      buttonAction = true;

      break;

    case AceButton::kEventClicked:

      //      // debugging
      //      Serial.print("kEventClicked:- eventType: ");
      //      Serial.print(eventType);
      //      Serial.print(", buttonState: ");
      //      Serial.print(buttonState);
      //      Serial.print(", button->getId(): ");
      //      Serial.println(button->getId());

      break;

    case AceButton::kEventDoubleClicked:

      //      // debugging
      //      Serial.print("kEventDoubleClicked:- eventType: ");
      //      Serial.print(eventType);
      //      Serial.print(", buttonState: ");
      //      Serial.print(buttonState);
      //      Serial.print(", button->getId(): ");
      //      Serial.println(button->getId());

      break;

    case AceButton::kEventLongPressed:

      //      // debugging
      //      Serial.print("kEventLongPressed:- eventType: ");
      //      Serial.print(eventType);
      //      Serial.print(", buttonState: ");
      //      Serial.print(buttonState);
      //      Serial.print(", button->getId(): ");
      //      Serial.println(button->getId());

      break;

    case AceButton::kEventRepeatPressed:

      // step through the modes on button hold
      if (button->getId() == 0) { // mode button
        buttonMode += 1;
        if (buttonMode > noModes) {
          buttonMode = 0;
        }
      }

      //      // debugging
      //      Serial.print("kEventRepeatPressed:- eventType: ");
      //      Serial.print(eventType);
      //      Serial.print(", buttonState: ");
      //      Serial.print(buttonState);
      //      Serial.print(", button->getId(): ");
      //      Serial.println(button->getId());

      buttonAction = true;

      break;
  }

  // debugging
  //  if (currentTime - lastTime >= delayTime) {
  //    for (int i = 0; i < noButtonsAttached; i++) {
  //      // Print out a message for all events.
  //      Serial.print(F("handleEvent(): eventType: "));
  //      Serial.print(eventType);
  //      Serial.print(F("; button["));
  //      Serial.print(i);
  //      Serial.print(F("]: "));
  //      Serial.println(buttonState);
  //    }
  //    lastTime = currentTime;  // Updates lastTime
  //  }

}

// TO DO - this code just copied in from encoder module sketch - I2C comms yet to be implemented
//////////////////////////////////////
/// Nashesizer module functions
///////////////////////////////////////

// function that executes whenever data is requested by Teensy I2C master
// this function is registered as an event, see setup()
void requestEvent() {

  //  Wire.write(byte(lastEncUsed)); // respond with message of 1 byte as expected by master
  //  Wire.write(encB[lastEncUsed]); // respond with message of 1 byte as expected by master

}

// function that executes whenever data is received from Teensy I2C master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {

  //  while (Wire.available()) { // slave may send less than requested
  //    encNoInB = Wire.read(); // receive first byte as character
  //    encNoInI = int(encNoInB);
  //    encValueInB = Wire.read(); // receive second byte as character
  //    encValueInI = encValueInB;
  //    newFromTeensyMessage = true;
  //  }

}

void handleTeensyMessage() {

  //  // write the value to the encoder
  //  rotEnc[encNoInI]->write(encValueInI);
  //  // update it's RGB LED
  //  rgbLedRE[encNoInI]->writeRGB(0, encValueInI, 255 - encValueInI);

  newFromTeensyMessage = false; // ready for next messaage

}
