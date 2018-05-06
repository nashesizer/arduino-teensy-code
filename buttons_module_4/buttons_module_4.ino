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
  RGBLED Library by Bret Stateham, November 13, 2014 - latest version from http://github.com/BretStateham/RGBLED
*/
#include <RGBLED.h>

/// Wire configuration
int clockFrequency = 400000;

/// AceButton
// pointer to pointer - enables less repetion in the code via for loops using multiple pointers to
// button objects of the form button[i] - these declared in setup();
using namespace ace_button;
AceButton** button;
AdjustableButtonConfig adjustableButtonConfig;
int pinsB[4] = {0, 1, 2, 4};

/// RGB LEDs
RGBLED** rgbLedRE; // pointer to pointer - as per button
int pinsLED[12] = {3, 5, 6, 7, 8, 9, 10, 11, 12, 16, 14, 15};

// buttons
int noButtonsAttached = 4;
int buttonMode, buttonModeLast = 0;
int noModes = 5;
boolean buttonToggle[4] = {0, 0, 0, 0}; // button state - on or off
boolean buttonToggleReceived[4] = {0, 0, 0, 0}; // button state via Teensy - on or off
boolean physicalUpdate = false; // has a physical button press beenqwwqww detected
int lastButtonUsed = 0;
byte buttonToggleB = 0;
byte butEventTypeB = 0;
byte buttonNoB = 0;
int buttonNoI, buttonModeReceived = 0;

// for data sent from Teensy 3.6
boolean newFromTeensyMessage = false;

// simple timer for polling buttons and printing debugging info to Serial Monitor
unsigned long currentTime;
unsigned long lastTime;
unsigned long delayTime = 10; // recommended under 20ms


///////////////////////////////////////
/// SET UP
///////////////////////////////////////

void setup() {

  Serial.begin(250000);

  Wire.begin(3); // join i2c bus with address #3
  Wire.setClock(clockFrequency);
  Wire.onRequest(requestEvent); // register event - function below
  Wire.onReceive(receiveEvent); // register event - function below

  // buttons
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  pinMode(4, INPUT);

  /// AceButton
  // Configure the adjustableButtonConfig with the event handler and enable LongPress an RepeatPress.
  // SupressAfterLongPress is optional since we don't do anything if we get it.
  adjustableButtonConfig.setEventHandler(handleEvent);
  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
  adjustableButtonConfig.setLongPressDelay(1000);
  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterRepeatPress);
  adjustableButtonConfig.setRepeatPressInterval(1000);

  // just using the above for now - but could include the following features from demo sketch
  //  // Configure the adjustableButtonConfig with the event handler, and enable all higher level events.
  //  adjustableButtonConfig.setEventHandler(handleEvent);
  //  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureClick);
  //  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterClick);
  //  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
  //  adjustableButtonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterDoubleClick);

  button = new AceButton*[noButtonsAttached];

  for (int i = 0; i < noButtonsAttached; i++) {
    button[i] = new AceButton(pinsB[i], HIGH, i /* id */);
    button[i]->setButtonConfig(&adjustableButtonConfig);
  }

  for (int i = 0; i < 12; i++) {
    pinMode(pinsLED[i], OUTPUT);
  }

  // SoftPWM Library
  SoftPWMBegin(SOFTPWM_INVERTED); // this works empirically - suspect because RGB LEDs are common anode type

  // Declare an RGBLED instance
  rgbLedRE = new RGBLED*[noButtonsAttached];

  for (int i = 0; i < noButtonsAttached; i++) {
    rgbLedRE[i] = new RGBLED(pinsLED[3 * i], pinsLED[3 * i + 1], pinsLED[3 * i + 2], COMMON_ANODE);
  }

  // flash the button LEDs green at power up
  // some LEDs are use non-PWM pins so use SoftPWM library - though values of 0 and 255 via RGBLED library are intepreted as LOW and HIGH respectively
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

  // receive message from Teensy I2C master
  if (newFromTeensyMessage) handleTeensyMessage(); // do something with message from the Teensy
  newFromTeensyMessage = false;

  // TO DO - rotary encoder module now uses something like this - but buttons don't currently work in the same way - update?
  //  // sets the last button used - either physically or via OSC
  //    if (physicalUpdate == true) {
  //      lastButtonUsed = lastButtonUsed;
  //    } else if (physicalUpdate == false) {
  //      lastButtonUsed = buttonNoI;
  //    }

}


///////////////////////////////////////
/// button LEDs
///////////////////////////////////////

void setLEDs() {

  if (buttonMode == 0) { // default mode - transport

    // the mode change button
    rgbLedRE[0]->writeRGB(255, 255, 255); // white
    //delay(5);
    // sets the colours of the other buttons when pressed - dependent on mode
    // Live transport - play
    if (buttonToggle[1]) rgbLedRE[1]->writeRGB(0, 255, 0); // green
    else rgbLedRE[1]->writeRGB(0, 0, 0); // off
    //delay(5);
    // Live transport - stop
    if (buttonToggle[2]) rgbLedRE[2]->writeRGB(255, 255, 255); // white
    else rgbLedRE[2]->writeRGB(0, 0, 0); // off
    //delay(5);
    // Live transport - record
    if (buttonToggle[3]) rgbLedRE[3]->writeRGB(255, 0, 0); // red
    else rgbLedRE[3]->writeRGB(0, 0, 0); //turnOff(); // off

  } else if (buttonMode == 1) { // track mode

    // the mode change button
    rgbLedRE[0]->writeRGB(255, 255, 0); // yellow
    // sets the colours of the other buttons when pressed - dependent on mode
    // Track parameters - solo
    if (buttonToggle[2]) rgbLedRE[2]->writeRGB(0, 200, 255); // light blue
    else rgbLedRE[2]->writeRGB(0, 0, 0); // off
    // Track parameters - mute
    if (buttonToggle[3]) rgbLedRE[3]->writeRGB(255, 120, 0); // orange - track is on i.e. mute is off by default in Live
    else rgbLedRE[3]->writeRGB(0, 0, 0); // off

  } else if (buttonMode == 2) { // ? mode

    // the mode change button
    rgbLedRE[0]->writeRGB(255, 128, 0); // orange

  } else if (buttonMode == 3) { // ? mode

    // the mode change button
    rgbLedRE[0]->writeRGB(255, 0, 255); // magenta

  } else if (buttonMode == 4) { // ? mode

    // the mode change button
    rgbLedRE[0]->writeRGB(0, 0, 255); // blue

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
  SoftPWMSet(A2, rgbLedRE[3]->redValue);
  SoftPWMSet(A0, rgbLedRE[3]->greenValue);
  SoftPWMSet(A1, rgbLedRE[3]->blueValue);

}


///////////////////////////////////////
/// buttons
///////////////////////////////////////

// The event handler for the buttons.
void handleEvent(AceButton * button, uint8_t eventType, uint8_t buttonState) {

  switch (eventType) {
    case AceButton::kEventPressed: // eventType = 0;

      butEventTypeB = byte(0);

      if (button->getId() == 1) {
        buttonToggle[1] = !buttonToggle[1];
        lastButtonUsed = 1;
      }
      if (button->getId() == 2) {
        buttonToggle[2] = !buttonToggle[2];
        lastButtonUsed = 2;
      }
      if (button->getId() == 3) {
        buttonToggle[3] = !buttonToggle[3];
        lastButtonUsed = 3;
      }

      //      // debugging
      //      Serial.print("kEventPressed:- eventType: ");
      //      Serial.print(eventType);
      //      Serial.print(", buttonState: ");
      //      Serial.print(buttonState);
      //      Serial.print(", button->getId(): ");
      //      Serial.println(button->getId());
      //      Serial.print("buttonToggle[0]: ");
      //      Serial.println(buttonToggle[0]);
      //      Serial.print("buttonToggle[1]: ");
      //      Serial.println(buttonToggle[1]);
      //      Serial.print("buttonToggle[2]: ");
      //      Serial.println(buttonToggle[2]);
      //      Serial.print("buttonToggle[3]: ");
      //      Serial.println(buttonToggle[3]);

      physicalUpdate = true;

      break;

    case AceButton::kEventReleased: // eventType = 1;

      butEventTypeB = byte(1);

      // trigger on the Released event not the Pressed event to distinguish this event from the LongPressed event.
      if ((button->getId() == 2) && (buttonMode == 0)) { // for stop button in transport mode
        buttonToggle[2] = !buttonToggle[2];
        lastButtonUsed = 2;
      }

      //      // debugging
      //      Serial.print("kEventReleased:- eventType: ");
      //      Serial.print(eventType);
      //      Serial.print(", buttonState: ");
      //      Serial.print(buttonState);
      //      Serial.print(", button->getId(): ");
      //      Serial.println(button->getId());

      physicalUpdate = true;

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

    case AceButton::kEventLongPressed: // eventType = 4;

      butEventTypeB = byte(4);

      //      // debugging
      //      Serial.print("kEventLongPressed:- eventType: ");
      //      Serial.print(eventType);
      //      Serial.print(", buttonState: ");
      //      Serial.print(buttonState);
      //      Serial.print(", button->getId(): ");
      //      Serial.println(button->getId());

      break;

    case AceButton::kEventRepeatPressed: // eventType = 4;

      butEventTypeB = byte(5);

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

      lastButtonUsed = 0;
      physicalUpdate = true;

      break;
  }

  //    // debugging
  //    if (currentTime - lastTime >= delayTime) {
  //      for (int i = 0; i < noButtonsAttached; i++) {
  //        // Print out a message for all events.
  //        Serial.print(F("handleEvent(): eventType: "));
  //        Serial.print(eventType);
  //        Serial.print(F("; button["));
  //        Serial.print(i);
  //        Serial.print(F("]: "));
  //        Serial.println(buttonState);
  //      }
  //      lastTime = currentTime;  // Updates lastTime
  //    }

}


//////////////////////////////////////
/// I2C Comms functions
///////////////////////////////////////

// function that executes whenever data is requested by Teensy I2C master
// this function is registered as an event, see setup()
void requestEvent() {

  if (lastButtonUsed != 0) { // other three buttons
    Wire.write(byte(lastButtonUsed)); // respond with message of 1 byte as expected by master
    Wire.write(byte(buttonToggle[lastButtonUsed]));
    Wire.write(butEventTypeB);
  } else if (lastButtonUsed == 0) { // mode button
    Wire.write(byte(lastButtonUsed));
    Wire.write(byte(buttonMode));
    Wire.write(butEventTypeB);
  }

}

//-------------------------------------------------------
// TO DO - change this to receive button # + toggle state
// function that executes whenever data is received from Teensy I2C master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {

  while (Wire.available()) { // slave may send less than requested

    // old code - in for reference but to delete
    //    buttonToggleB = Wire.read(); // receive first byte as character
    //    buttonToggleReceived[1] = boolean(buttonToggleB);
    //    buttonToggleB = Wire.read(); // receive first byte as character
    //    buttonToggleReceived[2] = boolean(buttonToggleB);qwewwqweww
    //    buttonToggleB = Wire.read(); // receive first byte as character
    //    buttonToggleReceived[3] = boolean(buttonToggleB);

    // new code - doesn't work
    buttonNoB = Wire.read();
    buttonNoI = int(buttonNoB);
    buttonToggleB = Wire.read();
    if (buttonNoI == 0) { // mode button
      //buttonModeReceived = int(buttonToggleB);
      buttonMode = int(buttonToggleB);
    } else {
      //buttonToggleReceived[buttonNoI] = boolean(buttonToggleB);
      buttonToggle[buttonNoI] = boolean(buttonToggleB);
    }

    //        // debugging
    if (buttonNoI == 0) {
      Serial.print("buttonMode: ");
      Serial.println(buttonMode);
    } else {
      Serial.print("buttonToggle[");
      Serial.print(buttonNoI);
      Serial.print("]: ");
      Serial.println(buttonToggle[buttonNoI]);
    }

    //    if (buttonNoI == 0) { // mode button
    //      buttonMode = buttonModeReceived;
    //    } else {
    //      buttonToggle[buttonNoI] = buttonToggleReceived[buttonNoI];
    //    }

    physicalUpdate = false;
    newFromTeensyMessage = true;
  }

  //        // debugging
  //        for (int i = 0; i < noButtonsAttached; i++) {
  //          Serial.print("buttonToggleReceived[");
  //          Serial.print(i);
  //          Serial.print("]: ");
  //          Serial.println(buttonToggleReceived[i]);
  //        }

}

//-------------------------------------------------------
void handleTeensyMessage() {

  // old code - in for reference but to delete
  //  for (int i = 1; i < noButtonsAttached; i++) {qwqwewwqwewwqwewwqwewwqew
  //    buttonToggle[i] = buttonToggleReceived[i];
  //  }

  // new code - doesn't work
  // moved to receiveEvent()
  //  if (buttonNoI == 0) { // mode button
  //    buttonMode = buttonModeReceived;
  //  } else {
  //    buttonToggle[buttonNoI] = buttonToggleReceived[buttonNoI];
  //  }
  delay(5);
  //  Serial.print("buttonNoI: ");
  //  Serial.print(buttonNoI);
  //  Serial.print(", buttonToggle: ");
  //  Serial.println(buttonToggle[buttonNoI]);

  setLEDs();
  newFromTeensyMessage = false; // ready for next messaage

}
