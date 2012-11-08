/*
Copyright 2012 Kasper Skårhøj, SKAARHOJ, kasperskaarhoj@gmail.com

This file is part of the HyperDeckStudio library for Arduino

The HyperDeckStudio library is free software: you can redistribute it and/or modify 
it under the terms of the GNU General Public License as published by the 
Free Software Foundation, either version 3 of the License, or (at your 
option) any later version.

The HyperDeckStudio library is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
or FITNESS FOR A PARTICULAR PURPOSE. 
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with the HyperDeckStudio library. If not, see http://www.gnu.org/licenses/.

*/

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "HyperDeckStudio.h"
#include "HardwareSerial.h"


/**
 * Constructor.
 * Pass the hardware serial object you want to use AND make sure the UARTnum corresponds to it (Serial=0, Serial1=1,Serial2=2, Serial3=3)
 */
HyperDeckStudio::HyperDeckStudio(HardwareSerial &serial, uint8_t UARTnum) : _HardSerial(serial){
	_UARTnum = UARTnum;
}


/**
 * 
 */
void HyperDeckStudio::begin(){
  _serialOutput = false;	// By default, no Serial output for debuggin
  _lastStatusRequestTime = 0;			// Initialize it to zero
  _statusRequestPeriodLength = 200;	// ms between status request polls.
  _RXbufferPointer=0;
  _lastStatusCheckSum = 0;
  _isOnline = false;
  _lastStatusReceivedTime = 0;			// Initialize it to zero
  _awaitingResponse = false;
	
  _HardSerial.begin(38400);  		 // Used for communication with Hyper Deck Studio
  
  if (_UARTnum==0)	{	// Honestly, I never made it work with Serial, only Serial1-3!
  	UCSR0C = UCSR0C | B00110000;   // Sets the Odd Parity for Serial (thats the "0" in UCSR0C)
  }
  if (_UARTnum==1)	{
  	UCSR1C = UCSR1C | B00110000;   // Sets the Odd Parity for Serial1 (thats the "1" in UCSR1C)
  }
  if (_UARTnum==2)	{
  	UCSR2C = UCSR2C | B00110000;   // Sets the Odd Parity for Serial2 (thats the "2" in UCSR2C)
  }
  if (_UARTnum==3)	{
  	UCSR3C = UCSR3C | B00110000;   // Sets the Odd Parity for Serial3 (thats the "3" in UCSR3C)
  }
}

/**
 */
void HyperDeckStudio::runLoop() {

		// Read from Serial Port 1:
	  while (_HardSerial.available()) {
		_RXbuffer[_RXbufferPointer]=_HardSerial.read();
		_lastDataRXTime = millis();
		_awaitingResponse = false;	// As soon as we begin to receive bytes, we are not waiting for a response anymore - it's happening!
		//Serial.println(_RXbuffer[_RXbufferPointer],HEX);

		if (_RXbufferPointer==0)	{	// start of session... init it
			_RXbufferCheckSum = 0;	// Reset checksum register
			_RXbufferExpectedLength = (_RXbuffer[0] & B00001111)+2;	// Get length of the packet
		}	
			// If the pointer is equal to the expected length it means we have just read the checksum byte, so lets evaluate that:
		if (_RXbufferPointer == _RXbufferExpectedLength)	{
			if (_RXbufferCheckSum==_RXbuffer[_RXbufferPointer])	{
				HyperDeckStudio::_parsePacket();
			} else {
				if (_serialOutput)	Serial.println(F("Bad checksum"));
				delay(5);	// Let more data arrive, so we can flush it.
				while (_HardSerial.available()) {_HardSerial.read();} 
			}
			_RXbufferPointer=-1;	// Will be increased to zero in a moment.
		} else {
				// If we are not at the end (checksum byte), we continously calculate the checksum:
			_RXbufferCheckSum+=_RXbuffer[_RXbufferPointer];
		}
		_RXbufferPointer++;
		
			// Finally: Make sure we don't create a buffer overrun in memory at any rate:
		if (_RXbufferPointer>=15)	_RXbufferPointer=0;
	  }

		// Make sure we don't stall in data reception for any reason:
	 if ((_RXbufferPointer > 0 || _awaitingResponse) && ((unsigned long)_lastDataRXTime+1000 < (unsigned long)millis()))	{
			// Reset:
		_RXbufferPointer=0;
		_awaitingResponse = false;
		if (_serialOutput)	Serial.println(F("Had to reset RX situation."));
	 }

		// Send a status request to the HyperDeck once every XXX milliseonds:
	 if (TXready() && ((unsigned long)_lastStatusRequestTime+_statusRequestPeriodLength < (unsigned long)millis()))  {
	    _HardSerial.write(0x61);  // CMD1
	    _HardSerial.write(0x20);  // CMD2
	    _HardSerial.write(0x09);
	    _HardSerial.write(0x61+0x20+0x09);  // Checksum byte
	    _lastStatusRequestTime = millis();
		_awaitingResponse = true;
	 }
	
		// Go offline if a status has not been received recently:
	if ((unsigned long)_lastStatusReceivedTime+_statusRequestPeriodLength*3 < (unsigned long)millis())	{
		_isOnline = false;
	}
}

/**
 * Returns true if it's ok to send a command (it's not ok if we are currently receiving data from the deck or if we are waiting for a response!)
 */
bool HyperDeckStudio::TXready()	{
	return (_RXbufferPointer==0 && !_awaitingResponse);
}

/**
 * Will parse and act on the received data packet from the slave
 */
void HyperDeckStudio::_parsePacket()	{
	unsigned int cmd = ((unsigned int)_RXbuffer[0]<<8) + (unsigned int)_RXbuffer[1];
	
	switch(cmd)	{
		case 0x7920:
			_isOnline = true;
			_lastStatusReceivedTime = millis();
			if (_RXbufferCheckSum != _lastStatusCheckSum)	{	// Only update if new information (we use the checksum to determine that, since it depends entirely on the data transmitted)
				_lastStatusCheckSum = _RXbufferCheckSum;
				if (_serialOutput)	Serial.println(F("*** Updating states:"));
				
				// Playing:
				if (_isPlaying != ((_RXbuffer[3] & B00000001)>0))	{
					_isPlaying = ((_RXbuffer[3] & B00000001)>0);
					if (_serialOutput)	Serial.print(F("isPlaying: "));
					if (_serialOutput)	Serial.println(isPlaying());
				}
				
				// Recording
				if (_isRecording != ((_RXbuffer[3] & B00000010)>0))	{
					_isRecording = ((_RXbuffer[3] & B00000010)>0);
					if (_serialOutput)	Serial.print(F("isRecording: "));
					if (_serialOutput)	Serial.println(isRecording());
				}
				
				// Forwarding x2 or more:
				if (_isForwarding != ((_RXbuffer[3] & B00000100)>0))	{
					_isForwarding = ((_RXbuffer[3] & B00000100)>0);
					if (_serialOutput)	Serial.print(F("isForwarding: "));
					if (_serialOutput)	Serial.println(isForwarding());
				}
				
				// Rewinding x1 or more:
				if (_isRewinding != ((_RXbuffer[3] & B00001000)>0))	{
					_isRewinding = ((_RXbuffer[3] & B00001000)>0);
					if (_serialOutput)	Serial.print(F("isRewinding: "));
					if (_serialOutput)	Serial.println(isRewinding());
				}
				
				// Stopped:
				if (_isStopped != ((_RXbuffer[3] & B00100000)>0))	{
					_isStopped = ((_RXbuffer[3] & B00100000)>0);
					if (_serialOutput)	Serial.print(F("isStopped: "));
					if (_serialOutput)	Serial.println(isStopped());
				}
				
				// Cassette Out:
				if (_isCassetteOut != ((_RXbuffer[2] & B00100000)>0))	{
					_isCassetteOut = ((_RXbuffer[2] & B00100000)>0);
					if (_serialOutput)	Serial.print(F("isCassetteOut: "));
					if (_serialOutput)	Serial.println(isCassetteOut());
				}

				// Local mode (If REM is not enabled):
				if (_isInLocalModeOnly != ((_RXbuffer[2] & B00000001)>0))	{
					_isInLocalModeOnly = ((_RXbuffer[2] & B00000001)>0);
					if (_serialOutput)	Serial.print(F("isInLocalModeOnly: "));
					if (_serialOutput)	Serial.println(isInLocalModeOnly());
				}

				// Stand By:
				if (_isStandby != ((_RXbuffer[3] & B10000000)>0))	{
					_isStandby = ((_RXbuffer[3] & B10000000)>0);
					if (_serialOutput)	Serial.print(F("isStandby: "));
					if (_serialOutput)	Serial.println(isStandby());
				}

				// Jog Mode:
				if (_isInJogMode != ((_RXbuffer[4] & B00010000)>0))	{
					_isInJogMode = ((_RXbuffer[4] & B00010000)>0);
					if (_serialOutput)	Serial.print(F("isInJogMode: "));
					if (_serialOutput)	Serial.println(isInJogMode());
				}

				// Direction Backwards:
				if (_isDirectionBackwards != ((_RXbuffer[4] & B00000100)>0))	{
					_isDirectionBackwards = ((_RXbuffer[4] & B00000100)>0);
					if (_serialOutput)	Serial.print(F("isDirectionBackwards: "));
					if (_serialOutput)	Serial.println(isDirectionBackwards());
				}

				// Still:
				if (_isStill != ((_RXbuffer[4] & B00000010)>0))	{
					_isStill = ((_RXbuffer[4] & B00000010)>0);
					if (_serialOutput)	Serial.print(F("isStill: "));
					if (_serialOutput)	Serial.println(isStill());
				}

				// Near EOT:
				if (_isNearEOT != ((_RXbuffer[10] & B00100000)>0))	{
					_isNearEOT = ((_RXbuffer[10] & B00100000)>0);
					if (_serialOutput)	Serial.print(F("isNearEOT: "));
					if (_serialOutput)	Serial.println(isNearEOT());
				}

				// EOT:
				if (_isEOT != ((_RXbuffer[10] & B00010000)>0))	{
					_isEOT = ((_RXbuffer[10] & B00010000)>0);
					if (_serialOutput)	Serial.print(F("isEOT: "));
					if (_serialOutput)	Serial.println(isEOT());
				}
			}
		break;
		case 0x1001:
			if (_serialOutput)	Serial.println(F("ACK"));
		break;
		case 0x1112:
			if (_serialOutput)	Serial.println(F("NACK"));
			if (_serialOutput)	Serial.println(_RXbuffer[2]);
		break;
		default:
		if (_serialOutput)	{
			if (_serialOutput)	Serial.println(F("Unsupported Data Packet:"));
			for(uint8_t i=0; i<_RXbufferExpectedLength; i++)	{
				Serial.println(_RXbuffer[i], HEX);
			}
			if (_serialOutput)	Serial.println(F("==="));
		}
		break;
	}
}



/**
 * If set, will output debug information to Serial object.
 */
void HyperDeckStudio::serialOutput(const bool serialOutput) {
//	if (!(_HardSerial==Serial))	{
		_serialOutput = serialOutput;
//	}
}

/**
 * If this function returns false, you CANNOT count on any status received from the "is*" functions below. That will be the previous state (or whatever the memory contained upon boot if no connection ever)
 * So: Always make sure you are online with the HyperDeck before using status information.
 */
bool HyperDeckStudio::isOnline() {
	return _isOnline;
}



/**
 * 
 */
bool HyperDeckStudio::isPlaying() {
	return _isPlaying;
}

/**
 * 
 */
bool HyperDeckStudio::isRecording() {
	return _isRecording;
}

/**
 * 
 */
bool HyperDeckStudio::isForwarding() {
	return _isForwarding;
}

/**
 * 
 */
bool HyperDeckStudio::isRewinding() {
	return _isRewinding;
}

/**
 * 
 */
bool HyperDeckStudio::isStopped() {
	return _isStopped;
}

	// Other status registers:

/**
 * 
 */
bool HyperDeckStudio::isCassetteOut() {
	return _isCassetteOut;
}

/**
 * 
 */
bool HyperDeckStudio::isInLocalModeOnly() {
	return _isInLocalModeOnly;
}

/**
 * 
 */
bool HyperDeckStudio::isStandby() {
	return _isStandby;
}

/**
 * 
 */
bool HyperDeckStudio::isInJogMode() {
	return _isInJogMode;
}

/**
 * 
 */
bool HyperDeckStudio::isDirectionBackwards() {
	return _isDirectionBackwards;
}

/**
 * 
 */
bool HyperDeckStudio::isStill() {
	return _isStill;
}

/**
 * 
 */
bool HyperDeckStudio::isNearEOT() {
	return _isNearEOT;
}

/**
 * 
 */
bool HyperDeckStudio::isEOT() {
	return _isEOT;
}









/**
 * 
 */
void HyperDeckStudio::doPlay() {
	 if (TXready())  {
	    _HardSerial.write(0x20);  // CMD1
	    _HardSerial.write(0x01);  // CMD2
	    _HardSerial.write(0x20+0x01);  // Checksum byte
   	    _awaitingResponse = true;
	 } 
}

/**
 * 
 */
void HyperDeckStudio::doRecord() {
	 if (TXready())  {
	    _HardSerial.write(0x20);  // CMD1
	    _HardSerial.write(0x02);  // CMD2
	    _HardSerial.write(0x20+0x02);  // Checksum byte
  	    _awaitingResponse = true;
	 } 
}

/**
 * 
 */
void HyperDeckStudio::doFastForward() {
	 if (TXready())  {
	    _HardSerial.write(0x20);  // CMD1
	    _HardSerial.write(0x10);  // CMD2
	    _HardSerial.write(0x20+0x10);  // Checksum byte
  	    _awaitingResponse = true;
	 } 
}

/**
 * 
 */
void HyperDeckStudio::doRewind() {
	 if (TXready())  {
	    _HardSerial.write(0x20);  // CMD1
	    _HardSerial.write(0x20);  // CMD2
	    _HardSerial.write(0x20+0x20);  // Checksum byte
  	    _awaitingResponse = true;
	 } 
}

/**
 * 
 */
void HyperDeckStudio::doStop() {
	 if (TXready())  {
	    _HardSerial.write(0x20);  // CMD1
	    _HardSerial.write((uint8_t) 0x00);  // CMD2
	    _HardSerial.write(0x20+0x00);  // Checksum byte
  	    _awaitingResponse = true;
	 } 
}

