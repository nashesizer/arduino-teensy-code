///////////////////////////////////////
/// Buttons module functions
///////////////////////////////////////

void readButtons() {

  Wire.requestFrom(3, 3); // slave device #3 request 3 bytes
  while (Wire.available()) { // slave may send less than requested
    butNoB = Wire.read(); // receive first byte as character
    butValueB = Wire.read(); // receive second byte as character
    butEventTypeB = Wire.read(); // receive third byte as character
  }

  // convert message bytes to ints
  butNoI = int(butNoB);
  butValueI = int(butValueB);
  butEventTypeI = int(butEventTypeB);

  if (butNoI == 0) { // mode button
    buttonMode = butValueI;
    //    Serial2.print("buttonMode: ");
    //    Serial2.println(buttonMode);
  } else { // all other buttons
    buttonToggle[butNoI] = butValueI;
  }

  // only trigger a new button message or update and print to CoolTerm Serial Monitor if values change
  if ((butNoI != butNoILast) || (butValueILast != butValueI) || (butEventTypeI != butEventTypeILast)) {

    // debugging
    //    Serial2.print("butNoI: ");
    //    Serial2.print(butNoI);
    //    Serial2.print(", butValueI: ");
    //    Serial2.print(butValueI);
    //    Serial2.print(", butEventTypeI: ");
    //    Serial2.println(butEventTypeI);
    //    if ((butNoI != butNoILast) || (butValueILast != butValueI) || (butEventTypeILast != butEventTypeI)) {
    //      Serial2.print("buttonMode: ");
    //      Serial2.println(buttonMode);
    //      for (int i = 1; i < noButtonsAttached; i++) {
    //        Serial2.print("buttonToggle[");
    //        Serial2.print(i);
    //        Serial2.print("]: ");
    //        Serial2.println(buttonToggle[i]);
    //      }
    //    }

    //  } else {
    //  }

    newBMessage = true;

    butNoILast = butNoI;
    butValueILast = butValueI;
    butEventTypeILast = butEventTypeI;

  }

  goTime3 = millis() + nextTime3;

}


//-------------------------------------------------------
void handleButtonsMessage() {

  // much of this code was orinally in the buttons module - but let the Teensy do all the logic and keep the button module dumb - unecessarily complicates things otherwise

  if (buttonMode == 0) { // default - Live transport

    // switch buttons and LEDs to default state to start
    if (buttonMode != buttonModeLast) {
      buttonToggle[1] = false;
      buttonToggle[2] = false;
      buttonToggle[3] = false;
    }

    if ((butNoI == 1) && (butEventTypeI == 0)) { // play - button press
      if (buttonToggle[1] == true) {
        // send a key press
        Keyboard.press('q');
        // release key press
        Keyboard.release('q');
      } else if (buttonToggle[1] == false) {
        Keyboard.press(' ');
        Keyboard.release(' ');
      }
    }

    if ((butNoI == 2) && (butEventTypeI == 1)) { // stop - button release
      // toggle button state
      //      buttonToggle[2] = false;
      //updateSingleBM(2, buttonToggle[2]);
      //      Serial2.print("buttonToggle[2]: ");
      //      Serial2.println(buttonToggle[2]);
      // if play and/or record buttons are toggled on then toggle off
      if (buttonToggle[1]) buttonToggle[1] = false; // stop play
      //updateSingleBM(1, buttonToggle[1]);
      if (buttonToggle[3]) buttonToggle[3] = false; // stop record
      //updateSingleBM(3, buttonToggle[3]);
      Keyboard.press('w');
      Keyboard.release('w');
    }

    if ((butNoI == 3) && (butEventTypeI == 0)) { // record - button press
      if (buttonToggle[3] == true) {
        // play button always on when record on
        buttonToggle[1] = true;
        Keyboard.press('e'); // send a key press
        Keyboard.release('e');

      } else if (buttonToggle[3] == false) {
        Keyboard.press('e'); // send a key press
        Keyboard.release('e');
      }
    }

  } else if (buttonMode == 1) { // track mode

    // switch buttons and LEDs to default state
    if (buttonMode != buttonModeLast) {
      //      Serial2.println("new Track Mode");
      newTrkMode = true;
      buttonToggle[1] = false; // not used - off
      // set state from trkSettings array
      buttonToggle[2] = readTrkSettings(currentTrkNo - 1, 8);
      buttonToggle[3] = readTrkSettings(currentTrkNo - 1, 9);
      //      Serial2.print("buttonToggle[2]: ");
      //      Serial2.println(buttonToggle[2]);
      Serial2.print("buttonToggle[3]: ");
      Serial2.println(buttonToggle[3]);
      updateBM();
    }

    // solo - button 2
    if ((butNoI == 2) && (butEventTypeI == 0)) { // solo - button press
      // write to the trkSettings array
      if (buttonToggle[2] == true) {
        writeTrkSettings(currentTrkNo - 1, 8, 1);
      } else if (buttonToggle[2] == false) {
        writeTrkSettings(currentTrkNo - 1, 8, 0);
      }
      // compile the message
      toHubBMessage = "/T";
      toHubBMessage += currentTrkNo;
      toHubBMessage += "/Solo,f,";
      toHubBMessage += float(readTrkSettings(currentTrkNo - 1, 8)); // LiveGrabber seems to only like float values in
      newBMode1Message = true;
      sendHubOutgoing();

      //      Serial2.print("buttonToggle[2]: ");
      //      Serial2.println(buttonToggle[2]);


    }

    // mute - button 3
    if ((butNoI == 3) && (butEventTypeI == 0)) { // mute - button press
      // write to the trkSettings array
      if (buttonToggle[3] == true) {
        writeTrkSettings(currentTrkNo - 1, 9, 0);
      } else if (buttonToggle[3] == false) {
        writeTrkSettings(currentTrkNo - 1, 9, 1);
      }
      // compile the message
      toHubBMessage = "/T";
      toHubBMessage += currentTrkNo;
      toHubBMessage += "/Mute,f,";
      //toHubBMessage += float(trkSettings[currentTrkNo - 1][9]); // LiveGrabber only seems to like float values
      toHubBMessage += float(readTrkSettings(currentTrkNo - 1, 9));
      newBMode1Message = true;
      sendHubOutgoing();

      //      Serial2.print("buttonToggle[3]: ");
      //      Serial2.println(buttonToggle[3]);

    }

  } else if (buttonMode == 2) { // ? - mode yet to be decided

    // switch buttons and LEDs to default state
    if (buttonMode != buttonModeLast) {
      buttonToggle[1] = false;
      buttonToggle[2] = false;
      buttonToggle[3] = false;
    }

  } else if (buttonMode == 3) { // ? - mode yet to be decided

    // switch buttons and LEDs to default state
    if (buttonMode != buttonModeLast) {
      buttonToggle[1] = false;
      buttonToggle[2] = false;
      buttonToggle[3] = false;
    }

  } else if (buttonMode == 4) { // ? - mode yet to be decided

    // switch buttons and LEDs to default state
    if (buttonMode != buttonModeLast) {
      buttonToggle[1] = false;
      buttonToggle[2] = false;
      buttonToggle[3] = false;
    }

  }

  //  Serial2.print("butNoI: ");
  //  Serial2.print(butNoI);
  //  Serial2.print(", butValueI: ");
  //  Serial2.print(butValueI);
  //  Serial2.print(", butEventTypeI: ");
  //  Serial2.println(butEventTypeI);


  //  // TO DO - rework I2C comms with buttons module - to behave more like the rotary encoders module
  //  // send the Teensy button states to the buttons module
  //  // old code - in for reference but to delete
  //  Wire.beginTransmission(3);  // transmit to device #3
  //  for (int i = 1; i < noButtonsAttached; i++) {
  //    Wire.write(byte(buttonToggle[i]));
  //  }
  //  Wire.endTransmission();     // stop transmitting

  // new code - not working
  //if (buttonMode == 0) { // default - transport
  for (int i = 1; i < noButtonsAttached; i++) {
    updateSingleBM(i, buttonToggle[i]);
  }
  //} else if (buttonMode == 1) { // track
  //  updateSingleBM(2, buttonToggle[2]);
  //  updateSingleBM(3, buttonToggle[3]);
  //}

  //  updateSingleBM(1, buttonToggle[1]);
  //  updateSingleBM(2, buttonToggle[2]);
  //  updateSingleBM(3, buttonToggle[3]);

  //updateBM();

  buttonModeLast = buttonMode;

  newBMessage = false;

}

//-------------------------------------------------------
void updateSingleBM(int butNo, int buttonValue) {

  Wire.beginTransmission(3);  // transmit to device #3
  Wire.write(byte(butNo));
  Wire.write(byte(buttonValue));
  Wire.endTransmission();     // stop transmitting
  delayMicroseconds(200);
}


//-------------------------------------------------------
void updateBM() {

  if (buttonMode == 0) {

    //    // new code - not working
    //    Wire.beginTransmission(3);  // transmit to device #3
    //    Wire.write(byte(0));
    //    Wire.write(byte(buttonMode));
    //    Wire.endTransmission();     // stop transmitting
    //    delayMicroseconds(100);
    //    Wire.beginTransmission(3);  // transmit to device #3
    //    Wire.write(byte(1));
    //    Wire.write(byte(buttonToggle[1])); // for unused button 1
    //    Wire.endTransmission();     // stop transmitting
    //    delayMicroseconds(100);
    //    Wire.beginTransmission(3);  // transmit to device #3
    //    Wire.write(byte(2));
    //    Wire.write(byte(buttonToggle[2])); // for solo - button 2
    //    Wire.endTransmission();     // stop transmitting
    //    delayMicroseconds(100);
    //    Wire.beginTransmission(3);  // transmit to device #3
    //    Wire.write(byte(3));
    //    Wire.write(byte(buttonToggle[3])); // for solo - button 2
    //    Wire.endTransmission();     // stop transmitting

  } else if (buttonMode == 1) { // track mode - toggle mute and solo

    //    // old code - in for reference but to delete
    //    Wire.beginTransmission(3);  // transmit to device #3
    //    Wire.write(byte(0)); // for unused button 1
    //    Wire.write(byte(readTrkSettings(currentTrkNo - 1, 8))); // for solo - button 2
    //    // for mute - button 3
    //    // there's an inversion in Live where if the mute toggle state = 0 i.e. mute is off the button is on
    //    if (trkSettings[currentTrkNo - 1][9] == 0) {
    //      Wire.write(byte(1));
    //    } else if (trkSettings[currentTrkNo - 1][9] == 1) {
    //      Wire.write(byte(0));
    //    }
    //    Wire.endTransmission();     // stop transmitting

    //new code - not working
    Wire.beginTransmission(3);  // transmit to device #3
    Wire.write(byte(0));
    Wire.write(byte(buttonMode));
    Wire.endTransmission();     // stop transmitting
    delayMicroseconds(200);
    Wire.beginTransmission(3);  // transmit to device #3
    Wire.write(byte(1));
    Wire.write(byte(0)); // for unused button 1
    Wire.endTransmission();     // stop transmitting
    delayMicroseconds(200);
    Wire.beginTransmission(3);  // transmit to device #3
    Wire.write(byte(2));
    Wire.write(byte(readTrkSettings(currentTrkNo - 1, 8))); // for solo - button 2
    Wire.endTransmission();     // stop transmitting
    delayMicroseconds(200);
    Wire.beginTransmission(3);  // transmit to device #3
    Wire.write(byte(3));
    //Wire.write(byte(readTrkSettings(currentTrkNo - 1, 9))); // for mute - button 3
    // for mute - button 3
    if (newTrkMode == true) {
        Wire.write(byte(trkSettings[currentTrkNo - 1][9]));
        newTrkMode = false;
      } else {
        // there's an inversion in Live where if the mute toggle state = 0 i.e. mute is off the button is on
        if (trkSettings[currentTrkNo - 1][9] == 0) {
          Wire.write(byte(1));
        } else if (trkSettings[currentTrkNo - 1][9] == 1) {
          Wire.write(byte(0));
        }
      }
    Wire.endTransmission();     // stop transmitting

  } else {
    // do nothing - for the time being
  }

}
