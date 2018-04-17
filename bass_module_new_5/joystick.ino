///////////////////////////////////////
/// Joystick module functions
///////////////////////////////////////

void readJoystick() {

  Wire.requestFrom(1, 1); // slave device #1 request 1 byte
  while (Wire.available()) { // slave may send less than requested
    jsB = Wire.read(); // receive a byte as character
  }
  ////debugging
  //Serial2.print(jsB, BIN); // print the byte
  //Serial2.print(",");

  if ((jsB == jsUp) && (jsBLast != jsB)) { // if a new joystick UP byte is received and the previous byte received was not UP
    Keyboard.press(KEY_UP_ARROW); // send a key press
    Keyboard.releaseAll();
    // debugging
    //Serial2.print("jsB: ");
    //Serial2.print(jsB);
    //Serial2.print(", jsBLast: ");
    //Serial2.println(jsBLast);
  } else if ((jsB == jsDown) && (jsBLast != jsB)) { // else if a new joystick DOWN byte is received and the previous byte received was not DOWN
    Keyboard.press(KEY_DOWN_ARROW);
    Keyboard.releaseAll();
    // debugging
    //Serial2.print("jsB: ");
    //Serial2.print(jsB);
    //Serial2.print(", jsBLast: ");
    //Serial2.println(jsBLast);
  } else if ((jsB == jsRight) && (jsBLast != jsB)) { // else if a new joystick RIGHT byte is received and the previous byte received was not RIGHT
    currentTrkNo += 1;
    jsTrkChange = true;
    Keyboard.press(KEY_RIGHT_ARROW);
    Keyboard.releaseAll();
    // debugging
    //Serial2.print("jsB: ");
    //Serial2.print(jsB);
    //Serial2.print(", jsBLast: ");
    //Serial2.println(jsBLast);
  } else if ((jsB == jsLeft) && (jsBLast != jsB)) { // else if a new joystick LEFT byte is received and the previous byte received was not LEFT
    currentTrkNo -= 1;
    jsTrkChange = true;
    Keyboard.press(KEY_LEFT_ARROW);
    Keyboard.releaseAll();
    // debugging
    //Serial2.print("jsB: ");
    //Serial2.print(jsB);
    //Serial2.print(", jsBLast: ");
    //Serial2.println(jsBLast);
  } else {
    // debugging
    //Serial2.print("jsB: ");
    //Serial2.print(jsB);
    //Serial2.print(", jsBLast: ");
    //Serial2.println(jsBLast);
  }

  // limit the joystick defined current track # to between 1-24
  if (currentTrkNo < 1) {
    currentTrkNo = 1;
  }
  if (currentTrkNo > 24) {
    currentTrkNo = 24;
  }
  //  // debugging
  //  Serial2.print("currentTrkNo: ");
  //  Serial2.println(currentTrkNo);

  if (jsBLast == jsB) { // if the new byte is the same as the last byte - joystick is being held - start the stopwatch
    jsHoldSW.start();
  } else { // otherwise stop and reset the stopwatch
    jsHoldSW.stop();
    jsHoldSW.reset();
  }
  // debugging
  //    Serial2.print("jsHoldSW.elapsed(): ");
  //    Serial2.println(jsHoldSW.elapsed());

  // use the stopwatch to reset the last byte to CENTRE after a set time so that the code above retriggers creating an auto repeat key press
  if ((jsHoldSW.elapsed() > 250) && ((jsB == jsUp) || (jsB == jsDown) || (jsB == jsRight) || (jsB == jsLeft))) {
    jsBLast = B00000000;
  } else {
    jsBLast = jsB;
  }

  goTime1 = millis() + nextTime1;

}

