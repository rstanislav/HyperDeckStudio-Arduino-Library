/*****************
 * Will connect to a HyperDeck Studio via RS422 over the Serial1 (UART1, pin 18/19) on an Arduino Mega
 * Requires a TTL-level to RS422 converter in between
 * Will print status information to the Serial out, so open the serial monitor and watch.
 * If the connection is set up right, you should see status messages in the serial monitor as states
 * change on the HyperDeck Studio.
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
  Serial << F("\n- - - - - - - -\nSerial Started\n");

  // Initialize HyperDeck object:
  theHyperDeck.begin();
  theHyperDeck.serialOutput(true);  // Normally, this should be commented out!

  // Shows free memory:  
//  Serial << F("freeMemory()=") << freeMemory() << "\n";  
}

bool isOnline = false;

void loop() {
  // Runloop:
  theHyperDeck.runLoop();

  // Display online status:
  if (theHyperDeck.isOnline()!=isOnline)  {
    isOnline=theHyperDeck.isOnline();
    Serial << "Deck Online Status: ";
    Serial << isOnline << "\n";
  }
}

