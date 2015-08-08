/* EspSoftSerial loopback example

Uses Serial1 transmitter on GPIO2 to transmit
Uses EspSoftSerial receiver on GPIO12 to receive

Scott Day 2015
github.com/scottwday/EspSoftSerial

*/

#include <EspSoftSerialRx.h>

EspSoftSerialRx serialRx;

void setup() {

   //Start the receiver on pin 12
   serialRx.begin(9600, 12);

   Serial1.begin(9600);
}

void loop() {

  unsigned char b;
  
  while (serialRx.read(b))
    Serial1.write((char)b);

  //Important weirdness: Call this at least once every 10 seconds
  serialRx.service();

  //Give the ESP a chance to do WiFi stuff
  delay(200);
}
