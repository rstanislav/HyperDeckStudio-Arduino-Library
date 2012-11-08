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


/**
  Version 1.0.0
**/


#ifndef HyperDeckStudio_h
#define HyperDeckStudio_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "HardwareSerial.h"

class HyperDeckStudio
{
  private:
	HardwareSerial _HardSerial;
	uint8_t _UARTnum;
	
	bool _serialOutput;		// If set, the library will print status/debug information to the Serial object
	unsigned long _lastStatusRequestTime;
	unsigned long _lastDataRXTime;
	unsigned long _statusRequestPeriodLength;
	unsigned long _lastStatusReceivedTime;
	bool _awaitingResponse;

	uint8_t _RXbuffer[15];
	uint8_t _RXbufferPointer;
	uint8_t _RXbufferCheckSum;
	uint8_t _RXbufferExpectedLength;
	uint8_t _lastStatusCheckSum;

	bool _isOnline;

	bool _isPlaying;
	bool _isRecording;
	bool _isForwarding;
	bool _isRewinding;
	bool _isStopped;

	bool _isCassetteOut;
	bool _isInLocalModeOnly;
	bool _isStandby;
	bool _isInJogMode;
	bool _isDirectionBackwards;
	bool _isStill;
	bool _isNearEOT;
	bool _isEOT;		

  public:
    HyperDeckStudio(HardwareSerial &serial, uint8_t UARTnum);
    void begin();
    void runLoop();

  private:
	void _parsePacket();

  public:
  	void serialOutput(bool serialOutput);
	bool isOnline();
	bool TXready();


/********************************
* HyperDeckStudio State methods
* Returns the most recent information we've 
* got about the device state
 ********************************/

		// Deck status:
	bool isPlaying();		// If playing
	bool isRecording();		// If recording
	bool isForwarding();		// If fast forwarding x2 or more
	bool isRewinding();		// If rewinding x1 or more
	bool isStopped();		// If stopped

		// Other status registers:
	bool isCassetteOut();	
	bool isInLocalModeOnly();
	bool isStandby();		// If 
	bool isInJogMode();
	bool isDirectionBackwards();
	bool isStill();
	bool isNearEOT();	// 3 minutes before
	bool isEOT();	// 30 seconds before


/********************************
 * HyperDeckStudio Command methods
 * Asks the deck to do something
 ********************************/
	void doPlay();
	void doRecord();
	void doFastForward();
	void doRewind();
	void doStop();
};

#endif

