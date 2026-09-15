#include<stdint.h>
#include"timer0.h"

unsigned long millis(void); // Arduino core

void timer0_init()
{
}

uint16_t timer0_getTick()
{
  return (uint16_t)millis();
}
