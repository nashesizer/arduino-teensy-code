//Sends random notes from a Teensy 3.x out on USB MIDI
// when pin 7 is grounded
// Select USB Type = Serial + MIDI in the tools menu
// Mike Cook Jan 2018

const int channel = 1;
int noteFire = 7 ; // push buton to trigger random notes
char buf[30];
boolean newMessage = false;
byte ledPin = 13;
 
void setup() { 
Serial.begin(250000);
Serial.println("Serial / MIDI test");
pinMode(noteFire,INPUT_PULLUP);
pinMode(ledPin,OUTPUT);
}

//loop: wait for serial data, and interpret the message 
void loop () {
 if(digitalRead(noteFire) == 0) sendRandomNote();
 if(Serial.available() > 0) readIncomming();
 if(newMessage) handleMessage(); // do something with message 
 if((millis() & 0xFFF) == 0xFFF) sendTest(); // test message to processing
 }

void sendTest(){
  Serial.println("Teensy is running");
  delay(1);
}

void handleMessage(){
 // do something with the message - here just flash LED
 for(int i=0; i<10; i++){ // make this an odd number to leave the LED toggled
  digitalWrite(ledPin,!(digitalRead(ledPin))); // toggle LED
  delay(100);
 }
 echoMessage();
 newMessage = false; // ready for next messaage
}

void echoMessage(){
  Serial.print(" echoing:- ");
  int i=0;
  while(buf[i] != 10){
    Serial.write(buf[i]);
    i+=1;
  }
  Serial.write(10);
}

void readIncomming(){
  static int i=0;
 while(Serial.available() > 0 && newMessage == false) {
  buf[i] = Serial.read();
  if(buf[i] == 10) {  // LF recieved
    newMessage = true;
    i=0;
  }
  else {
  i+=1;
  }
 } 
}

void sendRandomNote(){
    int d1 = random(100, 1000); // length of note
  int d2 = random(800, 1000); // time between notes
  byte note = random(35,54); // what note number to send in Ableton drum range
  usbMIDI.sendNoteOn(note, 99, channel); // fixed velocity of 99
  delay(d1);
  usbMIDI.sendNoteOff(note, 99, channel);
  delay(d2); 
 }

