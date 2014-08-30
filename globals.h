#include <Arduino.h>

#define SIZE_BYTE 8;
#define SIZE_INT 16;

//The status struct contains the current values for all 'live' variables
//In current version this is 64 bytes
struct statuses {
  volatile boolean hasSync;
  unsigned int RPM;
  byte MAP;
  byte TPS; //The current TPS reading (0% - 100%)
  byte TPSlast; //The previous TPS reading
  byte tpsADC; //0-255 byte representation of the TPS
  byte tpsDOT;
  byte VE;
  byte O2;
  byte coolant;
  byte cltADC;
  byte IAT;
  byte iatADC;
  byte advance;
  volatile byte squirt;
  byte engine;
  unsigned long PW; //In uS
  volatile byte runSecs; //Counter of seconds since cranking commenced (overflows at 255 obviously)
  volatile byte secl; //Continous 
  volatile int loopsPerSecond;
  
  //Helpful bitwise operations:
  //Useful reference: http://playground.arduino.cc/Code/BitMath
  // y = (x >> n) & 1;    // n=0..15.  stores nth bit of x in y.  y becomes 0 or 1.
  // x &= ~(1 << n);      // forces nth bit of x to be 0.  all other bits left alone.
  // x |= (1 << n);       // forces nth bit of x to be 1.  all other bits left alone.
  
};

struct config2 {
  
    byte dwellRun;
    byte triggerTeeth;
    byte HardRevLim; //Hard rev limit (RPM/100)
    byte NbTeeth;
};



