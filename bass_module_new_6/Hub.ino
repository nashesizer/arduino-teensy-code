///////////////////////////////////////
/// Hub Serial functions
///////////////////////////////////////

void readHubIncoming() {
  for ( int i = 0; i < sizeof(buf);  ++i ) buf[i] = (char)0; // clear the buffer
  fromHubMessage = ""; // reset the message
  static int i = 0;
  while (Serial.available() > 0 && newHubIncoming == false) {
    buf[i] = Serial.read();
    //Serial2.print(buf[i]);
    if (buf[i] == ':') { // added ':' delimiter to end of message in Nashesizer Hub Processing sketch
      newHubIncoming = true;
      i = 0;
    } else {
      // compile the message from the buffer
      fromHubMessage += buf[i];
      //Serial2.println(fromHubMessage);
      i += 1;
    }
  }
}


//-------------------------------------------------------
void handleHubIncoming() {

  //// debugging
  //Serial2.println(fromHubMessage);

  // extracts values from fromHubMessage and updates the trkSettings array
  // messages from Live parameters Volume, Panning, SendA-SendF are of the form /T2/Panning,f,0.2777778
  // messages from Live parameters Mute + Solo are of the form /T2/Mute,i,1
  // TO DO - clips names etc. won't use this format - need to check for type
  // indexes for the various delimiters
  ind1 = fromHubMessage.indexOf('T');  // finds location of the T
  ind2 = fromHubMessage.indexOf('/', ind1 + 1 ); // finds location of second /
  ind3 = fromHubMessage.indexOf(',', ind2 + 1 ); // finds location of first ,
  ind4 = fromHubMessage.indexOf(',', ind3 + 1 ); // finds location of second ,
  // extract the the track # - used later to set the column position in the trkSettings array
  trkNo = fromHubMessage.substring(ind1 + 1, ind2).toInt();
  //// debugging
  //    Serial2.print("trkNo: ");
  //    Serial2.println(trkNo);
  // TO DO - should an update to a parameter via Live in any track other than the currently selected one reset the current track #? - to test.
  // Testing to date shows that its relatively easy to loose track focus in Live resulting in the joystick controlled trackNo falling out of sync -
  // this might help sort that out
  //  // updates the current track # defined by the joystick
  //  currentTrkNo = trkNo;
  // extract the parameter type
  paramType = fromHubMessage.substring(ind2 + 1, ind3);
  //// debugging
  //  Serial2.print("paramType: ");
  //  Serial2.println(paramType);
  // assigns a parameter type to a row position in the array
  if (paramType == "Volume") {
    paramNo = 0;
  } else if (paramType == "Panning") {
    paramNo = 1;
  } else if (paramType == "SendA") {
    paramNo = 2;
  } else if (paramType == "SendB") {
    paramNo = 3;
  } else if (paramType == "SendC") {
    paramNo = 4;
  } else if (paramType == "SendD") {
    paramNo = 5;
  } else if (paramType == "SendE") {
    paramNo = 6;
  } else if (paramType == "SendF") {
    paramNo = 7;
  } else if (paramType == "Solo") {
    paramNo = 8;
  } else if (paramType == "Mute") {
    paramNo = 9;
  }

  if ((paramNo == 0) || (paramNo == 2) || (paramNo == 3) || (paramNo == 4)) { // motorised slider module
    // extracts the parameter value as float between 0.0-1.0
    paramValueF = fromHubMessage.substring(ind4 + 1, fromHubMessage.length()).toFloat();
    // converts it to an integer between 0-255
    paramValueI = mapfi(paramValueF, 0.0, 1.0, 0, 255);
    // writes the value into the array
    writeTrkSettings(trkNo - 1, paramNo, paramValueI);
  } else if ((paramNo == 1) || (paramNo == 5) || (paramNo == 6) || (paramNo == 7)) { // rotary encoders
    // extracts the parameter value as float between 0.0-1.0
    paramValueF = fromHubMessage.substring(ind4 + 1, fromHubMessage.length()).toFloat();
    // converts it to an integer between 0-255
    paramValueI = mapfi(paramValueF, 0.0, 1.0, 0, 255);
    // writes the value into the array
    writeTrkSettings(trkNo - 1, paramNo, paramValueI);
  } else if ((paramNo == 8) || (paramNo == 9)) { // buttons
    // extracts the parameter value as an int
    paramValueI = fromHubMessage.substring(ind4 + 1, fromHubMessage.length()).toInt();
    // writes the value into the array
    writeTrkSettings(trkNo - 1, paramNo, paramValueI);
  }

  //// debugging
  //  Serial2.print("paramNo: ");
  //  Serial2.println(paramNo);
  //  Serial2.print("paramValueF: ");
  //  Serial2.println(paramValueF);
  //  // debugging
  //  Serial2.print("from Hub message: trkSettings[");
  //  Serial2.print(trkNo - 1);
  //  Serial2.print("][");
  //  Serial2.print(paramNo);
  //  Serial2.print("]: ");
  //  Serial2.println(trkSettings[trkNo - 1][paramNo]);

  // if the track # of a changed track parameter via Live's interface is the same as the currently active track # update the motorised sliders, rotary encoders and buttons module
  if (trkNo == currentTrkNo) {

    if ((paramNo == 0) || (paramNo == 2) || (paramNo == 3) || (paramNo == 4)) { // motorised slider module
      updateMSM();
      //delay(400);
    } else if ((paramNo == 1) || (paramNo == 5) || (paramNo == 6) || (paramNo == 7)) { // rotary encoders module
      updateSingleREM(paramNo, readTrkSettings(currentTrkNo - 1, paramNo));
    } else if ((paramNo == 8) || (paramNo == 9)) { // buttons module
      // TO DO - rework I2C comms with buttons module - to behave more like the rotary encoders module
      updateBM();
    }

  }

  newHubIncoming = false; // ready for next messaage

}


//-------------------------------------------------------
void sendHubOutgoing() {

  if (newBMode1Message == true) {
    Serial.println(toHubBMessage);
    newBMode1Message = false;
  } else if (newREMessage == true) {
    Serial.println(toHubREMessage);
    newREMessage = false;
  } else if (newMSMessage == true) {
    Serial.println(toHubMSMessage);
    newMSMessage = false;
  }

}

