/*****************
 * Will connect to a HyperDeck Studio via RS422 over the Serial1 (UART1, pin 18/19) on an Arduino Mega
 * Requires a TTL-level to RS422 converter in between
 * Will play and pause the deck with regular intervals.
 *
 * HyperDeck Studio is a SSD disk video recorder from BlackMagic Design 
 * http://www.blackmagicdesign.com/products/hyperdeckstudio/
 *
 * 
 * - kasper
 *
 * This example code is under GNU GPL license
 */


#include <HyperDeckStudio.h>
HyperDeckStudio theHyperDeck(Serial1,1);  // Uses around 100 bytes...

//#include <MemoryFree.h>

// no-cost stream operator as described at 
// http://arduiniana.org/libraries/streaming/
template<class T>
inline Print &operator <<(Print &obj, T arg)
{  
  obj.print(arg); 
  return obj; 
}


void setup() {

  // Initialize the serial port:
  Serial.begin(9600);
  Serial << F("\n- - - - - - - -\nSerial Started.\n");

  // Initialize HyperDeck object:
  theHyperDeck.begin();
  theHyperDeck.serialOutput(true);  // Normally, this should be commented out!

  // Shows free memory:  
  //Serial << F("freeMemory()=") << freeMemory() << "\n";  
}

bool isOnline = false;

unsigned long timer=0;
bool playingNow = false;

void loop() {
  // Runloop:
  theHyperDeck.runLoop();

  // Display online status:
  if (theHyperDeck.isOnline()!=isOnline)  {
    isOnline=theHyperDeck.isOnline();
    Serial << "Deck Online Status: ";
    Serial << isOnline << "\n";
  }

  if (theHyperDeck.isOnline())  {
    if ((unsigned long)timer+2000 < (unsigned long)millis())  {

      timer=millis();

      if (!theHyperDeck.isInLocalModeOnly())  {  // Only makes sense to send play command if REM is enabled:
        if (!playingNow)  {
          theHyperDeck.doPlay();
        } 
        else {
          theHyperDeck.doStop();
        }
        playingNow=!playingNow;
      } else {
         Serial << "Press REM on HyperDeck (otherwise we can't remote control it!)\n";
      }
    }
  }
}

