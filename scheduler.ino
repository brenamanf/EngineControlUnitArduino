


void initialiseSchedulers()
  {

    //Ignition Schedules, which uses timer 1
    TCCR1B = 0x00; //Disbale Timer1 while we set it up
    TCNT1 = 0; //Reset Timer Count
    TIFR1 = 0x00; //Timer1 INT Flag Reg: Clear Timer Overflow Flag
    TCCR1A = 0x00; //Timer1 Control Reg A: Wave Gen Mode normal
    TCCR1B = (1 << CS12); //Timer1 Control Reg B: Timer Prescaler set to 256. Refer to http://www.instructables.com/files/orig/F3T/TIKL/H3WSA4V7/F3TTIKLH3WSA4V7.jpg
    ignitionSchedule1.Status = OFF;
    
  }
  
/*
These 8 function turn a schedule on, provides the time to start and the duration and gives it callback functions.
All 8 functions operate the same, just on different schedules
Args:
startCallback: The function to be called once the timeout is reached
timeout: The number of uS in the future that the startCallback should be triggered
duration: The number of uS after startCallback is called before endCallback is called
endCallback: This function is called once the duration time has been reached
*/

//Ignition schedulers use Timer 1
void setIgnitionSchedule1(void (*startCallback)(), unsigned long timeout, unsigned long duration, void(*endCallback)())
  {
    if(ignitionSchedule1.Status == RUNNING) { return; } //Check that we're not already part way through a schedule
    //if(ignitionSchedule1.Status == PENDING) { return; } //Check that we're not already part way through a schedule
    
    //We need to calculate the value to reset the timer to (preload) in order to achieve the desired overflow time
    //As the timer is ticking every 16uS (Time per Tick = (Prescale)*(1/Frequency))
    //unsigned int absoluteTimeout = TCNT5 + (timeout / 16); //Each tick occurs every 16uS with the 256 prescaler, so divide the timeout by 16 to get ther required number of ticks. Add this to the current tick count to get the target time. This will automatically overflow as required
    unsigned int absoluteTimeout = TCNT1 + (timeout >> 4); //As above, but with bit shift instead of / 16
    OCR1A = absoluteTimeout;
    ignitionSchedule1.duration = duration;
    ignitionSchedule1.StartCallback = startCallback; //Name the start callback function
    ignitionSchedule1.EndCallback = endCallback; //Name the start callback function
    ignitionSchedule1.Status = PENDING; //Turn this schedule on
    TIMSK1 |= (1 << OCIE1A); //Turn on the A compare unit (ie turn on the interrupt)
  }
  
  


ISR(TIMER1_COMPA_vect) //ignitionSchedule1
  {
    noInterrupts();
    if (ignitionSchedule1.Status == PENDING) //Check to see if this schedule is turn on
    {
      ignitionSchedule1.StartCallback();
      ignitionSchedule1.Status = RUNNING; //Set the status to be in progress (ie The start callback has been called, but not the end callback)
      //unsigned int absoluteTimeout = TCNT5 + (ignitionSchedule1.duration / 16);
      unsigned int absoluteTimeout = TCNT1 + (ignitionSchedule1.duration >> 4); //Divide by 16
      OCR1A = absoluteTimeout;
    }
    else if (ignitionSchedule1.Status == RUNNING)
    {
       ignitionSchedule1.EndCallback();
       ignitionSchedule1.Status = OFF; //Turn off the schedule
       TIMSK1 &= ~(1 << OCIE1A); //Turn off this output compare unit (This simply writes 0 to the OCIE3A bit of TIMSK3)
    }
    interrupts();
  }

