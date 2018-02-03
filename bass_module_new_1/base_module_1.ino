///////////////////////////////////////
/// LIBRARIES + GLOBALS
///////////////////////////////////////

#include <Wire.h>
#include <MIDI.h>

int led = LED_BUILTIN;

byte b, bLast;

byte dataChange = B10000000;
bool dataChangeF = false;

byte jsUp = B1000;
byte jsDown = B0100;
byte jsLeft = B0010;
byte jsRight = B0001;

int channel = 0;
int velocityNOn = 127;
int velocityNOff = 64;
int pitch = 0;


///////////////////////////////////////
/// SET UP
///////////////////////////////////////

void setup()
{
  pinMode(led, OUTPUT);
  Wire.begin(0);             // join i2c bus (address optional for master)
  Serial.begin(9600);       // start serial for output
  MIDI.begin();              // Launch MIDI with default options
}


///////////////////////////////////////
/// MAIN LOOP
///////////////////////////////////////

void loop()
{
  //Serial.print("read: ");

  digitalWrite(led, HIGH);  // briefly flash the LED
  Wire.requestFrom(1, 1);   // request 1 byte from slave device #8

  while (Wire.available()) { // slave may send less than requested
    b = Wire.read();   // receive a byte as character
    Serial.print(b);        // print the character
    Serial.print(",");

    //    if ((b == jsUp) && (bLast == b)) {
    //      pitch = pitch + 1;
    //      if (pitch > 5) {
    //        pitch = 5;
    //      }
    //      Serial.print("pitch: ");
    //      Serial.println(pitch);
    //    } else if ((b == jsDown) && (bLast == b)) {
    //      pitch = pitch - 1;
    //      if (pitch < 0) {
    //        pitch = 0;
    //      }
    //      Serial.print("pitch: ");
    //      Serial.println(pitch);
    //    } else if ((b == jsLeft) && (bLast == b)) {
    //      channel = channel + 1;
    //      if (channel > 16) {
    //        channel = 1;
    //      }
    //      Serial.print("channel: ");
    //      Serial.println(channel);
    //    } else if ((b == jsRight) && (bLast == b)) {
    //      channel = channel - 1;
    //      if (channel < 1) {
    //        channel = 16;
    //      }
    //      Serial.print("channel: ");
    //      Serial.println(channel);
    //    }
    //
    //    //    if (b == dataChange) {
    //    //      dataChangeF = true;
    //    //    } else {
    //    //      dataChangeF = false;
    //    //    }
  }
  //
  //  //  if (dataChangeF == true) {
  //  //    Wire.requestFrom(8, 1);
  //  //  }
  //
  //  bLast = b;

  //Serial.println();
  digitalWrite(led, LOW);
  delay(100);
}
