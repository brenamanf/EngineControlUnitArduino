//Pin mappings
#define triggerInterrupt 0 // Interrupt Reluctor
#define pinCoil1 4 //Pin for coil 1
#define AccPinCoil 5 //Pin for coil 2
#define pinTrigger 2 //The CAS pin
#define pinMAP A1 //MAP sensor pin
#define pinIAT A2 //IAT sensor pin

#define MAX_RPM 9000 //This is the maximum rpm that the ECU will attempt to run at. It is NOT related to the rev limiter, but is instead dictates how fast certain operations will be allowed to run. Lower number gives better performance

#include "globals.h"
#include "utils.h"
#include "table.h"
#include "testing.h"
#include "scheduler.h"
#include "math.h"
#include "timers.h"

//Initialise variable
volatile unsigned long toothLastToothTime = 0; //The time (micros()) that the last tooth was registered 
volatile unsigned long toothLastMinusOneToothTime = 0; //The time (micros()) that the tooth before the last tooth was registered 
volatile unsigned long toothOneTime = 0; //The time (micros()) that tooth 1 last triggered 

int triggerFilterTime; // The shortest time (in uS) that pulses will be accepted (Used for debounce filtering)

unsigned long counter; 
unsigned long currentLoopTime; //The time the current loop started (uS) 
unsigned long previousLoopTime; //The time the previous loop started (uS) 
unsigned long scheduleStart; 
unsigned long scheduleEnd; 

struct statuses currentStatus; 
volatile int mainLoopCount; 
unsigned long secCounter; //The next time to increment 'runSecs' counter. 
int crankDegreesPerCylinder; //The number of crank degrees between cylinders (180 in a 4 cylinder, usually 120 in a 6 cylinder etc) 
int triggerToothAngle;
 

struct table3D ignitionTable; //8x8 ignition map
struct config2 configPage2;




void setup() {
  
  //Config en hard direct no memory
configPage2.dwellRun = 45;
configPage2.triggerTeeth = 4;
configPage2.HardRevLim = 75; // *100 = 7500RPM

  
  pinMode(pinCoil1, OUTPUT);
  pinMode(AccPinCoil, OUTPUT);
  digitalWrite(AccPinCoil, HIGH);

  
  //Setup the dummy fuel and ignition tables
  //dummyFuelTable(&fuelTable);
  dummyIgnitionTable(&ignitionTable);
  
  
  initialiseSchedulers();
  initialiseTimers();
  
  currentStatus.RPM = 0; 
  currentStatus.hasSync = false; 
  currentStatus.runSecs = 0;  
  currentStatus.secl = 0; 
  triggerFilterTime = (int)(1000000 / (MAX_RPM / 60 * configPage2.triggerTeeth)); //Trigger filter time is the shortest possible time (in uS) that there can be between crank teeth (ie at max RPM). Any pulses that occur faster than this time will be disgarded as noise 
  triggerToothAngle = 360 / configPage2.triggerTeeth; //The number of degrees that passes from tooth to tooth
  
  pinMode(pinTrigger, INPUT);
  digitalWrite(pinTrigger, HIGH);
  attachInterrupt(triggerInterrupt, trigger, FALLING); // Attach the crank trigger wheel interrupt (Hall sensor drags to ground when triggering)
  
  previousLoopTime = 0;
  currentLoopTime = micros();
  
  Serial.begin(115200);
  
   #ifdef sbi
    sbi(ADCSRA,ADPS2);
    cbi(ADCSRA,ADPS1);
    cbi(ADCSRA,ADPS0);
   #endif
  
  mainLoopCount = 0;

}

void loop() {
  
      mainLoopCount++;
      //Check for any requets from serial
      if ((mainLoopCount & 63) == 1) {
        Serial.print(currentStatus.RPM);
        Serial.print(" ");
        Serial.println(currentStatus.advance);
        //Do somthing every 64 main loop  
      }
      
    //Calculate the RPM based on the uS between the last 2 times tooth One was seen.
    previousLoopTime = currentLoopTime;
    currentLoopTime = micros();
    if ((currentLoopTime - toothLastToothTime) < 500000L) //Check how long ago the last tooth was seen compared to now. If it was more than half a second ago then the engine is probably stopped
    {
      noInterrupts();
      unsigned long revolutionTime = (toothOneTime - toothLastToothTime); //The time in uS that one revolution would take at current speed (The time tooth 1 was last seen, minus the time it was seen prior to that)
      interrupts();
      currentStatus.RPM = (ldiv(US_IN_MINUTE, revolutionTime).quot)/4; //Calc RPM based on last full revolution time (uses ldiv rather than div as US_IN_MINUTE is a long)
    }
    else
    {
      //We reach here if the time between teeth is too great. This VERY likely means the engine has stopped
      currentStatus.RPM = 0;
      currentStatus.hasSync = false;
      currentStatus.runSecs = 0; //Reset the counter for number of seconds running.
      secCounter = 0; //Reset our seconds counter.
    }
    
    
      //END SETTING STATUSES
      //-----------------------------------------------------------------------------------------------------
      //At the end : currentStatus.MAP
     // currentStatus.MAP = fastMap(analogRead(pinMAP), 0, 1023, 10, 260); //Get the current MAP value
      currentStatus.MAP = 0;
      currentStatus.advance = get3DTableValue(ignitionTable, currentStatus.MAP, currentStatus.RPM); //As above, but for ignition advance

      

 
      
}
    

  
//************************************************************************************************
//Interrupts

//These functions simply trigger the injector/coil driver off or on.
//NOTE: squirt status is changed as per http://www.msextra.com/doc/ms1extra/COM_RS232.htm#Acmd
void beginCoil1Charge() { digitalWrite(pinCoil1, HIGH); }
void endCoil1Charge() { digitalWrite(pinCoil1, LOW); }


//The trigger function is called everytime a crank tooth passes the sensor
void trigger()
  {
   // http://www.msextra.com/forums/viewtopic.php?f=94&t=22976
   // http://www.megamanual.com/ms2/wheel.htm
   noInterrupts(); //Turn off interrupts whilst in this routine
   volatile unsigned long curTime = micros();
   if ( (curTime - toothLastToothTime) < triggerFilterTime) { interrupts(); return; } //Debounce check. Pulses should never be less than triggerFilterTime, so if they are it means a false trigger. (A 36-1 wheel at 8000pm will have triggers approx. every 200uS)
   toothLastToothTime = toothOneTime ;
   toothOneTime  = curTime;
   
   //IGNITION   
   int timePerDegree = ldiv( (toothOneTime - toothLastToothTime) , triggerToothAngle).quot; //The time (uS) it is currently taking to move 1 degree
   Serial.println("fire");
   //ignition1StartAngle = currentStatus.advance - (div((configPage2.dwellRun*100), timePerDegree).quot ); 
   int dwell = (configPage2.dwellRun * 100); //Dwell is stored as ms * 10. ie Dwell of 4.3ms would be 43 in configPage2. This number therefore needs to be multiplied by 100 to get dwell in uS
   if (currentStatus.RPM < ((unsigned int)(configPage2.HardRevLim) * 100) ) //Check for hard cut rev limit (If we're above the hardcut limit, we simply don't set a spark schedule)
        { 
          setIgnitionSchedule1(beginCoil1Charge,
                    ((triggerToothAngle - advance) * timePerDegree)-dwell,//(ignition1StartAngle) * timePerDegree, // Timeout
                    dwell, // Duration
                    endCoil1Charge
                    );
        }
   
   interrupts(); //Turn interrupts back on
   



}
