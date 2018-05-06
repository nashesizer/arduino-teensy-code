///////////////////////////////////////
/// Rotary encoders module functions
///////////////////////////////////////

void readEncoders() {

  Wire.requestFrom(2, 2); // slave device #2 request 2 bytes
  while (Wire.available()) { // slave may send less than requested
    encNoB = Wire.read(); // receive first byte as character
    encValueB = Wire.read(); // receive second byte as character
  }
  int encValueI = int(encValueB);
  float encValueF = mapif(encValueI, 0, 255, 0.0, 1.0);
  //  //debugging
  //  Serial2.print(encNoB, DEC); // print the byte
  //  Serial2.print(":");
  //  Serial2.print(bEnc, DEC); // print the byte
  //  Serial2.println();
  //  Serial2.print("encValueI: ");
  //  Serial2.println(encValueI);
  //  Serial2.print("encValueF: ");
  //  Serial2.println(encValueF);

  if (encNoB == 0) { // right most encoder

    paramNo = 7; // SendF
    // writes the value into the trkSettings array
    writeTrkSettings(currentTrkNo - 1, paramNo, encValueI);
    // compiles the message
    toHubREMessage = "/T";
    toHubREMessage += currentTrkNo;
    toHubREMessage += "/SendF,f,";
    toHubREMessage += encValueF;

  } else if (encNoB == 1) { // 2nd right most encoder

    paramNo = 6; // SendE
    // writes the value into the trkSettings array
    writeTrkSettings(currentTrkNo - 1, paramNo, encValueI);
    // compiles the message
    toHubREMessage = "/T";
    toHubREMessage += currentTrkNo;
    toHubREMessage += "/SendE,f,";
    toHubREMessage += encValueF;

  } else if (encNoB == 2) { // 2nd left most encoder

    paramNo = 5; // SendD
    // writes the value into the trkSettings array
    writeTrkSettings(currentTrkNo - 1, paramNo, encValueI);
    // compiles the message
    toHubREMessage = "/T";
    toHubREMessage += currentTrkNo;
    toHubREMessage += "/SendD,f,";
    toHubREMessage += encValueF;

  } else if (encNoB == 3) {  // left most encoder

    paramNo = 1; // Panning
    // writes the value into the trkSettings array
    writeTrkSettings(currentTrkNo - 1, paramNo, encValueI);
    // compiles the message
    toHubREMessage = "/T";
    toHubREMessage += currentTrkNo;
    toHubREMessage += "/Panning,f,";
    toHubREMessage += encValueF;

  }

  // send new message only if it's different to the last message
  if (toHubREMessage != toHubREMessageLast) {
    // set flag to true
    newREMessage = true;
    // send the message
    sendHubOutgoing();
  }
  toHubREMessageLast = toHubREMessage;

  goTime2 = millis() + nextTime2;

}


//-------------------------------------------------------
void updateSingleREM(int paramNo, int paramValue) {

  Wire.beginTransmission(2);  // transmit to device #2

  // this worked - but switching the simpler sending two bytes
  //    char buf1[2];
  //    for (int i = 0; i < 2; i++) {
  //      dtostrf(paramNo, 1, 0, buf1);
  //    }
  //    Wire.write(buf1);
  //    Wire.write(',');
  //    char buf2[6];
  //    for (int i = 0; i < 3; i++) {
  //      dtostrf(paramValueI, 1, 3, buf2);  //4 is mininum width, 6 is precision
  //    }
  //    Wire.write(buf2);
  //    Wire.write(':');

  Wire.write(byte(paramNo));
  Wire.write(byte(paramValue));
  //  // debugging
  //  Serial2.print("updateSingleREM(): ");
  //  Serial2.print(paramNo);
  //  Serial2.print(", ");
  //  Serial2.println(paramValue);
  //Serial2.println(trkSettings[currentTrkNo - 1][paramNo]);

  Wire.endTransmission();     // stop transmitting

}


//-------------------------------------------------------
void updateREM() {

  Wire.beginTransmission(2);  // transmit to device #2
  Wire.write(byte(1));
  Wire.write(byte(trkSettings[currentTrkNo - 1][1]));
  Wire.endTransmission();     // stop transmitting

  Wire.beginTransmission(2);  // transmit to device #2
  Wire.write(byte(5));
  Wire.write(byte(trkSettings[currentTrkNo - 1][5]));
  Wire.endTransmission();     // stop transmitting

  Wire.beginTransmission(2);  // transmit to device #2
  Wire.write(byte(6));
  Wire.write(byte(trkSettings[currentTrkNo - 1][6]));
  Wire.endTransmission();     // stop transmitting

  Wire.beginTransmission(2);  // transmit to device #2
  Wire.write(byte(7));
  Wire.write(byte(trkSettings[currentTrkNo - 1][7]));
  Wire.endTransmission();     // stop transmitting

}
