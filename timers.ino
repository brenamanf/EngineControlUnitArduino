/*
Timers are used for having actions performed repeatedly at a fixed interval (Eg every 100ms)
They should not be confused with Schedulers, which are for performing an action once at a given point of time in the future

Timers are typically low resolution (Compared to Schedulers), with maximum frequency currently being approximately every 10ms
*/

void initialiseTimers()
{
   //Configure Timer2 for our low-freq interrupt code.
   TCCR2B = 0x00; //Disbale Timer2 while we set it up
   TCNT2 = 99; //Preload timer2 with 100 cycles, leaving 156 till overflow.
   TIFR2 = 0x00; //Timer2 INT Flag Reg: Clear Timer Overflow Flag
   TIMSK2 = 0x01; //Timer2 Set Overflow Interrupt enabled.
   TCCR2A = 0x00; //Timer2 Control Reg A: Wave Gen Mode normal
   TCCR2B = ((1 << CS10) | (1 << CS11) | (1 << CS12)); //Timer2 Set Prescaler to 5 (101), 1024 mode.
}


//Timer2 Overflow Interrupt Vector, called when the timer overflows.
//Executes every ~10ms.
ISR(TIMER2_OVF_vect)
{
  
  //Increment Loop Counters
  loop250ms++;
  loopSec++;
  
  
  //Loop executed every 250ms loop (10ms x 25 = 250ms)
  //Anything inside this if statement will run every 250ms.
  if (loop250ms == 25)
  {
    loop250ms = 0; //Reset Counter.
  }
  
  //Loop executed every 1 second (10ms x 100 = 1000ms)
  if (loopSec == 100)
  {
    loopSec = 0; //Reset counter.


    //**************************************************************************************************************************************************
    //This records the number of main loops the system has completed in the last second
    currentStatus.loopsPerSecond = mainLoopCount;
    mainLoopCount = 0;
    //**************************************************************************************************************************************************
    //increament secl (secl is simply a counter that increments every second and is used to track whether the system has unexpectedly reset
    currentStatus.secl++;
    

  }
      //Reset Timer2 to trigger in another ~10ms
    TCNT2 = 99; //Preload timer2 with 100 cycles, leaving 156 till overflow.
    TIFR2 = 0x00; //Timer2 INT Flag Reg: Clear Timer Overflow Flag
  
}
