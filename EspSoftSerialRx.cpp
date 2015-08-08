/* EspSoftSerial receiver
Scott Day 2015
github.com/scottwday/EspSoftSerial

Uses a pin change interrupt and the tick timer as a uart receiver
*/

#include <arduino.h>
#include "EspSoftSerialRx.h"

//Make sure static numInstances gets initialised to zero.
byte EspSoftSerialRx::_numInstances = 0;

//Static pointers to instances, allows interrupts to be forwarded
EspSoftSerialRx* EspSoftSerialRx::_instances[MAX_ESPSOFTSERIAL_INSTANCES];

// Supply the baud rate and the pin you're interested in
void EspSoftSerialRx::begin(const unsigned long baud, const byte rxPin)
{
  _rxPin = rxPin;
  
  _halfBitRate = (ESP8266_CLOCK/2) / baud;
  
  pinMode(rxPin, INPUT_PULLUP);

  //You can't register an interrupt on a class member, so these shims pass on the call
  if (_numInstances < MAX_ESPSOFTSERIAL_INSTANCES)
  {
	_instances[_numInstances] = this;
	switch(_numInstances)
	{
	  case 0: attachInterrupt(rxPin, onRxPinChange0, CHANGE); break;
	  case 1: attachInterrupt(rxPin, onRxPinChange1, CHANGE); break;
	  case 2: attachInterrupt(rxPin, onRxPinChange2, CHANGE); break;
	  case 3: attachInterrupt(rxPin, onRxPinChange3, CHANGE); break;
	}
	++_numInstances;
  }
}

//These shim the interrupt call into a class instance
// It's inefficient but it shields the cruft from the user
void EspSoftSerialRx::onRxPinChange0() { _instances[0]->onRxPinChange(); }
void EspSoftSerialRx::onRxPinChange1() { _instances[1]->onRxPinChange(); }
void EspSoftSerialRx::onRxPinChange2() { _instances[2]->onRxPinChange(); }
void EspSoftSerialRx::onRxPinChange3() { _instances[3]->onRxPinChange(); }

void EspSoftSerialRx::service()
{
  if (_inInterrupt)
	return;
  
  //Complete bytes with high MSB and Protect against rollover
  if (_lastChangeTicks)
  {
	unsigned long delta = (ESP.getCycleCount() - _lastChangeTicks) & 0x7FFFFFFF;
	//Has it been less than 16 bits worth of time since the last edge?
	
	if (delta > (_halfBitRate<<5))
	{
	  //Is there an incomplete byte that we need to finish now that it's been a while since the last edge?
	  // Complete the current byte with 1's
	  if ((_bitCounter) && (_bitCounter <= 10))
	  {
		addBits(11 - _bitCounter, 0);
	  }
	  
	  _lastChangeTicks = 0;
	}
  }
}

bool EspSoftSerialRx::read(byte& c)
{
  return _buffer.read(c);
}

//Returns the time difference saturated to 127 bits
byte EspSoftSerialRx::getNumBitPeriodsSinceLastChange(unsigned long ticks)
{
  //Get how long since last change in cpu ticks
  // If it's zero then it's been a long time
  unsigned long delta = 0xFFFFFFFF;
  if (_lastChangeTicks)
	delta = (ticks - _lastChangeTicks) & 0x7FFFFFFF;

  //Has it been less than 16 bits worth of time since the last edge?
  // 16 bits is the max we care about and we don't want to overflow.
  //If so, round up the number of half bits / 2 to get the number of bits
  // otherwise leave it as 0x7F to indicate a long time
  byte numBits = 0x7F;
  if (delta < (_halfBitRate<<5)) // <==> delta / _halfBitRate < 32
	numBits = ((byte)(delta / _halfBitRate) + 1) >> 1;
  
  return numBits;
}

//Add bits to _bitBuffer and increment _bitCounter up till 10 bits
inline void EspSoftSerialRx::addBits(byte numBits, byte value)
{
  //Add bits, LSB first, we'll eventually have 10 bits
  byte i;
  for (i=0; i<numBits && _bitCounter <= 10; i++)
  {
	_bitBuffer = _bitBuffer >> 1;
	if (!value) _bitBuffer |= 0x200;
	++_bitCounter;
  }

  //The previous byte in the buffer has finished once 10 bit times are up
  if ((!_errorState) && (_bitCounter > 10))
  {
	unsigned short controlBits = _bitBuffer & 0xFE01;
	if (controlBits == 0x200)
	{
	  char b = (char)((_bitBuffer>>1)&0xFF);
	  _buffer.write(b);
	}
	else
	{
	  _errorState = SOFTSERIAL_ERROR_STOPBIT;      
	}
	
	_bitCounter = 0;
	_bitBuffer = 0;
  }
}

//Interrupt handler
void EspSoftSerialRx::onRxPinChange()
{
  _inInterrupt = true;
  
  byte value = digitalRead(_rxPin);
  unsigned long ticks = ESP.getCycleCount();
  byte numBits = getNumBitPeriodsSinceLastChange(ticks);

  //Add bits to the buffer
  addBits(numBits, value);
  
  //The longest possible run of equal bits is 9
  if (numBits > 9)
  {
	if (value == 0) 
	{
	  //A falling edge after a long time means we're coming out of an idle condition
	  _errorState = 0;
	  _bitCounter = 0;
	}
	else
	{
	  //The line shouldn't ever be held down for longer than 9 bits
	  _errorState = SOFTSERIAL_ERROR_LONGLOW;
	}
  }

  //Check for beginning of start bit
  if ((_bitCounter == 0) && (value == 0))
  {
	_bitCounter = 1;
	_bitBuffer = 0;
  }

  //Update timer, be careful of zero: it's reserved to mean 'a long time ago'
  _lastChangeTicks = ticks;
  if (_lastChangeTicks == 0) _lastChangeTicks = 1;

  _inInterrupt = false;
}

	
	
	
    