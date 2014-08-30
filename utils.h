#include <Arduino.h>
#define MS_IN_MINUTE 60000
#define US_IN_MINUTE 60000000 


/*
Returns how much free dynamic memory exists (between heap and stack)
*/
int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
