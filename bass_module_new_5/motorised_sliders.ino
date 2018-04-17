///////////////////////////////////////
/// Sliders module functions
///////////////////////////////////////

void readSliders() {

  int data1, data2;
  //int data[8];
  Wire.requestFrom(0x1C, 8); // get 8 bytes from sliders
  for (int i = 0; i < 4; i++) {
    data1 = Wire.read();
    data2 = Wire.read();
    //    // debugging
    //    Serial2.print("data1: ");
    //    Serial2.print(data1);
    //    Serial2.print(", data2: ");
    //    Serial2.println(data2);
    if ((data1 == '0xff') && (data2 == '0xff')) {
      // do nothing - slider is moving
      msMoving = true;
    } else {
      sliderReadings[i] = (data1 << 8) | data2; // combine the two bytes
      msMoving = false;
    }
  }

//      // debugging
//      if (msMoving == true) {
//        Serial2.println("sliders moving");
//      }
  
//      // debugging
//      for (int i = 0; i < 4; i++) {
//        Serial2.print(sliderReadings[i]);
//        Serial2.print(" ");
//      }
//      Serial2.println("");

  // there's a bit of 'jitter' in values from the motorised sliders - this conditional only reads changes to slider values that are greater than a preset jitter margin
  if ((sliderReadingsLast[3] <= sliderReadings[3] - jitter) || (sliderReadingsLast[3] >= sliderReadings[3] + jitter)) { // left most slider - Volume

    float sldValueF = mapif(sliderReadings[3], 0, 1023, 0.0, 1.0);
    int sldValueI = map(sliderReadings[3], 0, 1023, 0, 255);

    paramNo = 0; // Volume
    // writes the value into the trkSettings array
    writeTrkSettings(currentTrkNo - 1, paramNo, sldValueI);
    // compiles the message
    toHubMSMessage = "/T";
    toHubMSMessage += currentTrkNo;
    toHubMSMessage += "/Volume,f,";
    toHubMSMessage += sldValueF;

    //    // debugging
    //    Serial2.print("trkSettings[");
    //    Serial2.print(currentTrkNo - 1);
    //    Serial2.print("][0]: ");
    //    Serial2.println(readTrkSettings(currentTrkNo - 1, 0));

  } else if ((sliderReadingsLast[2] <= sliderReadings[2] - jitter) || (sliderReadingsLast[2] >= sliderReadings[2] + jitter)) { // second left most slider - Send A

    float sldValueF = mapif(sliderReadings[2], 0, 1023, 0.0, 1.0);
    int sldValueI = map(sliderReadings[2], 0, 1023, 0, 255);

    paramNo = 2; // Send A
    // writes the value into the trkSettings array
    writeTrkSettings(currentTrkNo - 1, paramNo, sldValueI);
    // compiles the message
    toHubMSMessage = "/T";
    toHubMSMessage += currentTrkNo;
    toHubMSMessage += "/SendA,f,";
    toHubMSMessage += sldValueF;

    //    // debugging
    //    Serial2.print("trkSettings[");
    //    Serial2.print(currentTrkNo - 1);
    //    Serial2.print("][2]: ");
    //    Serial2.println(readTrkSettings(currentTrkNo - 1, 2));

  } else if ((sliderReadingsLast[1] <= sliderReadings[1] - jitter) || (sliderReadingsLast[1] >= sliderReadings[1] + jitter)) { // second right most slider - Send B

    float sldValueF = mapif(sliderReadings[1], 0, 1023, 0.0, 1.0);
    int sldValueI = map(sliderReadings[1], 0, 1023, 0, 255);

    paramNo = 3; // Send B
    // writes the value into the trkSettings array
    writeTrkSettings(currentTrkNo - 1, paramNo, sldValueI);
    // compiles the message
    toHubMSMessage = "/T";
    toHubMSMessage += currentTrkNo;
    toHubMSMessage += "/SendB,f,";
    toHubMSMessage += sldValueF;

    //    // debugging
    //    Serial2.print("trkSettings[");
    //    Serial2.print(currentTrkNo - 1);
    //    Serial2.print("][3]: ");
    //    Serial2.println(readTrkSettings(currentTrkNo - 1, 3));

  } else if ((sliderReadingsLast[0] <= sliderReadings[0] - jitter) || (sliderReadingsLast[0] >= sliderReadings[0] + jitter)) { // right most slider - Send C

    float sldValueF = mapif(sliderReadings[0], 0, 1023, 0.0, 1.0);
    int sldValueI = map(sliderReadings[0], 0, 1023, 0, 255);

    paramNo = 4; // Send C
    // writes the value into the trkSettings array
    writeTrkSettings(currentTrkNo - 1, paramNo, sldValueI);
    // compiles the message
    toHubMSMessage = "/T";
    toHubMSMessage += currentTrkNo;
    toHubMSMessage += "/SendC,f,";
    toHubMSMessage += sldValueF;

    //    // debugging
    //    Serial2.print("trkSettings[");
    //    Serial2.print(currentTrkNo - 1);
    //    Serial2.print("][4]: ");
    //    Serial2.println(readTrkSettings(currentTrkNo - 1, 4));

  }

  for (int i = 0; i < 4; i++) {
    sliderReadingsLast[i] = sliderReadings[i];
  }

  // send new message only if it's different to the last message
  if (toHubMSMessage != toHubMSMessageLast) {
    // set flag to true
    newMSMessage = true;
    // send the message
    sendHubOutgoing();
  }
  toHubMSMessageLast = toHubMSMessage;

  goTime4 = millis() + nextTime4;

}


//-------------------------------------------------------
void updateMSM() {
  int valueToSend;
  Wire.beginTransmission(0x1C);      // transmit to device 0x1C
  Wire.write(0); // sending sliders header
  for (int i = 0; i < 4; i++) {
    //valueToSend = analogRead(i);
    if (i == 3) { // Volume
      valueToSend = map(trkSettings[currentTrkNo - 1][3 - i], 0, 255, 0, 1023);
      // limit valueToSend to between 0-1023
      if (valueToSend > 1023) {
        valueToSend = 1023;
      } else if (valueToSend < 0) {
        valueToSend = 0;
      }
      //      // debugging
      //      Serial2.print("valueToSend 3: ");
      //      Serial2.println(valueToSend);
    } else { // Send A-C
      valueToSend = map(trkSettings[currentTrkNo - 1][4 - i], 0, 255, 0, 1023);
      // limit valueToSend to between 0-1023
      if (valueToSend > 1023) {
        valueToSend = 1023;
      } else if (valueToSend < 0) {
        valueToSend = 0;
      }
      //    // debugging
      //      Serial2.print("valueToSend ");
      //      Serial2.print(i);
      //      Serial2.print(": ");
      //      Serial2.println(valueToSend);
    }
    Wire.write(valueToSend >> 8);      // sends MSByte
    Wire.write(valueToSend & 0xff);    // sends LSByte
  }
  Wire.endTransmission();    // stop transmitting
  //readSliders();
  //delay(500);
  //readSliders();

}


// Mike Cook's demo code includes the code below to change the 'forward' and 'back' colours of the motorised slider LED strips
// TO DO - implement this via the 7" touchscreen display ?
//-------------------------------------------------------
void setMSMColours(byte forward[], byte back[]) {
  int valueToSend;
  Wire.beginTransmission(0x1C);      // transmit to device 0x1C
  Wire.write(1); // sending sliders header
  for (int i = 0; i < 12; i++) {
    Wire.write(forward[i]);        // sends colour
  }
  for (int i = 0; i < 12; i++) {
    // Wire.write(slidePost[i]);   // sends post colour
    Wire.write(back[i]);   // sends post colour
  }
  Wire.endTransmission();    // stop transmitting
}

